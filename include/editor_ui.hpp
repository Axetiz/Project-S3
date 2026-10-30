// Apsilume: UI state and widget interface; physics and persistence remain behind controllers.
// The six editor_ui*.cpp files separate composition, text, widgets, input, sidebar and dialogs.
#pragma once
#include "actions.hpp"
#include "app_paths.hpp"
#include "editor_controller.hpp"
#include "renderer3d.hpp"
#include "simulation_controller.hpp"
#include <array>
#include <functional>
#include <map>
namespace orbitsimlite {
enum class UiDialog { None, Startup, Save, Load, ConfirmOverwrite, ConfirmDelete, ConfirmDiscard, Help };
// Read-only borrowed references; rebuilding this view does not copy or advance a simulation.
struct UiModel {
    const SceneDocument &document;
    const SimulationController &playback;
    const Renderer3D &world;
    const TrajectoryPrediction &prediction;
    const AppPaths &paths;
    const std::vector<ScenarioPreset> &scenes;
    const std::vector<std::filesystem::path> &saved_files;
    const OutputOptions &outputs;
    const std::string &status;
    int startup_scene{0}, selected_saved{0};
    double forecast_range{1.5};
    EditorPlane plane{EditorPlane::XY};
};
// Reusable widgets own focus, layout, tooltips and dialogs, never simulation logic.
class EditorUI {
  public:
    UiDialog dialog{UiDialog::Startup};
    std::string scene_name{"My scene"};
    bool load_font(const std::filesystem::path &executable);
    void draw(sf::RenderWindow &window, const UiModel &model);
    bool click(sf::Vector2f point, SceneDocument &document, EditorController &editor,
               const std::function<void(Command)> &dispatch);
    bool wheel(sf::Vector2f point, float delta, const Renderer3D &world);
    bool key(sf::Keyboard::Key key, bool ctrl, bool shift, SceneDocument &document, EditorController &editor,
             const std::function<void(Command)> &dispatch);
    bool text(std::uint32_t unicode);
    void pointer(sf::Vector2f point) { pointer_ = point; }
    bool editing() const { return focus_.has_value(); }
    // Editing/dialogs freeze solver advancement while leaving the window responsive.
    bool blocks_simulation() const { return dialog != UiDialog::None || editing(); }
    float canvas_top() const { return canvas_top_; }
    void cancel_edit();
    void begin_save(const std::string &name);
    void focus_property(Property property, const SceneDocument &document);
    const std::string &error() const { return error_; }
    std::map<std::string, sf::FloatRect> widget_bounds() const;

  private:
    // Text remains a UTF-8 buffer until commit(); caret offsets are bytes at character boundaries.
    struct Focus {
        std::optional<Property> property;
        std::optional<BodyId> body;
        std::string value;
        std::size_t caret{0};
        bool selected{true};
    };
    // A frame-local hit target. Rebuilt from the same rectangles used to draw controls.
    struct Widget {
        std::string id, label, hint;
        sf::FloatRect bounds;
        Command command;
        std::optional<Property> property;
        bool name_field{false}, enabled{true}, active{false}, panel{false};
    };
    // Font and interaction state survive frames; widgets_ and bounds_ are rebuilt by draw().
    sf::Font font_;
    bool font_ready_{false};
    std::vector<Widget> widgets_;
    std::optional<Focus> focus_;
    std::string error_;
    sf::Vector2f pointer_{};
    float scroll_{0}, content_height_{0};
    bool panel_building_{false};
    float window_height_{840};
    float canvas_top_{88}, maximum_scroll_{0};
    int help_page_{0};
    std::array<bool, 4> sections_{{true, true, false, true}};
    std::size_t body_scroll_{0};
    std::map<std::string, sf::FloatRect> bounds_;
    // Text primitives live in editor_ui_text.cpp. Measurements and drawing share glyph metrics.
    void draw_text(sf::RenderWindow &window, const std::string &value, sf::Vector2f position, unsigned size,
                   sf::Color color) const;
    sf::FloatRect text_bounds(const std::string &value, unsigned pixels) const;
    float text_advance(const std::string &value, unsigned pixels) const;
    std::string fit_text(const std::string &value, float width, unsigned pixels) const;
    sf::Vector2f text_origin(const std::string &value, sf::FloatRect bounds, unsigned pixels,
                             bool centered = true) const;
    void label(sf::RenderWindow &window, const std::string &value, sf::FloatRect bounds, unsigned pixels,
               sf::Color color, bool centered = true, unsigned minimum = 11) const;
    float paragraph(sf::RenderWindow &window, const std::string &value, sf::FloatRect bounds, unsigned pixels,
                    sf::Color color) const;
    // Reusable field/button drawing lives in editor_ui_widgets.cpp.
    void input_value(sf::RenderWindow &window, const std::string &value, sf::FloatRect bounds, bool active,
                     unsigned pixels = 13) const;
    // Focus/validation/hit routing lives in editor_ui_input.cpp.
    bool commit(SceneDocument &document, EditorController &editor);
    void button(sf::RenderWindow &window, std::string id, std::string label, sf::FloatRect rect,
                Command command, bool enabled = true, bool active = false, std::string hint = {});
    void field(sf::RenderWindow &window, const std::string &label, Property property,
               const std::string &value, sf::FloatRect rect);
    // Persistent sidebar layout and modal layout have separate implementation files.
    void panel(sf::RenderWindow &window, const UiModel &model);
    void overlay(sf::RenderWindow &window, const UiModel &model);
};
} // namespace orbitsimlite
