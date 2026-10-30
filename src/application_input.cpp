// Apsilume: Turn pointer/key events into camera gestures, world-space edits and shared application commands.
// Module relationships and invariants: docs/architecture.md.
#include "application.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
namespace orbitsimlite {
namespace {
float dot(sf::Vector2f a, sf::Vector2f b) {
    return a.x * b.x + a.y * b.y;
}
} // namespace
void Application::pointer(sf::Vector2i point) {
    const auto delta = point - last_mouse_;
    ui_.pointer({float(point.x), float(point.y)});
    // Project pointer motion onto the selected screen segment, then recover its world-axis distance.
    if (drag_ == Drag::Axis && dragged_axis_) {
        const auto screen = dragged_axis_->end - dragged_axis_->start;
        const float length = dot(screen, screen);
        if (length > 4) {
            const double amount =
                dot({float(delta.x), float(delta.y)}, screen) / length * dragged_axis_->world_length;
            const Vec3 axes[] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
            editor_.move(document_, axes[dragged_axis_->axis] * amount);
        }
    } else if (drag_ == Drag::Pan) {
        world_.camera.pan(delta.x, delta.y, world_.height);
        follow_ = false;
    } else if (drag_ == Drag::Orbit)
        world_.camera.orbit(delta.x, delta.y);
    last_mouse_ = point;
    // Creation uses a world-plane intersection, not a fixed screen depth. Edge-on planes have no hit.
    const auto hit = world_.camera.intersect_plane(
        world_.camera.ray(point.x, point.y, world_.canvas_width(), world_.height), plane_normal(plane_),
        plane_offset_);
    cursor_valid_ = hit && point.x >= 0 && point.x < world_.canvas_width() && point.y > ui_.canvas_top() &&
                    point.y < int(world_.height) - 28;
    if (hit)
        cursor_ = *hit;
}
void Application::mouse_press(int button, sf::Vector2i point) {
    last_mouse_ = point;
    const sf::Vector2f screen{float(point.x), float(point.y)};
    // Widgets get the first chance to consume a click, preventing edits through UI panels/dialogs.
    if (button == 0 && ui_.click(screen, document_, editor_, [&](Command command) { dispatch(command); }))
        return;
    if (ui_.dialog != UiDialog::None || point.x >= world_.canvas_width() || point.y < ui_.canvas_top() ||
        point.y >= int(world_.height) - 28)
        return;
    if (button == 1) {
        const bool shift = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) ||
                           sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift);
        drag_ = shift ? Drag::Pan : Drag::Orbit;
        return;
    }
    if (button == 2) {
        drag_ = Drag::Pan;
        return;
    }
    if (button != 0)
        return;
    // Handles take priority over sphere picking; only the useful outer part of a handle is draggable.
    for (const auto &handle : world_.gizmo_handles(document_.simulation, document_.selected_index())) {
        const auto axis = handle.end - handle.start;
        const float length = dot(axis, axis);
        if (length < 16)
            continue;
        const float t = dot(screen - handle.start, axis) / length;
        const auto nearest = handle.start + axis * std::clamp(t, 0.0f, 1.0f);
        if (t > 0.25f && t < 1.1f && dot(screen - nearest, screen - nearest) < 81) {
            dragged_axis_ = handle;
            drag_ = Drag::Axis;
            return;
        }
    }
    // Pick the nearest visible sphere along the ray; screen-space overlap alone is insufficient.
    const auto ray = world_.camera.ray(point.x, point.y, world_.canvas_width(), world_.height);
    double nearest = std::numeric_limits<double>::max();
    document_.selection.reset();
    for (const auto &body : document_.simulation.get_bodies()) {
        if (!body.visible || !world_.camera.project(body.pos, world_.canvas_width(), world_.height))
            continue;
        const auto hit = world_.camera.hit_sphere(ray, body.pos, world_.body_world_radius(body));
        if (hit && *hit < nearest) {
            nearest = *hit;
            document_.selection = body.id;
        }
    }
}
void Application::mouse_release(int) {
    drag_ = Drag::None;
    dragged_axis_.reset();
}
void Application::wheel(float delta) {
    if (ui_.wheel({float(last_mouse_.x), float(last_mouse_.y)}, delta, world_))
        return;
    world_.camera.dolly(delta);
    pointer(last_mouse_);
}
void Application::key(sf::Keyboard::Key code, bool ctrl, bool shift) {
    // A focused field owns navigation/typing keys before global shortcuts are considered.
    if (ui_.key(code, ctrl, shift, document_, editor_, [&](Command command) { dispatch(command); }))
        return;
    Action action = ActionRegistry::from_key(code);
    if (ctrl && code == sf::Keyboard::Key::S)
        action = Action::Save;
    if (ctrl && code == sf::Keyboard::Key::O)
        action = Action::Load;
    dispatch({action, 0, 0, shift});
    if (action == Action::Save && code == sf::Keyboard::Key::K)
        ignore_text_ = 'k';
}
void Application::text(std::uint32_t codepoint) {
    if (ignore_text_) {
        const char expected = ignore_text_;
        ignore_text_ = 0;
        if (codepoint == std::uint32_t(expected) || codepoint == std::uint32_t(expected - 'a' + 'A'))
            return;
    }
    ui_.text(codepoint);
}
} // namespace orbitsimlite
