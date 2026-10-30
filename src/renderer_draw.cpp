// Apsilume: Draw the 3D scene through the same camera used for picking, using camera-relative GPU
// coordinates. Module relationships and invariants: docs/architecture.md. Spatial drawing uses the same
// camera basis as selection and plane editing.
#include "renderer3d.hpp"
#include <SFML/OpenGL.hpp>
#include <algorithm>
#include <cmath>
namespace orbitsimlite {
namespace {
constexpr double pi = 3.14159265358979323846;
struct SpatialDraw {
    const Camera3D &camera;
    void vertex(const Vec3 &world) const {
        // Normalize only at the GPU boundary; physical state and picking remain in world doubles.
        const Vec3 v = camera.view(world) / camera.distance;
        glVertex3d(v.x, v.y, -v.z);
    }
    void color(std::uint32_t rgb, double alpha = 1) const {
        glColor4d(double((rgb >> 16) & 255) / 255, double((rgb >> 8) & 255) / 255, double(rgb & 255) / 255,
                  alpha);
    }
    void line(const Vec3 &a, const Vec3 &b) const {
        glBegin(GL_LINES);
        vertex(a);
        vertex(b);
        glEnd();
    }
    void arrow(const Vec3 &start, const Vec3 &vector, double length) const {
        if (vector.length_squared() == 0)
            return;
        const Vec3 dir = vector.normalized(), end = start + dir * length;
        Vec3 side = Vec3::cross(dir, camera.back()).normalized();
        if (side.length_squared() == 0)
            side = camera.right();
        line(start, end);
        line(end, end - dir * (length * 0.22) + side * (length * 0.09));
        line(end, end - dir * (length * 0.22) - side * (length * 0.09));
    }
    void sphere(const Body &body, double radius, bool selected) const {
        const Vec3 light = (camera.back() + camera.up() + camera.right() * 0.4).normalized();
        const auto point = [&](double latitude, double longitude) {
            const Vec3 n{std::cos(latitude) * std::cos(longitude), std::cos(latitude) * std::sin(longitude),
                         std::sin(latitude)};
            const double shade = body.is_star ? 0.95 : 0.23 + 0.77 * std::max(0.0, Vec3::dot(n, light));
            glColor3d(double((body.color >> 16) & 255) / 255 * shade,
                      double((body.color >> 8) & 255) / 255 * shade, double(body.color & 255) / 255 * shade);
            vertex(body.pos + n * radius);
        };
        for (int lat = 0; lat < 16; ++lat) {
            glBegin(GL_TRIANGLE_STRIP);
            for (int lon = 0; lon <= 28; ++lon) {
                point(-pi / 2 + pi * lat / 16, 2 * pi * lon / 28);
                point(-pi / 2 + pi * (lat + 1) / 16, 2 * pi * lon / 28);
            }
            glEnd();
        }
        if (selected) {
            glColor3d(0.95, 0.95, 1);
            glBegin(GL_LINE_LOOP);
            for (int i = 0; i < 64; ++i) {
                const double a = 2 * pi * i / 64;
                vertex(body.pos +
                       (camera.right() * std::cos(a) + camera.up() * std::sin(a)) * (radius * 1.18));
            }
            glEnd();
        }
    }
};
} // namespace
const char *Renderer3D::field_mode_name() const {
    switch (field_mode) {
    case FieldMode::None:
        return "None";
    case FieldMode::Arrows:
        return "3D arrows on edit plane";
    case FieldMode::Grid:
        return "Deformed edit plane";
    }
    return "None";
}
void Renderer3D::draw_world(sf::RenderWindow &, const Simulator &sim, const RenderContext &context) {
    const int canvas = std::max(1, int(width) - int(panel_width));
    glViewport(0, 0, canvas, int(height));
    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 0, canvas, int(height));
    glClearColor(0.025f, 0.035f, 0.065f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // The normalized near/far planes must match Camera3D::project and the grid shader depth mapping.
    const double aspect = double(canvas) / std::max(1u, height);
    if (camera.orthographic)
        glOrtho(-Camera3D::tangent * aspect, Camera3D::tangent * aspect, -Camera3D::tangent,
                Camera3D::tangent, 0.001, 10000);
    else
        glFrustum(-0.001 * Camera3D::tangent * aspect, 0.001 * Camera3D::tangent * aspect,
                  -0.001 * Camera3D::tangent, 0.001 * Camera3D::tangent, 0.001, 10000);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    SpatialDraw draw{camera};
    // Reconstruct the plane per pixel: no finite mesh border can enter the view.
    draw_continuous_field(sim, canvas, context);
    // RGB axes are fixed world X/Y/Z, independent of the camera and edit plane.
    if (show_origin_axes) {
        const double axis = std::max(camera.distance * 0.35, 100 / std::max(scale, 1e-30));
        glLineWidth(2);
        draw.color(0xF17575);
        draw.arrow({}, Vec3{1, 0, 0}, axis);
        draw.color(0x85DF9B);
        draw.arrow({}, Vec3{0, 1, 0}, axis);
        draw.color(0x79AFFF);
        draw.arrow({}, Vec3{0, 0, 1}, axis);
        glLineWidth(1);
    }
    if (field_mode == FieldMode::Arrows) {
        // Screen-distributed samples cover every visible part of the plane,
        // including after panning, rotating, and switching projections.
        constexpr int spacing = 72;
        for (int x = spacing / 2; x < canvas; x += spacing)
            for (int y = spacing / 2; y < int(height); y += spacing) {
                const auto hit = camera.intersect_plane(camera.ray(x, y, canvas, height),
                                                        plane_normal(context.plane), context.plane_offset);
                if (!hit)
                    continue;
                const double depth = camera.view(*hit).z / camera.distance;
                if (depth < 0.001 || depth > 50)
                    continue;
                const double opacity = 0.5 * (1 - std::clamp((depth - 15) / 35.0, 0.0, 1.0));
                draw.color(0x9CB9DD, opacity);
                const Vec3 acceleration =
                    Physics::acceleration(Body(1, *hit, {}, 1, 0), sim.get_bodies(), sim.get_gravity());
                const double length = (camera.orthographic ? camera.distance : camera.view(*hit).z) *
                                      Camera3D::tangent * spacing / std::max(1u, height) * 0.55;
                draw.arrow(*hit, acceleration, length);
            }
    }
    // Actual sphere depth determines occlusion; camera rotations cannot create collisions.
    for (std::size_t i = 0; i < sim.get_bodies().size(); ++i) {
        const Body &b = sim.get_bodies()[i];
        if (!b.visible)
            continue;
        draw.sphere(b, body_world_radius(b), int(i) == context.selected_index);
    }
    glDepthMask(GL_FALSE);
    for (const auto &body : sim.get_bodies()) {
        const auto found = trails_.find(body.id);
        if (!body.visible || found == trails_.end())
            continue;
        const auto &trail = found->second;
        glBegin(GL_LINE_STRIP);
        for (std::size_t j = 0; j < trail.size(); ++j) {
            draw.color(body.color, 0.15 + 0.7 * double(j + 1) / trail.size());
            draw.vertex(trail[j]);
        }
        glEnd();
    }
    // These arrows encode direction with display-scaled lengths; they are not a vector magnitude scale.
    if (show_vectors)
        for (const auto &b : sim.get_bodies()) {
            if (!b.visible)
                continue;
            draw.color(0x70E5EE);
            draw.arrow(b.pos, b.vel, camera.distance * 0.07);
            draw.color(0xF3B980);
            draw.arrow(b.pos, b.acc, camera.distance * 0.045);
        }
    if (show_prediction && context.forecast && context.forecast->points().size() > 1) {
        // Cached long-range points avoid running thousands of preview steps per draw.
        glLineWidth(2.5f);
        draw.color(0xFFD16B, 0.96);
        glBegin(GL_LINE_STRIP);
        for (const Vec3 &point : context.forecast->points())
            draw.vertex(point);
        glEnd();
        glLineWidth(1);
        if (context.forecast->complete()) {
            glPointSize(5);
            glBegin(GL_POINTS);
            draw.vertex(context.forecast->points().back());
            glEnd();
            glPointSize(1);
        }
    }
    if (context.show_cursor) {
        const double size = camera.distance * 0.006;
        draw.color(0xD8E0F0, 0.7);
        draw.line(context.cursor - Vec3{size, 0, 0}, context.cursor + Vec3{size, 0, 0});
        draw.line(context.cursor - Vec3{0, size, 0}, context.cursor + Vec3{0, size, 0});
        draw.line(context.cursor - Vec3{0, 0, size}, context.cursor + Vec3{0, 0, size});
    }
    if (show_gizmo && context.selected_index >= 0) {
        const Body &selected = sim.get_bodies()[std::size_t(context.selected_index)];
        const Vec3 axes[] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
        const std::uint32_t colors[] = {0xF17575, 0x85DF9B, 0x79AFFF};
        glDisable(GL_DEPTH_TEST);
        glLineWidth(3);
        for (const auto &handle : gizmo_handles(sim, context.selected_index)) {
            draw.color(colors[handle.axis]);
            draw.arrow(selected.pos, axes[handle.axis], handle.world_length);
        }
        glLineWidth(1);
        glEnable(GL_DEPTH_TEST);
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_SCISSOR_TEST);
}
} // namespace orbitsimlite
