// Apsilume: Read-only scene presentation: camera, GPU resources, trails and manipulation geometry.
// World rendering is split between renderer_draw.cpp and the continuous grid in renderer_field.cpp.
#pragma once
#include "camera3d.hpp"
#include "scene.hpp"
#include "trajectory_prediction.hpp"
#include <SFML/Graphics.hpp>
#include <deque>
#include <memory>
#include <unordered_map>
namespace orbitsimlite {
// Borrowed frame data. The selected index is resolved from the document ID for this frame only.
struct RenderContext {
    int selected_index{-1};
    EditorPlane plane{EditorPlane::XY};
    double plane_offset{0};
    Vec3 cursor{};
    bool show_cursor{false};
    const TrajectoryPrediction *forecast{nullptr}; // Borrowed; drawing must not advance it.
};
// Screen-space segment for hit testing plus the matching world-space displacement scale.
struct AxisHandle {
    int axis;
    sf::Vector2f start, end;
    double world_length;
};
// Owns the camera and GPU resources. It cannot edit or advance a simulation.
class Renderer3D {
  public:
    unsigned width{1280}, height{840};
    float panel_width{340};
    double scale{2e-9};
    Camera3D camera;
    FieldMode field_mode{FieldMode::Arrows};
    bool show_vectors{true}, show_prediction{false}, show_origin_axes{true};
    bool show_reference_grid{true}, show_gizmo{true};
    void draw_world(sf::RenderWindow &window, const Simulator &simulation, const RenderContext &context);
    void fit_camera(const Simulator &simulation);
    void resize(unsigned new_width, unsigned new_height);
    // Called after simulation steps, so trail sampling follows solver time rather than rendering FPS.
    void record_trails(const Simulator &simulation);
    void clear_trails();
    sf::Vector2f world_to_screen(const Vec3 &point) const;
    // Body.radius is a reference display radius; divide by scale to obtain the collision/render proxy.
    double body_world_radius(const Body &body) const;
    double canvas_width() const;
    std::vector<AxisHandle> gizmo_handles(const Simulator &simulation, int selected) const;
    const char *field_mode_name() const;

  private:
    void draw_continuous_field(const Simulator &simulation, int canvas, const RenderContext &context);
    std::unordered_map<BodyId, std::deque<Vec3>> trails_;
    std::unique_ptr<sf::Shader> field_shader_;
    bool field_shader_attempted_{false};
};
} // namespace orbitsimlite
