// Apsilume: Common entry point for the main executable and preset-specific demo launchers.
// Parses CLI options, chooses a preset, then selects the graphical or headless execution path.
#pragma once
namespace orbitsimlite {
int run_product_app(int argc, char **argv, int default_scene = 0);
}
