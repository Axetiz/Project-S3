// Apsilume: Value types shared by the numerical core, scene files and graphical editor.
// A preset is a complete starting state; view and playback controls are session state.
#pragma once
#include "simulator.hpp"
#include <string>
namespace orbitsimlite {
// Cyclic UI commands rely on contiguous enum values; update their wrap counts when extending these.
enum class EditorPlane { XY, XZ, YZ };
enum class FieldMode { None, Arrows, Grid };
enum class CollisionMode { PromptRemoveSmaller, RemoveSmaller, Merge, Ignore };
struct ScenarioPreset {
    std::string name;
    std::vector<Body> bodies;
    double gravity{Physics::DefaultG};
    double dt{1}; // One public Simulator::step(); each substep uses dt/substeps.
    Integrator integrator{Integrator::RK4};
    int substeps{1};
    double meters_to_pixels{2e-9}; // Historical name: fixed reference scale for display/collision radii.
    std::string units{"SI"};       // Labels/defaults only. The engine does not convert unit systems.
};
struct OutputOptions {
    bool enable_json{true};
    bool enable_csv{false};
};
Vec3 plane_normal(EditorPlane plane);
Vec3 plane_point(EditorPlane plane, double offset, double u, double v);
const char *plane_name(EditorPlane plane);
const char *integrator_name(Integrator integrator);
const char *collision_name(CollisionMode mode);
} // namespace orbitsimlite
