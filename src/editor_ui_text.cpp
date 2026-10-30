// Apsilume UI: Load the font and measure/draw text using the same kerning and glyph bounds.
// Public interface: include/editor_ui.hpp. Ownership and reading order: docs/architecture.md.
#include "editor_ui.hpp"
#include "editor_ui_detail.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>
namespace orbitsimlite {
using ui_detail::encoded;
using ui_detail::position;
using ui_detail::rectangle;
using ui_detail::single_line;
using ui_detail::size;

bool EditorUI::load_font(const std::filesystem::path &executable) {
    const auto directory = std::filesystem::absolute(executable).parent_path();
    const std::vector<std::filesystem::path> candidates = {
        directory / "assets/DejaVuSans.ttf", directory / "../share/orbitsimlite/DejaVuSans.ttf",
        std::filesystem::path(ORBITSIMLITE_ASSET_DIR) / "DejaVuSans.ttf"};
    for (const auto &path : candidates) {
        std::error_code ec;
        if (!std::filesystem::is_regular_file(path, ec))
            continue;
#if SFML_VERSION_MAJOR >= 3
        if (font_.openFromFile(path)) {
#else
        if (font_.loadFromFile(path.string())) {
#endif
            font_ready_ = true;
            return true;
        }
    }
    return false;
}

void EditorUI::draw_text(sf::RenderWindow &window, const std::string &text, sf::Vector2f point,
                         unsigned pixels, sf::Color color) const {
    if (!font_ready_)
        return;
#if SFML_VERSION_MAJOR >= 3
    sf::VertexArray triangles(sf::PrimitiveType::Triangles);
#else
    sf::VertexArray triangles(sf::Triangles);
#endif
    float x = point.x, y = point.y + pixels;
    char32_t previous = 0;
    // Draw glyphs directly: avoids this workstation's sf::Text shaping teardown bug.
    for (auto ch : sf::String::fromUtf8(text.begin(), text.end())) {
        if (ch == '\n') {
            x = point.x;
            y += font_.getLineSpacing(pixels);
            previous = 0;
            continue;
        }
        x += font_.getKerning(previous, ch, pixels, false);
        const auto &glyph = font_.getGlyph(ch, pixels, false);
        const auto p = position(glyph.bounds), s = size(glyph.bounds);
        const float left = x + p.x, top = y + p.y, right = left + s.x, bottom = top + s.y;
#if SFML_VERSION_MAJOR >= 3
        const float u = float(glyph.textureRect.position.x), v = float(glyph.textureRect.position.y),
                    u1 = u + glyph.textureRect.size.x, v1 = v + glyph.textureRect.size.y;
#else
        const float u = float(glyph.textureRect.left), v = float(glyph.textureRect.top),
                    u1 = u + glyph.textureRect.width, v1 = v + glyph.textureRect.height;
#endif
        const sf::Vector2f points[] = {{left, top}, {right, top},    {right, bottom},
                                       {left, top}, {right, bottom}, {left, bottom}};
        const sf::Vector2f coordinates[] = {{u, v}, {u1, v}, {u1, v1}, {u, v}, {u1, v1}, {u, v1}};
        for (int i = 0; i < 6; ++i) {
            sf::Vertex vertex;
            vertex.position = points[i];
            vertex.texCoords = coordinates[i];
            vertex.color = color;
            triangles.append(vertex);
        }
        x += glyph.advance;
        previous = ch;
    }
    sf::RenderStates states;
    states.texture = &font_.getTexture(pixels);
    window.draw(triangles, states);
}

// Bounds use exactly the same glyph/kerning sequence as draw_text(); alignment depends on this agreement.
sf::FloatRect EditorUI::text_bounds(const std::string &value, unsigned pixels) const {
    float x = 0, baseline = float(pixels), left = 0, top = 0, right = 0, bottom = 0;
    bool first = true;
    char32_t previous = 0;
    for (auto character : sf::String::fromUtf8(value.begin(), value.end())) {
        if (character == '\n') {
            x = 0;
            baseline += font_.getLineSpacing(pixels);
            previous = 0;
            continue;
        }
        x += font_.getKerning(previous, character, pixels, false);
        const auto &glyph = font_.getGlyph(character, pixels, false);
        const auto p = position(glyph.bounds), s = size(glyph.bounds);
        if (s.x > 0 && s.y > 0) {
            if (first) {
                left = x + p.x;
                top = baseline + p.y;
                right = left + s.x;
                bottom = top + s.y;
                first = false;
            } else {
                left = std::min(left, x + p.x);
                top = std::min(top, baseline + p.y);
                right = std::max(right, x + p.x + s.x);
                bottom = std::max(bottom, baseline + p.y + s.y);
            }
        }
        x += glyph.advance;
        previous = character;
    }
    return rectangle(left, top, std::max(0.0f, right - left), std::max(0.0f, bottom - top));
}

float EditorUI::text_advance(const std::string &value, unsigned pixels) const {
    float result = 0;
    char32_t previous = 0;
    for (auto character : sf::String::fromUtf8(value.begin(), value.end())) {
        result += font_.getKerning(previous, character, pixels, false) +
                  font_.getGlyph(character, pixels, false).advance;
        previous = character;
    }
    return result;
}

// Truncate by Unicode characters after measuring pixels; never cut a UTF-8 byte sequence.
std::string EditorUI::fit_text(const std::string &value, float width, unsigned pixels) const {
    auto text = single_line(value);
    if (size(text_bounds(text, pixels)).x <= width)
        return text;
    const std::string ellipsis = "…";
    if (size(text_bounds(ellipsis, pixels)).x > width)
        return {};
    auto characters = sf::String::fromUtf8(text.begin(), text.end());
    while (!characters.isEmpty()) {
        characters.erase(characters.getSize() - 1, 1);
        text = encoded(characters) + ellipsis;
        if (size(text_bounds(text, pixels)).x <= width)
            return text;
    }
    return ellipsis;
}

sf::Vector2f EditorUI::text_origin(const std::string &value, sf::FloatRect bounds, unsigned pixels,
                                   bool centered) const {
    const auto ink = text_bounds(value, pixels);
    const auto p = position(bounds), s = size(bounds), bearing = position(ink), extent = size(ink);
    return {std::round(p.x + (centered ? (s.x - extent.x) * 0.5f : 0) - bearing.x),
            std::round(p.y + (s.y - extent.y) * 0.5f - bearing.y)};
}

void EditorUI::label(sf::RenderWindow &window, const std::string &value, sf::FloatRect bounds,
                     unsigned pixels, sf::Color color, bool centered, unsigned minimum) const {
    auto text = single_line(value);
    const auto s = size(bounds);
    if (s.x <= 0 || s.y <= 0 || !font_ready_)
        return;
    minimum = std::min(minimum, pixels);
    while (pixels > minimum &&
           (size(text_bounds(text, pixels)).x > s.x || size(text_bounds(text, pixels)).y > s.y))
        --pixels;
    text = fit_text(text, s.x, pixels);
    if (text.empty() || size(text_bounds(text, pixels)).y > s.y)
        return;
    draw_text(window, text, text_origin(text, bounds, pixels, centered), pixels, color);
}

float EditorUI::paragraph(sf::RenderWindow &window, const std::string &value, sf::FloatRect bounds,
                          unsigned pixels, sf::Color color) const {
    const auto p = position(bounds), s = size(bounds);
    const float line_height = std::ceil(font_.getLineSpacing(pixels)) + 3;
    const int maximum = std::max(0, int(s.y / line_height));
    if (!maximum)
        return 0;
    std::istringstream words(single_line(value));
    std::vector<std::string> lines;
    std::string word, line;
    while (words >> word) {
        const auto candidate = line.empty() ? word : line + " " + word;
        if (!line.empty() && size(text_bounds(candidate, pixels)).x > s.x) {
            lines.push_back(line);
            line = word;
        } else
            line = candidate;
    }
    if (!line.empty())
        lines.push_back(line);
    const int shown = std::min(maximum, int(lines.size()));
    for (int i = 0; i < shown; ++i) {
        auto text = lines[std::size_t(i)];
        if (i + 1 == shown && int(lines.size()) > shown)
            text += " …";
        label(window, text, rectangle(p.x, p.y + i * line_height, s.x, line_height), pixels, color);
    }
    return shown * line_height;
}
} // namespace orbitsimlite
