// Apsilume: Built-in scene definitions used by both the GUI chooser and the command-line runner.
// Presets provide initial values; the integrator does not special-case these scenarios.
#pragma once
#include "scene.hpp"
namespace orbitsimlite {
std::vector<ScenarioPreset> make_builtin_scenes();
}
