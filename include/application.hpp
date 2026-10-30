// Apsilume: Owns the running editor and coordinates services without implementing their algorithms.
// Read application.cpp for frame order; actions, scene transitions, events and diagnostics have separate
// files.
#pragma once
#include "app_paths.hpp"
#include "editor_ui.hpp"
#include "scene_serializer.hpp"
#include "snapshot_exporter.hpp"
namespace orbitsimlite {
// Coordinates services; physics, editing, persistence and drawing have separate owners.
class Application {
  public:
    Application(unsigned width = 1280, unsigned height = 840, double scale = 2e-9);
    void set_presets(const std::vector<ScenarioPreset> &scenes) { scenes_ = scenes; }
    void set_output_options(OutputOptions options) { exporter_.options = options; }
    void set_default_scene(int index) { startup_scene_ = index; }
    void set_startup_screen(bool enabled) { startup_screen_ = enabled; }
    void set_executable(std::filesystem::path path) { executable_ = std::move(path); }
    void set_initial_speed(double speed) { initial_speed_ = speed; }
    void run(Simulator &simulation);
    void run_headless(Simulator &simulation, std::optional<double> duration, const std::string &name);

  private:
    // Domain services own their state; the application coordinates their lifetime and call order.
    SceneDocument document_;
    SimulationController playback_;
    EditorController editor_;
    Renderer3D world_;
    EditorUI ui_;
    SnapshotExporter exporter_;
    TrajectoryPrediction prediction_;
    AppPaths paths_;
    // Launch configuration and chooser state. These do not belong to a saved physical scene.
    std::vector<ScenarioPreset> scenes_;
    std::vector<std::filesystem::path> saved_files_;
    std::filesystem::path executable_{"orbit_simulator"};
    std::string status_;
    int startup_scene_{0}, selected_saved_{0};
    bool startup_screen_{true}, started_{false}, quit_{false}, follow_{false};
    double initial_speed_{1}, forecast_range_{1.5};
    // The edit plane is a world-space cross-section used for creation and field visualization.
    EditorPlane plane_{EditorPlane::XY};
    double plane_offset_{0};
    Vec3 cursor_{};
    bool cursor_valid_{false};
    // A drag is a temporary gesture; Axis suppresses live stepping until the mouse is released.
    enum class Drag { None, Orbit, Pan, Axis };
    Drag drag_{Drag::None};
    std::optional<AxisHandle> dragged_axis_;
    sf::Vector2i last_mouse_{};
    std::optional<Command> pending_;     // Resume this replacement/exit action after Save or Discard.
    char ignore_text_{0};                // Suppress K's paired text event when K opens the save-name field.
    std::uint64_t observed_revision_{0}; // Last edit revision consumed by trails/prediction.
    // The view model borrows service references; use it only during the current UI operation.
    UiModel model() const;
    void apply_scene(const ScenarioPreset &scene);
    // Buttons and shortcuts converge here. Confirmation bypasses the unsaved-change guard once.
    void dispatch(Command command, bool discard_confirmed = false);
    void refresh_files();
    void save_scene(bool overwrite);
    void poll_events(sf::RenderWindow &window);
    void pointer(sf::Vector2i point);
    void mouse_press(int button, sf::Vector2i point);
    void mouse_release(int button);
    void wheel(float delta);
    void key(sf::Keyboard::Key code, bool ctrl, bool shift);
    void text(std::uint32_t unicode);
    void dump_ui_state() const;
};
} // namespace orbitsimlite
