// Private UI utilities: rectangle compatibility, primitive boxes and display-only formatting.
// Shared by the UI implementation files; this is not part of the public library API.
#pragma once
#include <SFML/Graphics.hpp>
#include <iomanip>
#include <sstream>
#include <string>
namespace orbitsimlite::ui_detail {
inline sf::FloatRect rectangle(float x, float y, float width, float height) {
#if SFML_VERSION_MAJOR >= 3
    return {{x, y}, {width, height}};
#else
    return {x, y, width, height};
#endif
}

inline sf::Vector2f position(const sf::FloatRect &r) {
#if SFML_VERSION_MAJOR >= 3
    return r.position;
#else
    return {r.left, r.top};
#endif
}

inline sf::Vector2f size(const sf::FloatRect &r) {
#if SFML_VERSION_MAJOR >= 3
    return r.size;
#else
    return {r.width, r.height};
#endif
}

inline std::string number(double value, int precision = 6) {
    std::ostringstream text;
    text << std::setprecision(precision) << value;
    return text.str();
}

inline void box(sf::RenderWindow &window, sf::FloatRect bounds, sf::Color fill,
                sf::Color border = sf::Color(49, 65, 87)) {
    sf::RectangleShape shape(size(bounds));
    shape.setPosition(position(bounds));
    shape.setFillColor(fill);
    shape.setOutlineColor(border);
    shape.setOutlineThickness(1);
    window.draw(shape);
}

inline std::string single_line(std::string value) {
    for (auto &character : value)
        if (character == '\n' || character == '\r' || character == '\t')
            character = ' ';
    return value;
}

inline std::string encoded(const sf::String &value) {
    const auto bytes = value.toUtf8();
    return {bytes.begin(), bytes.end()};
}

// These screen-space boundaries must agree in drawing, clipping and hit testing.
inline constexpr float panel_top = 166;
inline constexpr float footer_height = 28;

// Action::None reserves these arguments for navigation consumed by EditorUI itself.
// Keeping them out of Application::dispatch avoids mixing UI pagination with scene commands.
inline constexpr int previous_body_page = -10;
inline constexpr int next_body_page = -11;
inline constexpr int previous_help_page = -12;
inline constexpr int next_help_page = -13;
} // namespace orbitsimlite::ui_detail
