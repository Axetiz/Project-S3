// Apsilume: Camera fitting, viewport changes, ID-keyed trail storage and axis-handle geometry shared with
// picking. Module relationships and invariants: docs/architecture.md.
#include "renderer3d.hpp"
#include <algorithm>
#include <set>
namespace orbitsimlite {
double Renderer3D::canvas_width() const {
    return std::max(1.0, double(width) - panel_width);
}
double Renderer3D::body_world_radius(const Body &body) const {
    return body.radius / std::max(scale, 1e-30);
}
sf::Vector2f Renderer3D::world_to_screen(const Vec3 &point) const {
    const auto projected = camera.project(point, canvas_width(), height);
    return projected ? sf::Vector2f{float(projected->x), float(projected->y)}
                     : sf::Vector2f{-100000, -100000};
}
void Renderer3D::resize(unsigned new_width, unsigned new_height) {
    const float new_panel = std::clamp(float(new_width) * 0.36f, 280.0f, 360.0f);
    camera.resize_viewport(canvas_width(), height, std::max(1.0, double(new_width) - new_panel), new_height);
    width = std::max(1u, new_width);
    height = std::max(1u, new_height);
    panel_width = new_panel;
}
void Renderer3D::fit_camera(const Simulator &simulation) {
    const auto &bodies = simulation.get_bodies();
    Vec3 lo{}, hi{};
    bool first = true;
    for (const auto &body : bodies) {
        if (!body.visible)
            continue;
        if (first) {
            lo = hi = body.pos;
            first = false;
        }
        lo.x = std::min(lo.x, body.pos.x);
        lo.y = std::min(lo.y, body.pos.y);
        lo.z = std::min(lo.z, body.pos.z);
        hi.x = std::max(hi.x, body.pos.x);
        hi.y = std::max(hi.y, body.pos.y);
        hi.z = std::max(hi.z, body.pos.z);
    }
    camera.target = (lo + hi) * 0.5;
    double radius = 120 / std::max(scale, 1e-30);
    for (const auto &body : bodies)
        if (body.visible)
            radius = std::max(radius, Vec3::distance(body.pos, camera.target) + body_world_radius(body));
    camera.distance = radius * 1.25 / (Camera3D::tangent * std::min(1.0, canvas_width() / height));
}
void Renderer3D::record_trails(const Simulator &simulation) {
    std::set<BodyId> active;
    for (const auto &body : simulation.get_bodies()) {
        active.insert(body.id);
        auto &trail = trails_[body.id];
        trail.push_back(body.pos);
        // Bound retained geometry per body; cached paths must not grow for the lifetime of a session.
        if (trail.size() > 512)
            trail.pop_front();
    }
    for (auto it = trails_.begin(); it != trails_.end();) {
        if (!active.count(it->first))
            it = trails_.erase(it);
        else
            ++it;
    }
}
void Renderer3D::clear_trails() {
    trails_.clear();
}
std::vector<AxisHandle> Renderer3D::gizmo_handles(const Simulator &simulation, int index) const {
    if (!show_gizmo || index < 0 || std::size_t(index) >= simulation.get_bodies().size())
        return {};
    const auto &body = simulation.get_bodies()[std::size_t(index)];
    if (!body.visible || !camera.project(body.pos, canvas_width(), height))
        return {};
    const double depth = camera.orthographic ? camera.distance : camera.view(body.pos).z;
    const double length = std::max(body_world_radius(body) * 3, depth * Camera3D::tangent * 0.24);
    const Vec3 axes[] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    std::vector<AxisHandle> handles;
    for (int axis = 0; axis < 3; ++axis)
        handles.push_back(
            {axis, world_to_screen(body.pos), world_to_screen(body.pos + axes[axis] * length), length});
    return handles;
}
} // namespace orbitsimlite
