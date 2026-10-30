// Apsilume UI: Compose the UI frame: sidebar, wrapping toolbar, status bar and modal overlay.
// Public interface: include/editor_ui.hpp. Ownership and reading order: docs/architecture.md.
#include "editor_ui.hpp"
#include "editor_ui_detail.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>
namespace orbitsimlite {
using ui_detail::box;
using ui_detail::footer_height;
using ui_detail::number;
using ui_detail::panel_top;
using ui_detail::position;
using ui_detail::rectangle;
using ui_detail::size;

void EditorUI::draw(sf::RenderWindow &window, const UiModel &model) {
    widgets_.clear();
    bounds_.clear();
    window_height_ = float(model.world.height);
    panel(window, model);
    const float canvas = float(model.world.canvas_width());
    struct Tool {
        const char *id;
        const char *text;
        Action action;
    };
    const Tool tools[] = {{"play", model.playback.paused ? "Play" : "Pause", Action::TogglePlay},
                          {"step", "Step", Action::Step},
                          {"reset", "Reset", Action::Reset},
                          {"fit", "Fit", Action::Fit},
                          {"prediction", "Predict", Action::Prediction},
                          {"save", "Save", Action::Save},
                          {"load", "Open", Action::Load},
                          {"scenes", "Scenes", Action::Scenes},
                          {"help", "Help", Action::Help}};
    float x = 8, y = 8;
    std::vector<sf::FloatRect> rectangles;
    for (const auto &tool : tools) {
        const float width = std::max(54.0f, std::ceil(size(text_bounds(tool.text, 13)).x) + 20);
        if (x + width > canvas - 8 && x > 8) {
            x = 8;
            y += 36;
        }
        rectangles.push_back(rectangle(x, y, width, 28));
        x += width + 6;
    }
    const float toolbar_height = y + 36;
    box(window, rectangle(0, 0, canvas, toolbar_height), sf::Color(15, 24, 38, 245));
    for (std::size_t index = 0; index < rectangles.size(); ++index) {
        const auto &tool = tools[index];
        const bool enabled = !ActionRegistry::info(tool.action).needs_body || model.document.selected_body();
        button(window, tool.id, tool.text, rectangles[index], {tool.action}, enabled,
               tool.action == Action::Prediction && model.world.show_prediction);
    }
    const float speed_y = toolbar_height + 8;
    button(window, "slower", "−", rectangle(10, speed_y, 28, 28), {Action::Slower});
    label(window, "x" + number(model.playback.speed), rectangle(44, speed_y, 65, 28), 13,
          sf::Color(213, 228, 247));
    button(window, "faster", "+", rectangle(115, speed_y, 28, 28), {Action::Faster});
    canvas_top_ = speed_y + 40;
    const std::string message = error_.empty() ? model.status : error_;
    box(window,
        rectangle(0, float(model.world.height) - footer_height, float(model.world.width), footer_height),
        sf::Color(15, 24, 37));
    std::string hint = message.empty()
                           ? "Select a body | Right drag orbit | Middle drag pan | Scroll over the sidebar"
                           : message;
    for (const auto &widget : widgets_)
        if (widget.bounds.contains(pointer_) && !widget.hint.empty() &&
            (!widget.panel || pointer_.y >= panel_top))
            hint = widget.hint;
    label(window, hint,
          rectangle(10, float(model.world.height) - footer_height + 4, float(model.world.width) - 20,
                    footer_height - 8),
          11, sf::Color(177, 200, 228));
    // Keep a small orientation indicator independent of the origin arrows toggle.
    const sf::Vector2f origin{46, float(model.world.height) - 72};
    const Vec3 axes[] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    const sf::Color colors[] = {sf::Color(241, 117, 117), sf::Color(133, 223, 155), sf::Color(121, 175, 255)};
    for (int i = 0; i < 3; ++i) {
        const sf::Vector2f end{origin.x + float(Vec3::dot(axes[i], model.world.camera.right()) * 28),
                               origin.y - float(Vec3::dot(axes[i], model.world.camera.up()) * 28)};
        sf::Vertex vertices[2];
        vertices[0].position = origin;
        vertices[1].position = end;
        vertices[0].color = vertices[1].color = colors[i];
#if SFML_VERSION_MAJOR >= 3
        window.draw(vertices, 2, sf::PrimitiveType::Lines);
#else
        window.draw(vertices, 2, sf::Lines);
#endif
        draw_text(window, std::string(1, "XYZ"[i]), end, 11, colors[i]);
    }
    if (model.playback.prompt && dialog == UiDialog::None)
        button(window, "collision_confirm", "Collision: remove smaller [C]",
               rectangle(12, speed_y + 40, std::min(300.0f, canvas - 24), 30), {Action::ConfirmCollision},
               true, true);
    overlay(window, model);
}

std::map<std::string, sf::FloatRect> EditorUI::widget_bounds() const {
    std::map<std::string, sf::FloatRect> visible;
    for (const auto &widget : widgets_) {
        const auto p = position(widget.bounds), s = size(widget.bounds);
        if (!widget.panel || (p.y >= panel_top && p.y + s.y <= window_height_ - footer_height))
            visible[widget.id] = widget.bounds;
    }
    return visible;
}
} // namespace orbitsimlite
