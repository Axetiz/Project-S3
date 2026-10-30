// Apsilume: Converts real elapsed time into fixed numerical steps and applies collision policy.
// The simulator owns dt; this controller owns playback rate, accumulated time and pending collision
// decisions.
#pragma once
#include "scene_document.hpp"
#include <functional>
namespace orbitsimlite {
// Playback changes the number of fixed steps, never the integration timestep.
class SimulationController {
  public:
    bool paused{false};
    double speed{1};     // Dimensionless multiplier, independent of numerical dt.
    double base_rate{1}; // Simulation-time units per real second at speed x1.
    CollisionMode collisions{CollisionMode::PromptRemoveSmaller};
    bool catching_up{false};
    struct CollisionPrompt {
        BodyId keep, remove;
    };
    std::optional<CollisionPrompt> prompt; // IDs remain meaningful after vector reordering.
    // Start at 60 outer steps/real second; subsequent dt edits leave base_rate unchanged.
    void configure(const SceneDocument &document);
    // Also clear queued time, even when value equals the current pause state.
    void set_paused(bool value);
    // Consume time within a small work budget, retaining backlog. after_step runs after each completed step.
    int advance(SceneDocument &document, double real_seconds, const std::function<void()> &after_step = {});
    // Exactly one outer dt, unless a collision decision is pending; the caller decides pause state.
    void single_step(SceneDocument &document);
    void confirm_collision(SceneDocument &document);
    void cancel_collision();

  private:
    double accumulated_{0}; // Simulation-time units still waiting to be integrated.
    void resolve_collisions(SceneDocument &document);
};
} // namespace orbitsimlite
