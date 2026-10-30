// Coordinate document replacement and named saves with presentation/cache state.
// Serialization itself belongs to SceneSerializer; user choices belong to dispatch().
#include "application.hpp"
#include <algorithm>
namespace orbitsimlite {
void Application::apply_scene(const ScenarioPreset &scene) {
    // Applying a scene resets model time and establishes the Reset baseline.
    document_.apply(scene);
    playback_.configure(document_);
    playback_.speed = initial_speed_;
    // Discard session-derived presentation state that belonged to the previous bodies.
    world_.scale = document_.scale;
    world_.camera = Camera3D{};
    world_.fit_camera(document_.simulation);
    world_.clear_trails();
    world_.record_trails(document_.simulation);
    prediction_.invalidate();
    follow_ = false;
    plane_ = EditorPlane::XY;
    plane_offset_ = 0;
    ui_.cancel_edit();
    observed_revision_ = document_.revision;
    exporter_.begin(paths_, document_.name);
    status_ = "Click a body, drag a colored handle, or edit a labeled field.";
}
void Application::refresh_files() {
    saved_files_.clear();
    std::error_code ec;
    for (std::filesystem::directory_iterator it(paths_.scenes, ec), end; !ec && it != end; it.increment(ec)) {
        if (it->is_regular_file(ec) && it->path().extension() == ".json")
            saved_files_.push_back(it->path());
    }
    if (ec)
        status_ = "Cannot read scene folder: " + ec.message();
    std::sort(saved_files_.begin(), saved_files_.end());
    selected_saved_ = std::clamp(selected_saved_, 0, std::max(0, int(saved_files_.size()) - 1));
}
void Application::save_scene(bool overwrite) {
    const auto name = ui_.scene_name.empty() ? document_.name : ui_.scene_name;
    const auto path = paths_.scenes / SceneSerializer::filename(name);
    std::error_code ec;
    if (!overwrite && std::filesystem::exists(path, ec)) {
        ui_.dialog = UiDialog::ConfirmOverwrite;
        return;
    }
    auto scene = document_.snapshot();
    scene.name = name;
    // Commit the document name/clean flag only after a successful whole-file save.
    const auto result = SceneSerializer::save(path, scene);
    status_ = result.message;
    if (!result)
        return;
    document_.name = name;
    document_.dirty = false;
    ui_.dialog = UiDialog::None;
    refresh_files();
    if (pending_) {
        const auto next = *pending_;
        pending_.reset();
        dispatch(next, true);
    }
}
} // namespace orbitsimlite
