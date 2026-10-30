// Apsilume: Execute shared commands and protect unsaved state before a document replacement or exit.
// Module relationships and invariants: docs/architecture.md.
#include "application.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
namespace orbitsimlite {
namespace {
constexpr double half_pi = 1.5707963267948966;
std::string numeric(double value) {
    std::ostringstream text;
    text << std::setprecision(17) << value;
    return text.str();
}
bool replaces_document(Action action) {
    return action == Action::Quit || action == Action::StartScene || action == Action::LoadSaved ||
           action == Action::Reset || action == Action::Scenes;
}
} // namespace
void Application::dispatch(Command command, bool confirmed) {
    const Action action = command.action;
    // Defer the original command until the user decides how to handle unsaved edits.
    if (!confirmed && document_.dirty && replaces_document(action)) {
        pending_ = command;
        ui_.dialog = UiDialog::ConfirmDiscard;
        return;
    }
    if (ActionRegistry::info(action).needs_body && !document_.selected_body()) {
        status_ = "Select a body in the scene or body list first.";
        return;
    }
    // This pointer is frame-local: never reuse it after an action adds/removes/merges bodies.
    auto *body = document_.selected_body();
    // Edit increments follow the current view/scene scale; Shift requests the fine increment.
    const double movement = world_.camera.distance * 0.005 * (command.fine ? 0.1 : 1);
    const double velocity =
        body ? std::max(body->vel.length() * 0.05,
                        world_.camera.distance / (1000 * std::max(1e-9, document_.simulation.get_dt()))) *
                   (command.fine ? 0.1 : 1)
             : 0;
    const auto property = [&](Property key, double value) {
        status_ = editor_.set_property(document_, key, numeric(value));
    };
    switch (action) {
    // Playback and scene reset.
    case Action::TogglePlay:
        playback_.set_paused(!playback_.paused);
        break;
    case Action::Step:
        playback_.set_paused(true);
        playback_.single_step(document_);
        world_.record_trails(document_.simulation);
        break;
    case Action::Reset:
        apply_scene(document_.initial);
        break;
    // View-only commands leave the physical document and its dirty flag unchanged.
    case Action::Fit:
        world_.fit_camera(document_.simulation);
        follow_ = false;
        break;
    case Action::Prediction:
        world_.show_prediction = !world_.show_prediction;
        prediction_.invalidate();
        break;
    case Action::Grid:
        world_.show_reference_grid = !world_.show_reference_grid;
        break;
    case Action::Origin:
        world_.show_origin_axes = !world_.show_origin_axes;
        break;
    case Action::Vectors:
        world_.show_vectors = !world_.show_vectors;
        break;
    case Action::Gizmo:
        world_.show_gizmo = !world_.show_gizmo;
        break;
    case Action::Field:
        world_.field_mode = FieldMode((int(world_.field_mode) + 1) % 3);
        break;
    case Action::Top:
        world_.camera.yaw = -half_pi;
        world_.camera.pitch = half_pi;
        plane_ = EditorPlane::XY;
        break;
    case Action::Front:
        world_.camera.yaw = -half_pi;
        world_.camera.pitch = 0;
        plane_ = EditorPlane::XZ;
        break;
    case Action::Side:
        world_.camera.yaw = 0;
        world_.camera.pitch = 0;
        plane_ = EditorPlane::YZ;
        break;
    case Action::Oblique:
        world_.camera.yaw = -half_pi;
        world_.camera.pitch = 0.65;
        break;
    case Action::Projection:
        world_.camera.orthographic = !world_.camera.orthographic;
        break;
    case Action::Focus:
        world_.camera.target = body->pos;
        follow_ = !follow_;
        break;
    // Selection is stored as an ID; vector indices are resolved only when needed.
    case Action::NextBody: {
        const auto count = document_.simulation.get_bodies().size();
        if (count)
            document_.select_index((document_.selected_index() + 1) % int(count));
        break;
    }
    case Action::SelectBody:
        if (document_.simulation.index_of(command.body))
            document_.selection = command.body;
        break;
    case Action::Visibility: {
        if (const auto index = document_.simulation.index_of(command.body)) {
            auto &selected = document_.simulation.access_bodies()[*index];
            selected.visible = !selected.visible;
            document_.mark_edited();
        }
        break;
    }
    case Action::AddBody: {
        Vec3 point = cursor_;
        if (!cursor_valid_ || command.argument == 1) {
            const auto normal = plane_normal(plane_);
            point = world_.camera.target + normal * (plane_offset_ - Vec3::dot(world_.camera.target, normal));
        }
        editor_.add(document_, point);
        status_ = "Body added. Edit its labeled properties or drag a colored handle.";
        break;
    }
    case Action::DeleteBody:
        editor_.remove(document_);
        break;
    // Dialog transitions and persistence commands. Filesystem work is in application_scene.cpp.
    case Action::Save:
        ui_.begin_save(document_.name);
        break;
    case Action::SaveNamed:
        save_scene(false);
        break;
    case Action::ConfirmOverwrite:
        save_scene(true);
        break;
    case Action::Load:
        refresh_files();
        ui_.dialog = UiDialog::Load;
        break;
    case Action::SelectSaved:
        selected_saved_ = command.argument;
        break;
    case Action::LoadSaved: {
        if (selected_saved_ < 0 || std::size_t(selected_saved_) >= saved_files_.size()) {
            status_ = "Choose a saved scene first.";
            break;
        }
        ScenarioPreset scene;
        const auto result = SceneSerializer::load(saved_files_[std::size_t(selected_saved_)], scene);
        status_ = result.message;
        if (result) {
            apply_scene(scene);
            ui_.dialog = UiDialog::None;
            started_ = true;
            status_ = "Opened " + scene.name;
        }
        break;
    }
    case Action::DeleteSaved:
        if (!saved_files_.empty())
            ui_.dialog = UiDialog::ConfirmDelete;
        break;
    case Action::ConfirmDelete: {
        if (std::size_t(selected_saved_) < saved_files_.size()) {
            std::error_code ec;
            std::filesystem::remove(saved_files_[std::size_t(selected_saved_)], ec);
            status_ = ec ? "Could not delete file: " + ec.message() : "Saved scene deleted.";
        }
        refresh_files();
        ui_.dialog = UiDialog::Load;
        break;
    }
    case Action::Scenes:
        ui_.dialog = UiDialog::Startup;
        break;
    case Action::SelectScene:
        startup_scene_ = std::clamp(command.argument, 0, int(scenes_.size()) - 1);
        break;
    case Action::StartScene:
        if (!scenes_.empty()) {
            apply_scene(scenes_[std::size_t(startup_scene_)]);
            started_ = true;
            ui_.dialog = UiDialog::None;
        }
        break;
    // Rate changes schedule more/fewer fixed steps; solver configuration is edited separately.
    case Action::Slower:
        playback_.speed = std::max(0.0625, playback_.speed * 0.5);
        break;
    case Action::Faster:
        playback_.speed = std::min(64.0, playback_.speed * 2);
        break;
    case Action::ShorterForecast:
        forecast_range_ = std::max(0.1875, forecast_range_ * 0.5);
        break;
    case Action::LongerForecast:
        forecast_range_ = std::min(24.0, forecast_range_ * 2);
        break;
    case Action::Integrator:
        document_.simulation.set_integrator(
            document_.simulation.get_integrator() == Integrator::Euler ? Integrator::RK4 : Integrator::Euler);
        document_.mark_edited();
        break;
    case Action::CollisionPolicy:
        playback_.collisions = CollisionMode((int(playback_.collisions) + 1) % 4);
        playback_.cancel_collision();
        break;
    case Action::ConfirmCollision:
        playback_.confirm_collision(document_);
        break;
    case Action::Plane:
        plane_ = EditorPlane((int(plane_) + 1) % 3);
        plane_offset_ = 0;
        break;
    case Action::PlaneBack:
        plane_offset_ -= world_.camera.distance * 0.01;
        break;
    case Action::PlaneForward:
        plane_offset_ += world_.camera.distance * 0.01;
        break;
    // Scene mutations share EditorController validation/dirty tracking with the inspector.
    case Action::MassDown:
        property(Property::Mass, body->mass * 0.8);
        break;
    case Action::MassUp:
        property(Property::Mass, body->mass * 1.25);
        break;
    case Action::RadiusDown:
        property(Property::Radius, std::max(1.0, body->radius - 1));
        break;
    case Action::RadiusUp:
        property(Property::Radius, body->radius + 1);
        break;
    case Action::Star:
        body->is_star = !body->is_star;
        if (body->is_star)
            body->is_satellite = false;
        document_.mark_edited();
        break;
    case Action::Satellite:
        body->is_satellite = !body->is_satellite;
        if (body->is_satellite)
            body->is_star = false;
        document_.mark_edited();
        break;
    case Action::MoveLeft:
        editor_.move(document_, {-movement, 0, 0});
        break;
    case Action::MoveRight:
        editor_.move(document_, {movement, 0, 0});
        break;
    case Action::MoveUp:
        editor_.move(document_, {0, movement, 0});
        break;
    case Action::MoveDown:
        editor_.move(document_, {0, -movement, 0});
        break;
    case Action::MoveBack:
        editor_.move(document_, {0, 0, -movement});
        break;
    case Action::MoveForward:
        editor_.move(document_, {0, 0, movement});
        break;
    case Action::VelocityLeft:
        editor_.change_velocity(document_, {-velocity, 0, 0});
        break;
    case Action::VelocityRight:
        editor_.change_velocity(document_, {velocity, 0, 0});
        break;
    case Action::VelocityUp:
        editor_.change_velocity(document_, {0, velocity, 0});
        break;
    case Action::VelocityDown:
        editor_.change_velocity(document_, {0, -velocity, 0});
        break;
    case Action::VelocityBack:
        editor_.change_velocity(document_, {0, 0, -velocity});
        break;
    case Action::VelocityForward:
        editor_.change_velocity(document_, {0, 0, velocity});
        break;
    case Action::ToggleJson:
        exporter_.options.enable_json = !exporter_.options.enable_json;
        break;
    case Action::ToggleCsv:
        exporter_.options.enable_csv = !exporter_.options.enable_csv;
        break;
    // Resume a deferred command exactly once, or forget it on Cancel.
    case Action::Discard:
        if (pending_) {
            const auto next = *pending_;
            pending_.reset();
            document_.dirty = false;
            dispatch(next, true);
        }
        break;
    case Action::Cancel:
        pending_.reset();
        ui_.cancel_edit();
        ui_.dialog = started_ ? UiDialog::None : UiDialog::Startup;
        break;
    case Action::Help:
        ui_.dialog = UiDialog::Help;
        break;
    case Action::Quit:
        quit_ = true;
        break;
    default:
        break;
    }
}
} // namespace orbitsimlite
