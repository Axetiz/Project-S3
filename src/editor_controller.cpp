// Apsilume: Apply user edits at the document boundary and mark successful mutations for saving/cache
// invalidation. Module relationships and invariants: docs/architecture.md.
#include "editor_controller.hpp"
#include "utils.hpp"
#include <algorithm>
#include <cmath>
namespace orbitsimlite {
std::string EditorController::set_property(SceneDocument &document, Property property,
                                           const std::string &text) {
    Body *body = document.selected_body();
    if (property == Property::Name) {
        if (!body)
            return "Select a body first.";
        if (text.empty())
            return "Body name cannot be empty.";
        body->name = text;
        document.mark_edited();
        return {};
    }
    std::size_t used = 0;
    double value = 0;
    try {
        value = std::stod(text, &used);
    } catch (...) {
        return "Enter a number, for example 1.5 or 1e24.";
    }
    if (used != text.size() || !std::isfinite(value))
        return "Enter one finite number.";
    if ((property == Property::Mass || property == Property::Radius || property == Property::Timestep) &&
        value <= 0)
        return "This value must be greater than zero.";
    if (property == Property::Gravity && value < 0)
        return "Gravity cannot be negative.";
    if (property == Property::Timestep)
        document.simulation.set_dt(value);
    else if (property == Property::Gravity)
        document.simulation.set_gravity(value);
    else if (property == Property::Substeps) {
        if (value < 1 || value > 4096 || std::floor(value) != value)
            return "Use a whole number between 1 and 4096.";
        document.simulation.set_substeps(int(value));
    } else {
        if (!body)
            return "Select a body first.";
        switch (property) {
        case Property::Mass:
            body->mass = value;
            break;
        case Property::Radius:
            body->radius = value;
            break;
        case Property::PositionX:
            body->pos.x = value;
            break;
        case Property::PositionY:
            body->pos.y = value;
            break;
        case Property::PositionZ:
            body->pos.z = value;
            break;
        case Property::VelocityX:
            body->vel.x = value;
            break;
        case Property::VelocityY:
            body->vel.y = value;
            break;
        case Property::VelocityZ:
            body->vel.z = value;
            break;
        default:
            break;
        }
    }
    document.mark_edited();
    return {};
}
void EditorController::move(SceneDocument &document, const Vec3 &delta) {
    if (auto *b = document.selected_body()) {
        b->pos += delta;
        document.mark_edited();
    }
}
void EditorController::change_velocity(SceneDocument &document, const Vec3 &delta) {
    if (auto *b = document.selected_body()) {
        b->vel += delta;
        document.mark_edited();
    }
}
void EditorController::add(SceneDocument &document, const Vec3 &point) {
    const auto *selected = document.selected_body();
    Body body(selected ? selected->mass : (document.units == "dimensionless" ? 1 : 5e24), point,
              selected ? selected->vel : Vec3{}, 8, random_color_u32(unsigned(document.revision + 1)), false,
              false, "Body " + std::to_string(document.simulation.get_bodies().size() + 1));
    document.simulation.add_body(body);
    document.select_index(int(document.simulation.get_bodies().size()) - 1);
    document.mark_edited();
}
void EditorController::remove(SceneDocument &document) {
    const int index = document.selected_index();
    if (index < 0)
        return;
    auto &bodies = document.simulation.access_bodies();
    bodies.erase(bodies.begin() + index);
    document.selection.reset();
    document.mark_edited();
}
} // namespace orbitsimlite
