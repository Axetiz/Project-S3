// Apsilume: Run a document without creating an SFML/OpenGL window.
// Supports an exact step count or wall-clock duration, with initial/final exported snapshots.
#pragma once
#include "snapshot_exporter.hpp"
namespace orbitsimlite {
int run_headless_session(SceneDocument &document, SimulationController &playback, SnapshotExporter &exporter,
                         const AppPaths &paths, std::optional<double> duration,
                         std::optional<std::size_t> steps = {});
}
