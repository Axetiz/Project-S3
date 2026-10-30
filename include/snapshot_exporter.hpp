// Apsilume: Throttled live-state snapshots for external readers and headless experiments.
// A session uses stable output filenames; each update replaces their contents atomically.
#pragma once
#include "app_paths.hpp"
#include "scene_document.hpp"
#include "simulation_controller.hpp"
namespace orbitsimlite {
class SnapshotExporter {
  public:
    OutputOptions options;
    std::filesystem::path json_path, csv_path;
    std::string error;
    // Choose one JSON/CSV filename pair for this scene session; filenames stay stable until the next begin().
    void begin(const AppPaths &paths, const std::string &name);
    // seconds is real elapsed time. Ordinary writes are limited to 10 Hz; force bypasses that cadence.
    void update(const SceneDocument &document, const SimulationController &controller, double real_seconds,
                bool force = false);

  private:
    double elapsed_{0};
};
} // namespace orbitsimlite
