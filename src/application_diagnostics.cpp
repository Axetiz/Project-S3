// Optional desktop-test snapshot. No file is written unless ORBITSIMLITE_UI_STATE is set.
// Reports UI geometry and current state for external test drivers; never mutates the scene.
#include "application.hpp"
#include <cstdlib>
#include <nlohmann/json.hpp>
namespace orbitsimlite {
void Application::dump_ui_state() const {
    const char *path = std::getenv("ORBITSIMLITE_UI_STATE");
    if (!path)
        return;
    using Json = nlohmann::json;
    Json state = {{"pointer", {{"x", last_mouse_.x}, {"y", last_mouse_.y}}},
                  {"scene", document_.name},
                  {"dialog", int(ui_.dialog)},
                  {"editing", ui_.editing()},
                  {"dirty", document_.dirty},
                  {"paused", playback_.paused},
                  {"time", document_.simulation.get_time()},
                  {"dt", document_.simulation.get_dt()},
                  {"speed", playback_.speed},
                  {"grid", world_.show_reference_grid},
                  {"origin", world_.show_origin_axes},
                  {"prediction", world_.show_prediction},
                  {"prediction_horizon", prediction_.horizon()},
                  {"forecast_range", forecast_range_},
                  {"data_folder", paths_.root.string()},
                  {"status", status_},
                  {"error", ui_.error()},
                  {"widgets", Json::object()}};
    for (const auto &pair : ui_.widget_bounds()) {
#if SFML_VERSION_MAJOR >= 3
        const auto p = pair.second.position, s = pair.second.size;
#else
        const sf::Vector2f p{pair.second.left, pair.second.top}, s{pair.second.width, pair.second.height};
#endif
        state["widgets"][pair.first] = {{"x", p.x}, {"y", p.y}, {"w", s.x}, {"h", s.y}};
    }
    if (const auto *selected = document_.selected_body())
        state["selected"] = {{"id", selected->id},    {"name", selected->name}, {"x", selected->pos.x},
                             {"y", selected->pos.y},  {"z", selected->pos.z},   {"vx", selected->vel.x},
                             {"vy", selected->vel.y}, {"vz", selected->vel.z}};
    state["gizmo"] = Json::array();
    for (const auto &handle : world_.gizmo_handles(document_.simulation, document_.selected_index()))
        state["gizmo"].push_back({{"axis", handle.axis},
                                  {"x0", handle.start.x},
                                  {"y0", handle.start.y},
                                  {"x1", handle.end.x},
                                  {"y1", handle.end.y}});
    SceneSerializer::atomic_write(path, state.dump());
}
} // namespace orbitsimlite
