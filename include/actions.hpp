// Apsilume: Shared command vocabulary for keyboard bindings and clickable controls.
// ActionRegistry supplies shortcut metadata; Application::dispatch performs the requested work.
#pragma once
#include "body.hpp"
#include <SFML/Window/Keyboard.hpp>
#include <vector>
namespace orbitsimlite {
enum class Action {
    None,
    TogglePlay,
    Step,
    Reset,
    Fit,
    Prediction,
    Grid,
    Origin,
    Vectors,
    Gizmo,
    Field,
    Top,
    Front,
    Side,
    Oblique,
    Projection,
    Focus,
    NextBody,
    AddBody,
    DeleteBody,
    Save,
    Load,
    Scenes,
    StartScene,
    SelectScene,
    SelectBody,
    Visibility,
    Slower,
    Faster,
    ShorterForecast,
    LongerForecast,
    Integrator,
    CollisionPolicy,
    ConfirmCollision,
    Plane,
    PlaneBack,
    PlaneForward,
    MassDown,
    MassUp,
    RadiusDown,
    RadiusUp,
    Star,
    Satellite,
    MoveLeft,
    MoveRight,
    MoveUp,
    MoveDown,
    MoveBack,
    MoveForward,
    VelocityLeft,
    VelocityRight,
    VelocityUp,
    VelocityDown,
    VelocityBack,
    VelocityForward,
    ToggleJson,
    ToggleCsv,
    SelectSaved,
    LoadSaved,
    DeleteSaved,
    ConfirmDelete,
    SaveNamed,
    ConfirmOverwrite,
    Discard,
    Cancel,
    Help,
    Quit
};
struct Command {
    Action action{Action::None};
    int argument{0};
    BodyId body{0};
    bool fine{false};
};
struct ActionInfo {
    Action action;
    const char *label;
    const char *shortcut;
    const char *hint;
    sf::Keyboard::Key key;
    bool needs_body{false};
};
class ActionRegistry {
  public:
    static const std::vector<ActionInfo> &all();
    static const ActionInfo &info(Action action);
    static Action from_key(sf::Keyboard::Key key);
};
} // namespace orbitsimlite
