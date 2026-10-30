// Apsilume: Versioned reusable scene files and atomic whole-file replacement.
// Parsing/validation are independent of windows; live JSON/CSV snapshots use SnapshotExporter.
#pragma once
#include "scene.hpp"
#include <filesystem>
namespace orbitsimlite {
struct FileResult {
    bool ok{false};
    std::string message;
    explicit operator bool() const { return ok; }
};
class SceneSerializer {
  public:
    static constexpr int schema_version = 2;
    static FileResult encode(const ScenarioPreset &scene, std::string &output);
    // Transactional: replace scene only after the entire candidate parses and validates.
    static FileResult decode(const std::string &text, ScenarioPreset &scene);
    static FileResult save(const std::filesystem::path &path, const ScenarioPreset &scene);
    static FileResult load(const std::filesystem::path &path, ScenarioPreset &scene);
    // Stage beside the destination, then replace it. Failure preserves the previous destination.
    // This prevents partial-file reads; it does not promise durability after power loss.
    static FileResult atomic_write(const std::filesystem::path &path, const std::string &text);
    // Sanitize a user label into a filename; callers still choose its parent directory.
    static std::string filename(const std::string &label);
};
} // namespace orbitsimlite
