// Apsilume: Resolve user-data locations and the running executable across supported platforms.
// The legacy OrbitSimLite directory name preserves access to existing scenes after rebranding.
#pragma once
#include <filesystem>
#include <string>
namespace orbitsimlite {
struct AppPaths {
    std::filesystem::path root, scenes, exports;
    // ORBITSIMLITE_DATA_DIR overrides platform defaults; return absolute root/scenes/exports paths.
    static AppPaths discover();
    // Resolve the launched binary before locating bundled assets, including launches through PATH.
    static std::filesystem::path executable(const std::string &argument);
    // Create data folders; report a user-facing filesystem error instead of silently losing output.
    bool ensure(std::string &error) const;
};
} // namespace orbitsimlite
