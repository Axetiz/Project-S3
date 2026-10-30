// Apsilume: Parse launch options once, then build the same scene/controller/exporter services for GUI or
// headless use. Module relationships and invariants: docs/architecture.md.
#include "app_launcher.hpp"
#include "headless_runner.hpp"
#include "scene_catalog.hpp"
#ifdef ORBITSIMLITE_HAS_GUI
#include "application.hpp"
#endif
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
namespace orbitsimlite {
int run_product_app(int argc, char **argv, int default_scene) {
    try {
        bool headless = false, startup = true;
        OutputOptions outputs;
        std::optional<double> duration;
        std::optional<std::size_t> steps;
        double speed = 1;
        auto scenes = make_builtin_scenes();
        const auto number = [](const std::string &s) {
            std::size_t end = 0;
            const double n = std::stod(s, &end);
            if (end != s.size() || !std::isfinite(n))
                throw std::runtime_error("Expected a finite number: " + s);
            return n;
        };
        for (int i = 1; i < argc; ++i) {
            const std::string flag = argv[i];
            const auto value = [&]() -> std::string {
                if (i + 1 >= argc)
                    throw std::runtime_error("Missing value after " + flag);
                return argv[++i];
            };
            if (flag == "--help" || flag == "-h") {
                std::cout
                    << "OrbitSimLite 3D experimental editor\nLaunch without arguments for the graphical "
                       "scene chooser.\n"
                       "  --scene INDEX|NAME  Choose a built-in scene (0 solar, 1 binary, 2 figure8, 3 "
                       "sandbox, 4 empty)\n"
                       "  --run               Start without the chooser\n"
                       "  --headless          Run without a graphics context\n"
                       "  --steps N           Exact numerical steps (headless only)\n"
                       "  --duration SECONDS  Wall-clock duration (headless only; INF means continuous)\n"
                       "  --speed NUMBER      Playback multiplier; leaves numerical dt unchanged\n"
                       "  --json | --no-json  Enable/disable JSON snapshots\n"
                       "  --csv               Enable CSV snapshots\n"
                       "  --data-dir PATH     Override the user data folder\n";
                return 0;
            } else if (flag == "--headless")
                headless = true;
            else if (flag == "--run")
                startup = false;
            else if (flag == "--json")
                outputs.enable_json = true;
            else if (flag == "--no-json")
                outputs.enable_json = false;
            else if (flag == "--csv")
                outputs.enable_csv = true;
            else if (flag == "--speed") {
                speed = number(value());
                if (speed <= 0 || speed > 1024)
                    throw std::runtime_error("Speed must be greater than zero and at most 1024.");
            } else if (flag == "--duration") {
                const auto s = value();
                if (s != "INF") {
                    duration = number(s);
                    if (*duration < 0)
                        throw std::runtime_error("Duration must be nonnegative.");
                }
            } else if (flag == "--steps") {
                const auto s = value();
                std::size_t end = 0;
                if (s.empty() || s[0] == '-')
                    throw std::runtime_error("Steps must be a nonnegative integer.");
                steps = std::stoull(s, &end);
                if (end != s.size())
                    throw std::runtime_error("Steps must be a nonnegative integer.");
            } else if (flag == "--scene") {
                const auto s = value();
                bool found = false;
                for (std::size_t j = 0; j < scenes.size(); ++j)
                    if (s == std::to_string(j) || s == scenes[j].name) {
                        default_scene = int(j);
                        found = true;
                        break;
                    }
                if (!found)
                    throw std::runtime_error("Unknown scene: " + s);
            } else if (flag == "--data-dir") {
                const auto path = value();
#ifdef _WIN32
                _putenv_s("ORBITSIMLITE_DATA_DIR", path.c_str());
#else
                setenv("ORBITSIMLITE_DATA_DIR", path.c_str(), 1);
#endif
            } else
                throw std::runtime_error("Unknown option: " + flag + " (use --help)");
        }
        if (!headless && (steps || duration))
            throw std::runtime_error("Use --headless with --steps or --duration.");
        // Assemble the same core services as the GUI without constructing any window or UI object.
        if (headless) {
            SceneDocument document;
            document.apply(scenes.at(std::size_t(default_scene)));
            SimulationController controller;
            controller.configure(document);
            controller.speed = speed;
            controller.collisions = CollisionMode::Ignore;
            SnapshotExporter exporter;
            exporter.options = outputs;
            return run_headless_session(document, controller, exporter, AppPaths::discover(), duration,
                                        steps);
        }
#ifdef ORBITSIMLITE_HAS_GUI
        Application app;
        app.set_presets(scenes);
        app.set_default_scene(default_scene);
        app.set_output_options(outputs);
        app.set_startup_screen(startup);
        app.set_initial_speed(speed);
        app.set_executable(AppPaths::executable(argv[0]));
        Simulator sim;
        app.run(sim);
        return 0;
#else
        (void)startup;
        throw std::runtime_error(
            "This build has no GUI. Use --headless or rebuild with ORBITSIMLITE_BUILD_GUI=ON.");
#endif
    } catch (const std::exception &e) {
        std::cerr << "OrbitSimLite: " << e.what() << '\n';
        return 1;
    }
}
} // namespace orbitsimlite
