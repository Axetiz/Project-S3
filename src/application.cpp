// Apsilume: Window/session lifetime and the frame pipeline: events, edit invalidation, fixed steps,
// prediction, export, drawing. Module relationships and invariants: docs/architecture.md.
#include "application.hpp"
#include "headless_runner.hpp"
#include "scene_catalog.hpp"
#include <SFML/OpenGL.hpp>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
namespace orbitsimlite {
Application::Application(unsigned width, unsigned height, double scale) : paths_(AppPaths::discover()) {
    world_.width = width;
    world_.height = height;
    world_.scale = scale;
}
UiModel Application::model() const {
    return {document_,    playback_,         world_,  prediction_,    paths_,          scenes_,
            saved_files_, exporter_.options, status_, startup_scene_, selected_saved_, forecast_range_,
            plane_};
}
void Application::run(Simulator &input) {
    if (scenes_.empty())
        scenes_.push_back({"Current scene", input.get_bodies(), input.get_gravity(), input.get_dt(),
                           input.get_integrator(), input.get_substeps(), world_.scale});
    startup_scene_ = std::clamp(startup_scene_, 0, int(scenes_.size()) - 1);
    std::string path_error;
    paths_.ensure(path_error);
    // Import legacy user scenes without replacing or deleting the originals.
    const auto executable_directory = std::filesystem::absolute(executable_).parent_path();
    for (const auto &legacy : {executable_directory / "Exports", std::filesystem::current_path() / "Exports",
                               executable_directory.parent_path() / "Exports"}) {
        std::error_code ec;
        if (!std::filesystem::is_directory(legacy, ec) ||
            legacy.lexically_normal() == paths_.scenes.lexically_normal())
            continue;
        for (std::filesystem::directory_iterator it(legacy, ec), end; !ec && it != end; it.increment(ec)) {
            if (it->path().extension() == ".json" && it->is_regular_file(ec))
                std::filesystem::copy_file(it->path(), paths_.scenes / it->path().filename(),
                                           std::filesystem::copy_options::skip_existing, ec);
        }
    }
    refresh_files();
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.majorVersion = 2;
    settings.minorVersion = 1;
#if SFML_VERSION_MAJOR >= 3
    sf::RenderWindow window(sf::VideoMode({world_.width, world_.height}), "Apsilume 3D Editor",
                            sf::Style::Default, sf::State::Windowed, settings);
#else
    sf::RenderWindow window(sf::VideoMode(world_.width, world_.height), "Apsilume 3D Editor",
                            sf::Style::Default, settings);
#endif
#if SFML_VERSION_MAJOR >= 3
    window.setMinimumSize(sf::Vector2u{760, 600});
#endif
    if (!window.isOpen()) {
        std::cerr << "Cannot create the 3D editor window.\n";
        return;
    }
    if (!ui_.load_font(executable_)) {
        std::cerr << "Missing bundled font. Reinstall the application with its assets folder.\n";
        return;
    }
    unsigned frame_limit = 0, frame = 0, frame_rate = 60;
    if (const char *value = std::getenv("ORBITSIMLITE_FRAME_LIMIT"))
        frame_limit = unsigned(std::max(0, std::atoi(value)));
    if (const char *value = std::getenv("ORBITSIMLITE_FRAME_RATE"))
        frame_rate = unsigned(std::clamp(std::atoi(value), 10, 240));
    window.setFramerateLimit(frame_rate);
    apply_scene(scenes_[std::size_t(startup_scene_)]);
    started_ = !startup_screen_;
    ui_.dialog = startup_screen_ ? UiDialog::Startup : UiDialog::None;
    if (!path_error.empty())
        status_ = path_error;
    // Real elapsed time schedules solver work. Rendering FPS never changes numerical dt.
    auto previous = std::chrono::steady_clock::now();
    while (window.isOpen() && !quit_) {
        const auto now = std::chrono::steady_clock::now();
        const double seconds = std::chrono::duration<double>(now - previous).count();
        previous = now;
        // 1. Finish input first so this frame integrates and renders the latest user state.
        poll_events(window);
        if (quit_)
            break;
        // 2. Explicit edits make derived paths stale. Normal physics steps do not bump revision.
        if (document_.revision != observed_revision_) {
            prediction_.invalidate();
            world_.clear_trails();
            observed_revision_ = document_.revision;
            playback_.set_paused(playback_.paused);
        }
        // 3. Dialogs/field edits and axis dragging freeze advancement; the window keeps drawing.
        if (started_ && !ui_.blocks_simulation() && drag_ != Drag::Axis)
            playback_.advance(document_, seconds, [&] { world_.record_trails(document_.simulation); });
        const auto *selected = document_.selected_body();
        if (follow_ && selected)
            world_.camera.target = selected->pos;
        // 4. Prediction reads a simulation copy; export reads the live state after stepping.
        if (world_.show_prediction)
            prediction_.update(document_.simulation, document_.selected_index(), forecast_range_,
                               world_.camera.distance,
                               playback_.paused || ui_.blocks_simulation() || bool(playback_.prompt));
        if (started_)
            exporter_.update(document_, playback_, seconds);
        if (!exporter_.error.empty())
            status_ = exporter_.error;
        else if (playback_.catching_up)
            status_ = "Simulation is catching up. Reduce playback speed if needed.";
        // 5. Draw world geometry, then restore a full-window SFML view for the UI.
        const RenderContext context{document_.selected_index(),
                                    plane_,
                                    plane_offset_,
                                    cursor_,
                                    cursor_valid_ && ui_.dialog == UiDialog::None,
                                    &prediction_};
        window.clear(sf::Color(7, 11, 20));
        window.pushGLStates();
        world_.draw_world(window, document_.simulation, context);
        if (const auto error = glGetError(); error != GL_NO_ERROR)
            std::cerr << "OpenGL error: " << error << '\n';
        window.popGLStates();
        // Raw OpenGL and SFML share a context. Reset state before drawing glyph-atlas widgets.
        window.resetGLStates();
        glViewport(0, 0, int(world_.width), int(world_.height));
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_SCISSOR_TEST);
#if SFML_VERSION_MAJOR >= 3
        window.setView(sf::View(sf::FloatRect({0, 0}, {float(world_.width), float(world_.height)})));
#else
        window.setView(sf::View(sf::FloatRect(0, 0, float(world_.width), float(world_.height))));
#endif
        ui_.draw(window, model());
        dump_ui_state();
        // Optional smoke-test capture happens before buffer swap, while this frame is readable.
        if (frame_limit && frame + 1 == frame_limit) {
            if (const char *path = std::getenv("ORBITSIMLITE_SCREENSHOT")) {
                sf::Texture texture;
#if SFML_VERSION_MAJOR >= 3
                if (texture.resize(window.getSize())) {
#else
                if (texture.create(world_.width, world_.height)) {
#endif
                    texture.update(window);
                    if (!texture.copyToImage().saveToFile(path))
                        std::cerr << "Screenshot failed.\n";
                }
            }
        }
        window.display();
        if (frame_limit && ++frame >= frame_limit)
            break;
    }
    if (started_)
        exporter_.update(document_, playback_, 0, true);
    input = document_.simulation;
}
void Application::run_headless(Simulator &input, std::optional<double> duration, const std::string &name) {
    document_.apply({name, input.get_bodies(), input.get_gravity(), input.get_dt(), input.get_integrator(),
                     input.get_substeps(), world_.scale});
    playback_.configure(document_);
    playback_.collisions = CollisionMode::Ignore;
    run_headless_session(document_, playback_, exporter_, paths_, duration);
    input = document_.simulation;
}
} // namespace orbitsimlite
