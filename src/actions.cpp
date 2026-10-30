// Apsilume: One binding/metadata table serves buttons, tooltips and keyboard lookup. Execution stays in
// Application::dispatch. Module relationships and invariants: docs/architecture.md.
#include "actions.hpp"
namespace orbitsimlite {
const std::vector<ActionInfo> &ActionRegistry::all() {
    using K = sf::Keyboard::Key;
    static const std::vector<ActionInfo> actions = {
        {Action::None, "", "", "", K::Unknown},
        {Action::TogglePlay, "Play / Pause", "Space",
         "Pause before editing; playback speed does not change numerical accuracy.", K::Space},
        {Action::Step, "Step", ".", "Advance one fixed numerical timestep.", K::Period},
        {Action::Reset, "Reset", "R", "Restore the initial state of this scene.", K::R},
        {Action::Fit, "Fit view", "Home", "Frame all visible bodies.", K::Home},
        {Action::Prediction, "Prediction", "P", "Forecast the selected body's future path.", K::P, true},
        {Action::Grid, "Grid", "H", "Show or hide both flat and deformed grids.", K::H},
        {Action::Origin, "Origin axes", "G", "Show XYZ arrows at world position (0,0,0).", K::G},
        {Action::Vectors, "Body vectors", "V", "Show velocity and acceleration directions.", K::V},
        {Action::Gizmo, "Move handles", "", "Drag a colored axis handle to move the selected body.",
         K::Unknown},
        {Action::Field, "Field mode", "F", "Cycle no field, acceleration arrows and deformed grid.", K::F},
        {Action::Top, "Top XY", "F2", "View the XY plane from above.", K::F2},
        {Action::Front, "Front XZ", "F3", "View and edit the XZ plane.", K::F3},
        {Action::Side, "Side YZ", "F4", "View and edit the YZ plane.", K::F4},
        {Action::Oblique, "3D view", "F1", "Restore the oblique 3D camera angle.", K::F1},
        {Action::Projection, "Projection", "O", "Switch perspective or orthographic projection.", K::O},
        {Action::Focus, "Focus / follow", "F5", "Center on the selected body and toggle camera following.",
         K::F5, true},
        {Action::NextBody, "Next body", "Tab", "Select the next body in the scene.", K::Tab},
        {Action::AddBody, "Add body", "N",
         "Create a body on the editing plane; the toolbar uses the camera target when needed.", K::N},
        {Action::DeleteBody, "Delete body", "Delete", "Remove the selected body.", K::Delete, true},
        {Action::Save, "Save scene", "K", "Save the current scene with a name.", K::K},
        {Action::Load, "Open scene", "L", "Open or manage scenes from your application data folder.", K::L},
        {Action::Scenes, "Scenes", "", "Choose a built-in scene and output formats.", K::Unknown},
        {Action::Slower, "Slower", "-", "Reduce playback speed; keep the numerical timestep unchanged.",
         K::Hyphen},
        {Action::Faster, "Faster", "+", "Increase playback speed; keep the numerical timestep unchanged.",
         K::Equal},
        {Action::ShorterForecast, "Shorter", "F6", "Halve the forecast horizon.", K::F6},
        {Action::LongerForecast, "Longer", "F7", "Double the forecast horizon.", K::F7},
        {Action::Integrator, "Integrator", "I", "Switch between coupled RK4 and symplectic Euler.", K::I},
        {Action::CollisionPolicy, "Collisions", "M", "Choose how overlapping bodies are handled.", K::M},
        {Action::ConfirmCollision, "Remove smaller", "C",
         "Confirm the pending collision without changing unrelated bodies.", K::C},
        {Action::Plane, "Editing plane", "B", "Cycle XY, XZ and YZ creation planes.", K::B},
        {Action::PlaneBack, "Plane backward", "[", "Move the editing plane along its normal.", K::LBracket},
        {Action::PlaneForward, "Plane forward", "]", "Move the editing plane along its normal.", K::RBracket},
        {Action::MassDown, "Decrease mass", "Q", "Decrease selected mass.", K::Q, true},
        {Action::MassUp, "Increase mass", "E", "Increase selected mass.", K::E, true},
        {Action::RadiusDown, "Smaller display", "Z", "Decrease display radius; this is not physical size.",
         K::Z, true},
        {Action::RadiusUp, "Larger display", "X", "Increase display radius.", K::X, true},
        {Action::Star, "Star", "T", "Toggle the star role.", K::T, true},
        {Action::Satellite, "Satellite", "Y", "Toggle the satellite role.", K::Y, true},
        {Action::MoveLeft, "Move -X", "Left", "Move along world X. Hold Shift for fine movement.", K::Left,
         true},
        {Action::MoveRight, "Move +X", "Right", "Move along world X.", K::Right, true},
        {Action::MoveUp, "Move +Y", "Up", "Move along world Y.", K::Up, true},
        {Action::MoveDown, "Move -Y", "Down", "Move along world Y.", K::Down, true},
        {Action::MoveBack, "Move -Z", "PageDown", "Move below the XY plane.", K::PageDown, true},
        {Action::MoveForward, "Move +Z", "PageUp", "Move above the XY plane.", K::PageUp, true},
        {Action::VelocityLeft, "Velocity -X", "A", "Decrease X velocity.", K::A, true},
        {Action::VelocityRight, "Velocity +X", "D", "Increase X velocity.", K::D, true},
        {Action::VelocityUp, "Velocity +Y", "W", "Increase Y velocity.", K::W, true},
        {Action::VelocityDown, "Velocity -Y", "S", "Decrease Y velocity.", K::S, true},
        {Action::VelocityBack, "Velocity -Z", "J", "Decrease Z velocity.", K::J, true},
        {Action::VelocityForward, "Velocity +Z", "U", "Increase Z velocity.", K::U, true},
        {Action::Help, "Help", "F8", "Show camera, editing and keyboard guidance.", K::F8},
        {Action::Quit, "Exit", "Esc", "Close the app, or cancel the current dialog.", K::Escape}};
    return actions;
}
const ActionInfo &ActionRegistry::info(Action action) {
    for (const auto &info : all())
        if (info.action == action)
            return info;
    return all().front();
}
Action ActionRegistry::from_key(sf::Keyboard::Key key) {
    if (key == sf::Keyboard::Key::Add)
        return Action::Faster;
    if (key == sf::Keyboard::Key::Subtract)
        return Action::Slower;
#if SFML_VERSION_MAJOR >= 3
    if (key == sf::Keyboard::Key::Backspace)
        return Action::DeleteBody;
#else
    if (key == sf::Keyboard::BackSpace)
        return Action::DeleteBody;
#endif
    for (const auto &info : all())
        if (info.key != sf::Keyboard::Key::Unknown && info.key == key)
            return info.action;
    return Action::None;
}
} // namespace orbitsimlite
