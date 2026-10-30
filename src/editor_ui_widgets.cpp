// Apsilume UI: Build reusable buttons and inspector fields; keep active input and its caret visible.
// Public interface: include/editor_ui.hpp. Ownership and reading order: docs/architecture.md.
#include "editor_ui.hpp"
#include "editor_ui_detail.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>
namespace orbitsimlite {
using ui_detail::box;
using ui_detail::encoded;
using ui_detail::footer_height;
using ui_detail::panel_top;
using ui_detail::position;
using ui_detail::rectangle;
using ui_detail::single_line;
using ui_detail::size;

void EditorUI::input_value(sf::RenderWindow &window, const std::string &value, sf::FloatRect bounds,
                           bool active, unsigned pixels) const {
    const auto p = position(bounds), s = size(bounds);
    const auto inside = rectangle(p.x + 8, p.y + 4, std::max(0.0f, s.x - 16), std::max(0.0f, s.y - 8));
    if (!active || !focus_) {
        label(window, value, inside, pixels, sf::Color(230, 236, 246));
        return;
    }
    if (focus_->selected)
        box(window, inside, sf::Color(34, 65, 99), sf::Color(34, 65, 99));
    const auto text = single_line(value);
    const auto characters = sf::String::fromUtf8(text.begin(), text.end());
    // Focus stores a UTF-8 byte caret; sf::String slicing below uses Unicode character indices.
    const auto end = value.begin() + std::min(focus_->caret, value.size());
    const auto caret = std::min(characters.getSize(), sf::String::fromUtf8(value.begin(), end).getSize());
    std::size_t first = 0, last = characters.getSize();
    // Scroll only the visible substring. The full editing buffer is preserved for commit().
    const bool scrolling = size(text_bounds(text, pixels)).x > size(inside).x;
    if (scrolling) {
        first = caret;
        last = caret;
        while (first > 0 && text_advance(encoded(characters.substring(first - 1, caret - first + 1)),
                                         pixels) <= size(inside).x - 3)
            --first;
        while (last < characters.getSize() &&
               text_advance(encoded(characters.substring(first, last - first + 1)), pixels) <=
                   size(inside).x - 3)
            ++last;
    }
    const auto shown = encoded(characters.substring(first, last - first));
    const auto origin = text_origin(shown, inside, pixels, !scrolling);
    draw_text(window, shown, origin, pixels, sf::Color(230, 236, 246));
    if (!focus_->selected) {
        const float offset = text_advance(encoded(characters.substring(first, caret - first)), pixels);
        const float caret_x = std::clamp(origin.x + offset, p.x + 7, p.x + s.x - 7);
        box(window, rectangle(caret_x, p.y + 5, 1, std::max(1.0f, s.y - 10)), sf::Color(180, 218, 252),
            sf::Color(180, 218, 252));
    }
}

void EditorUI::button(sf::RenderWindow &window, std::string id, std::string label, sf::FloatRect rect,
                      Command command, bool enabled, bool active, std::string hint) {
    const auto &info = ActionRegistry::info(command.action);
    if (hint.empty()) {
        hint = info.hint;
        if (*info.shortcut)
            hint += " [" + std::string(info.shortcut) + "]";
    }
    const bool hover = rect.contains(pointer_);
    box(window, rect,
        !enabled
            ? sf::Color(29, 35, 45)
            : (active ? sf::Color(40, 81, 112) : (hover ? sf::Color(48, 66, 88) : sf::Color(30, 42, 59))),
        active ? sf::Color(90, 176, 236) : sf::Color(52, 72, 94));
    const auto p = position(rect), s = size(rect);
    this->label(window, label, rectangle(p.x + 8, p.y + 4, s.x - 16, s.y - 8), 13,
                enabled ? sf::Color(223, 232, 244) : sf::Color(110, 122, 139));
    if (hint.empty())
        hint = label;
    widgets_.push_back({id, label, hint, rect, command, {}, false, enabled, active, panel_building_});
    bounds_[id] = rect;
}

void EditorUI::field(sf::RenderWindow &window, const std::string &label, Property property,
                     const std::string &value, sf::FloatRect rect) {
    const auto field_position = position(rect), field_size = size(rect);
    if (panel_building_ && (field_position.y - 19 < panel_top ||
                            field_position.y + field_size.y > window_height_ - footer_height))
        return;
    const bool active = focus_ && focus_->property == property;
    const std::string visible = active ? focus_->value : value;
    const auto p = position(rect), s = size(rect);
    this->label(window, label, rectangle(p.x, p.y - 19, s.x, 16), 11, sf::Color(155, 177, 203));
    box(window, rect, sf::Color(12, 20, 32), active ? sf::Color(92, 184, 242) : sf::Color(54, 71, 93));
    input_value(window, visible, rect, active);
    const std::string id = "property_" + std::to_string(int(property));
    widgets_.push_back({id,
                        label,
                        label + ": " + value + " | Enter applies; Esc cancels. Ctrl+A selects all.",
                        rect,
                        {},
                        property,
                        false,
                        true,
                        active,
                        true});
    bounds_[id] = rect;
}
} // namespace orbitsimlite
