// Apsilume: Drive exact-step or wall-clock experiments without creating graphics resources, then finish
// output snapshots. Module relationships and invariants: docs/architecture.md.
#include "headless_runner.hpp"
#include <chrono>
#include <iostream>
#include <thread>
namespace orbitsimlite {
int run_headless_session(SceneDocument &document, SimulationController &playback, SnapshotExporter &exporter,
                         const AppPaths &paths, std::optional<double> duration,
                         std::optional<std::size_t> steps) {
    std::string error;
    if (!paths.ensure(error)) {
        std::cerr << error << '\n';
        return 1;
    }
    exporter.begin(paths, document.name);
    exporter.update(document, playback, 0, true);
    // Exact-step mode bypasses wall-clock scheduling for reproducible numerical experiments.
    if (steps) {
        for (std::size_t i = 0; i < *steps; ++i)
            playback.single_step(document);
    } else {
        // Missing duration means continuous operation; the controller still integrates fixed dt steps.
        const auto start = std::chrono::steady_clock::now();
        auto previous = start;
        while (true) {
            const auto now = std::chrono::steady_clock::now();
            if (duration && std::chrono::duration<double>(now - start).count() >= *duration)
                break;
            const double seconds = std::chrono::duration<double>(now - previous).count();
            previous = now;
            playback.advance(document, seconds);
            exporter.update(document, playback, seconds);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    exporter.update(document, playback, 0, true);
    if (!exporter.error.empty()) {
        std::cerr << exporter.error << '\n';
        return 1;
    }
    std::cout << "Scene: " << document.name << "\nSimulated time: " << document.simulation.get_time()
              << "\nData folder: " << paths.root << '\n';
    if (exporter.options.enable_json)
        std::cout << "JSON: " << exporter.json_path << '\n';
    if (exporter.options.enable_csv)
        std::cout << "CSV: " << exporter.csv_path << '\n';
    return 0;
}
} // namespace orbitsimlite
