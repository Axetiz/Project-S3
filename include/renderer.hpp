// Apsilume: Compatibility facade for older code that constructed Renderer directly.
// New graphical behavior belongs to Application and Renderer3D; keep this adapter thin.
#pragma once
#include "application.hpp"
namespace orbitsimlite {
// Compatibility facade for the existing demos and external callers.
class Renderer {
  public:
    Renderer(unsigned width = 1280, unsigned height = 840, double scale = 2e-9)
        : application_(width, height, scale), scale_(scale) {}
    void set_presets(const std::vector<ScenarioPreset> &scenes) { application_.set_presets(scenes); }
    void set_output_options(OutputOptions options) { application_.set_output_options(options); }
    void run(Simulator &simulation) { application_.run(simulation); }
    void run_headless(Simulator &simulation, std::optional<double> duration,
                      const std::string &name = "Headless Scene") {
        application_.run_headless(simulation, duration, name);
    }
    bool save_scene_json(const Simulator &simulation, const std::string &name) const {
        ScenarioPreset scene{name,
                             simulation.get_bodies(),
                             simulation.get_gravity(),
                             simulation.get_dt(),
                             simulation.get_integrator(),
                             simulation.get_substeps(),
                             scale_};
        return bool(
            SceneSerializer::save(AppPaths::discover().scenes / SceneSerializer::filename(name), scene));
    }
    bool load_scene_json(const std::string &filename, ScenarioPreset &scene) const {
        return bool(SceneSerializer::load(AppPaths::discover().scenes / filename, scene));
    }

  private:
    Application application_;
    double scale_;
};
} // namespace orbitsimlite
