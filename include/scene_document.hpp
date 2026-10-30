// Apsilume: Editable scene ownership, reset baseline, revision tracking and stable body selection.
// Presentation code borrows this state; all body lookup after container edits goes through BodyId.
#pragma once
#include "scene.hpp"
#include <optional>
namespace orbitsimlite {
// Owns editable simulation state and stable selection, independently of windows.
class SceneDocument {
  public:
    Simulator simulation;
    ScenarioPreset initial; // Reset returns to the last applied preset or loaded scene.
    std::string name{"Untitled"};
    std::string units{"SI"};
    double scale{2e-9};        // Reference pixels per world-length unit; camera zoom is separate.
    bool dirty{false};         // Explicit edits/collisions need saving; normal playback does not set this.
    std::uint64_t revision{0}; // Invalidate derived presentation caches after an edit/apply.
    std::optional<BodyId> selection;
    // Replace the simulation, reset time, establish the reset baseline and clear dirty state.
    void apply(const ScenarioPreset &scene);
    void mark_edited();
    void select_index(int index);
    int selected_index() const;
    // Borrowed pointers are valid only until the body vector is structurally modified.
    Body *selected_body();
    const Body *selected_body() const;
    // Copy the current physical state for saving; playback/view state is intentionally absent.
    ScenarioPreset snapshot() const;
};
} // namespace orbitsimlite
