// Apsilume UI: Lay out the scrollable body inspector, simulation controls and camera sections.
// Public interface: include/editor_ui.hpp. Ownership and reading order: docs/architecture.md.
#include "editor_ui.hpp"
#include "editor_ui_detail.hpp"
#include <SFML/OpenGL.hpp>
#include <algorithm>
#include <cmath>
#include <sstream>
namespace orbitsimlite {
using ui_detail::box;
using ui_detail::footer_height;
using ui_detail::number;
using ui_detail::panel_top;
using ui_detail::rectangle;

void EditorUI::panel(sf::RenderWindow &window, const UiModel &model) {
    panel_building_ = true;
    const float left = float(model.world.canvas_width()), width = model.world.panel_width;
    box(window, rectangle(left, 0, width, float(model.world.height)), sf::Color(17, 25, 39));
    label(window, "Apsilume 3D", rectangle(left + 14, 10, width - 28, 25), 21, sf::Color(235, 242, 252));
    label(window, model.document.name + (model.document.dirty ? " *" : ""),
          rectangle(left + 14, 39, width - 28, 18), 13, sf::Color(137, 194, 242));
    label(window,
          std::string(model.playback.paused ? "PAUSED" : "PLAYING") + "  |  t " +
              number(model.document.simulation.get_time()) + " " +
              (model.document.units == "SI" ? "s" : "time units"),
          rectangle(left + 14, 61, width - 28, 17), 12, sf::Color(194, 211, 230));
    label(window,
          "Speed x" + number(model.playback.speed) + "  |  " +
              number(model.playback.base_rate * model.playback.speed) + " sim / real s",
          rectangle(left + 14, 83, width - 28, 16), 11, sf::Color(144, 164, 188));
    float y = panel_top - scroll_;
    const float x = left + 13, w = width - 26;
    const float bottom = float(model.world.height) - footer_height;
    // Clip panel content between its fixed header and the status footer, using OpenGL bottom-left Y.
    glEnable(GL_SCISSOR_TEST);
    glScissor(int(left), int(footer_height), int(width), std::max(1, int(bottom - panel_top)));
    const auto section = [&](int index, const std::string &title) {
        button(window, "section_" + std::to_string(index), title, rectangle(x, y, w, 27),
               {Action::None, index}, true, false,
               sections_[std::size_t(index)] ? "Collapse this section" : "Expand this section");
        label(window, sections_[std::size_t(index)] ? "−" : "+", rectangle(x + 6, y + 4, 16, 19), 13,
              sf::Color(155, 177, 203));
        y += 37;
        return sections_[std::size_t(index)];
    };
    const auto control = [&](const std::string &id, const std::string &label, Action action,
                             bool active = false, bool enabled = true) {
        button(window, "panel_" + id, label, rectangle(x, y, w, 27), {action}, enabled, active);
        y += 35;
    };
    if (section(0, "Bodies (" + std::to_string(model.document.simulation.get_bodies().size()) + ")")) {
        const auto &bodies = model.document.simulation.get_bodies();
        const std::size_t shown = std::min<std::size_t>(5, bodies.size());
        if (body_scroll_ > bodies.size() - shown)
            body_scroll_ = bodies.size() - shown;
        for (std::size_t row = 0; row < shown; ++row) {
            const auto &b = bodies[body_scroll_ + row];
            button(window, "panel_body_" + std::to_string(b.id), b.name, rectangle(x, y, w - 62, 27),
                   {Action::SelectBody, 0, b.id}, true, model.document.selection == b.id,
                   "Select " + b.name + ". Use Focus to center the camera.");
            button(window, "panel_visible_" + std::to_string(b.id), b.visible ? "Hide" : "Show",
                   rectangle(x + w - 58, y, 58, 27), {Action::Visibility, 0, b.id}, true, b.visible,
                   "Visibility affects drawing, not gravity.");
            y += 32;
        }
        if (bodies.size() > shown) {
            button(window, "panel_body_prev", "Previous", rectangle(x, y, w / 2 - 3, 25),
                   {Action::None, ui_detail::previous_body_page});
            button(window, "panel_body_next", "Next", rectangle(x + w / 2 + 3, y, w / 2 - 3, 25),
                   {Action::None, ui_detail::next_body_page});
            y += 31;
        }
        button(window, "panel_add", "Add body", rectangle(x, y, w / 2 - 3, 27), {Action::AddBody, 1});
        button(window, "panel_remove", "Delete", rectangle(x + w / 2 + 3, y, w / 2 - 3, 27),
               {Action::DeleteBody}, model.document.selected_body() != nullptr);
        y += 37;
    }
    if (section(1, "Selected body")) {
        const auto *b = model.document.selected_body();
        if (!b) {
            label(window, "Select a body in the list or scene.", rectangle(x, y, w, 24), 12,
                  sf::Color(173, 190, 213));
            y += 30;
        } else {
            field(window, "Name", Property::Name, b->name, rectangle(x, y + 17, w, 26));
            y += 52;
            field(window, model.document.units == "SI" ? "Mass (kg)" : "Mass (units)", Property::Mass,
                  number(b->mass), rectangle(x, y + 17, w, 26));
            y += 52;
            field(window, "Display radius (reference pixels)", Property::Radius, number(b->radius),
                  rectangle(x, y + 17, w, 26));
            y += 52;
            const char *axes[] = {"X", "Y", "Z"};
            const double positions[] = {b->pos.x, b->pos.y, b->pos.z},
                         velocities[] = {b->vel.x, b->vel.y, b->vel.z};
            const float cell = (w - 12) / 3;
            label(window, model.document.units == "SI" ? "Position (m)" : "Position (world units)",
                  rectangle(x, y, w, 20), 12, sf::Color(167, 189, 217));
            y += 25;
            for (int i = 0; i < 3; ++i)
                field(window, axes[i], Property(int(Property::PositionX) + i), number(positions[i]),
                      rectangle(x + i * (cell + 6), y + 17, cell, 26));
            y += 53;
            label(window, model.document.units == "SI" ? "Velocity (m/s)" : "Velocity (units / time)",
                  rectangle(x, y, w, 20), 12, sf::Color(167, 189, 217));
            y += 25;
            for (int i = 0; i < 3; ++i)
                field(window, axes[i], Property(int(Property::VelocityX) + i), number(velocities[i]),
                      rectangle(x + i * (cell + 6), y + 17, cell, 26));
            y += 53;
            button(window, "panel_focus", "Focus / follow", rectangle(x, y, w / 2 - 3, 27), {Action::Focus},
                   true);
            button(window, "panel_gizmo", "Move handles", rectangle(x + w / 2 + 3, y, w / 2 - 3, 27),
                   {Action::Gizmo}, true, model.world.show_gizmo);
            y += 37;
            button(window, "panel_star", "Star", rectangle(x, y, w / 2 - 3, 27), {Action::Star}, true,
                   b->is_star);
            button(window, "panel_satellite", "Satellite", rectangle(x + w / 2 + 3, y, w / 2 - 3, 27),
                   {Action::Satellite}, true, b->is_satellite);
            y += 37;
        }
    }
    if (section(2, "Simulation & outputs")) {
        control("integrator", integrator_name(model.document.simulation.get_integrator()),
                Action::Integrator);
        field(window, "Fixed numerical timestep", Property::Timestep,
              number(model.document.simulation.get_dt()), rectangle(x, y + 17, w, 26));
        y += 52;
        field(window, "Numerical substeps", Property::Substeps,
              std::to_string(model.document.simulation.get_substeps()), rectangle(x, y + 17, w, 26));
        y += 52;
        field(window, "Gravity constant", Property::Gravity, number(model.document.simulation.get_gravity()),
              rectangle(x, y + 17, w, 26));
        y += 52;
        control("collisions", collision_name(model.playback.collisions), Action::CollisionPolicy);
        control("json", "Live JSON output", Action::ToggleJson, model.outputs.enable_json);
        control("csv", "Live CSV output", Action::ToggleCsv, model.outputs.enable_csv);
    }
    if (section(3, "Camera & visualization")) {
        const float third = (w - 12) / 3;
        button(window, "panel_top", "Top XY", rectangle(x, y, third, 27), {Action::Top});
        button(window, "panel_front", "Front XZ", rectangle(x + third + 6, y, third, 27), {Action::Front});
        button(window, "panel_side", "Side YZ", rectangle(x + 2 * (third + 6), y, third, 27), {Action::Side});
        y += 35;
        button(window, "panel_3d", "3D view", rectangle(x, y, w / 2 - 3, 27), {Action::Oblique});
        button(window, "panel_projection", model.world.camera.orthographic ? "Orthographic" : "Perspective",
               rectangle(x + w / 2 + 3, y, w / 2 - 3, 27), {Action::Projection});
        y += 35;
        control("field", std::string("Field: ") + model.world.field_mode_name(), Action::Field);
        control("grid", "Grid", Action::Grid, model.world.show_reference_grid);
        control("origin", "World origin axes (0,0,0)", Action::Origin, model.world.show_origin_axes);
        control("vectors", "Velocity / acceleration vectors", Action::Vectors, model.world.show_vectors);
        control("prediction", "Forecast selected body", Action::Prediction, model.world.show_prediction,
                model.document.selected_body() != nullptr);
        const auto forecast_label = model.world.show_prediction
                                        ? "Forecast: " + number(model.prediction.horizon()) +
                                              (model.document.units == "SI" ? " s" : " time units") +
                                              (model.prediction.complete() ? "" : " (building)")
                                        : "Enable forecast to see its duration";
        label(window, forecast_label, rectangle(x, y, w, 17), 11, sf::Color(243, 204, 126));
        y += 20;
        label(window, "Range x" + number(model.forecast_range), rectangle(x, y, w, 17), 11,
              sf::Color(167, 189, 217));
        y += 23;
        button(window, "panel_shorter", "Shorter", rectangle(x, y, w / 2 - 3, 27), {Action::ShorterForecast});
        button(window, "panel_longer", "Longer", rectangle(x + w / 2 + 3, y, w / 2 - 3, 27),
               {Action::LongerForecast});
        y += 35;
        control("plane", std::string("Editing plane: ") + plane_name(model.plane), Action::Plane);
    }
    content_height_ = y + scroll_;
    glDisable(GL_SCISSOR_TEST);
    panel_building_ = false;
    maximum_scroll_ = std::max(0.0f, content_height_ + 12 - bottom);
    scroll_ = std::clamp(scroll_, 0.0f, maximum_scroll_);
    if (maximum_scroll_ > 0) {
        const float track = bottom - panel_top;
        const float thumb = std::max(24.0f, track * track / (track + maximum_scroll_));
        const float offset = scroll_ / maximum_scroll_ * (track - thumb);
        box(window, rectangle(float(model.world.width) - 5, panel_top + offset, 3, thumb),
            sf::Color(81, 139, 182));
    }
}
} // namespace orbitsimlite
