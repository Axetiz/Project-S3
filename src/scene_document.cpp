// Apsilume: Keep the live simulator, saved-scene metadata, reset baseline and selected identity coherent.
// Module relationships and invariants: docs/architecture.md.
#include "scene_document.hpp"
namespace orbitsimlite {
void SceneDocument::apply(const ScenarioPreset &scene) {
    initial = scene;
    name = scene.name;
    scale = scene.meters_to_pixels;
    units = scene.units;
    simulation = Simulator(scene.gravity, scene.dt, scene.integrator);
    simulation.set_substeps(scene.substeps);
    simulation.set_bodies(scene.bodies);
    // Preserve the assigned IDs in the reset baseline as well as in the live simulator.
    initial.bodies = simulation.get_bodies();
    // Prefer a moving non-star so the initial inspector/prediction has an informative target.
    selection.reset();
    if (!initial.bodies.empty()) {
        select_index(0);
        for (std::size_t i = 0; i < initial.bodies.size(); ++i) {
            if (!initial.bodies[i].is_star && initial.bodies[i].vel.length_squared() > 0) {
                select_index(int(i));
                break;
            }
        }
    }
    dirty = false;
    ++revision;
}
void SceneDocument::mark_edited() {
    dirty = true;
    ++revision;
}
void SceneDocument::select_index(int index) {
    const auto &bodies = simulation.get_bodies();
    selection = index >= 0 && std::size_t(index) < bodies.size()
                    ? std::optional<BodyId>(bodies[std::size_t(index)].id)
                    : std::nullopt;
}
int SceneDocument::selected_index() const {
    if (!selection)
        return -1;
    const auto index = simulation.index_of(*selection);
    return index ? int(*index) : -1;
}
Body *SceneDocument::selected_body() {
    const int index = selected_index();
    return index < 0 ? nullptr : &simulation.access_bodies()[std::size_t(index)];
}
const Body *SceneDocument::selected_body() const {
    const int index = selected_index();
    return index < 0 ? nullptr : &simulation.get_bodies()[std::size_t(index)];
}
ScenarioPreset SceneDocument::snapshot() const {
    return {name,
            simulation.get_bodies(),
            simulation.get_gravity(),
            simulation.get_dt(),
            simulation.get_integrator(),
            simulation.get_substeps(),
            scale,
            units};
}
} // namespace orbitsimlite
