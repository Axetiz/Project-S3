// Apsilume: Map edit-plane enums to world-space bases and provide readable names for shared scene options.
// Module relationships and invariants: docs/architecture.md.
#include "scene.hpp"
namespace orbitsimlite {
Vec3 plane_normal(EditorPlane plane) {
    switch (plane) {
    case EditorPlane::XY:
        return {0, 0, 1};
    case EditorPlane::XZ:
        return {0, 1, 0};
    case EditorPlane::YZ:
        return {1, 0, 0};
    }
    return {0, 0, 1};
}
Vec3 plane_point(EditorPlane plane, double offset, double u, double v) {
    switch (plane) {
    case EditorPlane::XY:
        return {u, v, offset};
    case EditorPlane::XZ:
        return {u, offset, v};
    case EditorPlane::YZ:
        return {offset, u, v};
    }
    return {};
}
const char *plane_name(EditorPlane plane) {
    switch (plane) {
    case EditorPlane::XY:
        return "XY";
    case EditorPlane::XZ:
        return "XZ";
    case EditorPlane::YZ:
        return "YZ";
    }
    return "XY";
}
const char *integrator_name(Integrator integrator) {
    return integrator == Integrator::Euler ? "Symplectic Euler" : "RK4";
}
const char *collision_name(CollisionMode mode) {
    switch (mode) {
    case CollisionMode::PromptRemoveSmaller:
        return "Ask before removal";
    case CollisionMode::RemoveSmaller:
        return "Remove smaller";
    case CollisionMode::Merge:
        return "Merge";
    case CollisionMode::Ignore:
        return "Ignore";
    }
    return "Ignore";
}
} // namespace orbitsimlite
