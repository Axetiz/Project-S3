// Apsilume: Reconstruct an infinite grid from camera rays. The deformed grid is illustrative; it does not
// alter gravity. Module relationships and invariants: docs/architecture.md. Infinite world-plane field
// reconstructed from camera rays, with no mesh edges.
#include "renderer3d.hpp"
#include <SFML/OpenGL.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <numeric>
namespace orbitsimlite {
namespace {
// GLSL 1.20 works with the same compatibility context as the existing renderer.
// Camera-relative coordinates preserve precision at astronomical distances.
const char *fragment = R"GLSL(
#version 120
uniform vec2 viewport;
uniform vec3 cameraRight, cameraUp, cameraBack;
uniform vec3 planeNormal, planeU, planeV;
uniform vec2 gridOrigin;
uniform float planeHeight, gridStep, gridBlend;
uniform bool orthographic, deform;
uniform int bodyCount;
uniform vec4 bodies[64];
// Mirrors Camera3D::tangent; keep this and the C++ projection in agreement.
const float tangent=0.414213562373095;

float grid(vec2 coordinate,float spacing) {
    vec2 cell=coordinate/spacing;
    vec2 footprint=max(fwidth(cell),vec2(0.00001));
    vec2 distanceToLine=abs(fract(cell-0.5)-0.5)/footprint;
    // Suppress subpixel grids rather than letting them shimmer at the horizon.
    float density=1.0-smoothstep(0.35,1.5,max(footprint.x,footprint.y));
    return (1.0-min(min(distanceToLine.x,distanceToLine.y),1.0))*density;
}
void main() {
    vec2 pixel=(2.0*gl_FragCoord.xy-viewport)/viewport.y*tangent;
    vec3 origin=cameraBack;
    vec3 direction=-cameraBack;
    if (orthographic) origin+=cameraRight*pixel.x+cameraUp*pixel.y;
    else direction=normalize(direction+cameraRight*pixel.x+cameraUp*pixel.y);
    float incidence=dot(direction,planeNormal);
    if (abs(incidence)<0.000001) discard;
    float travel=(planeHeight-dot(origin,planeNormal))/incidence;
    if (travel<=0.0) discard;
    vec3 point=origin+direction*travel;
    float depth=dot(point-origin,-cameraBack);
    if (depth<=0.001 || depth>=10000.0) discard;
    vec2 coordinate=vec2(dot(point,planeU),dot(point,planeV))+gridOrigin;
    if (deform) {
        vec3 field=vec3(0.0);
        for (int i=0;i<64;++i) {
            if (i>=bodyCount) break;
            vec3 delta=bodies[i].xyz-point;
            float r2=dot(delta,delta)+gridStep*gridStep;
            field+=bodies[i].w*delta/(r2*sqrt(r2));
        }
        float magnitude=length(field);
        if (magnitude>0.00000001) {
            // Inverse visual mapping bends grid lines toward the summed field.
            vec3 displacement=field/magnitude*gridStep*0.6*(1.0-exp(-magnitude*0.16));
            coordinate-=vec2(dot(displacement,planeU),dot(displacement,planeV));
        }
    }
    float fine=grid(coordinate,gridStep)*(1.0-gridBlend);
    float coarse=grid(coordinate,gridStep*10.0);
    float opacity=max(fine*0.72,coarse*0.72);
    float horizon=smoothstep(0.008,0.06,abs(incidence));
    float distanceFade=1.0-smoothstep(20.0,80.0,depth);
    float alpha=opacity*horizon*distanceFade*(deform ? 0.72 : 0.38);
    if (alpha<0.003) discard;
    gl_FragColor=vec4(deform ? vec3(0.83,0.87,0.94) : vec3(0.28,0.40,0.57),alpha);
    // Match the projection used by the rest of the 3D scene.
    gl_FragDepth=orthographic ? (depth-0.001)/(10000.0-0.001) :
        0.5*(1.0+(10000.0+0.001)/(10000.0-0.001)-2.0*10000.0*0.001/((10000.0-0.001)*depth));
}
)GLSL";
sf::Glsl::Vec3 vector(const Vec3 &v) {
    return {float(v.x), float(v.y), float(v.z)};
}
} // namespace
void Renderer3D::draw_continuous_field(const Simulator &sim, int canvas, const RenderContext &context) {
    const bool deformed = field_mode == FieldMode::Grid;
    if (!show_reference_grid)
        return;
    if (!field_shader_attempted_) {
        field_shader_attempted_ = true;
        if (sf::Shader::isAvailable()) {
            auto shader = std::make_unique<sf::Shader>();
#if SFML_VERSION_MAJOR >= 3
            const bool loaded = shader->loadFromMemory(fragment, sf::Shader::Type::Fragment);
#else
            const bool loaded = shader->loadFromMemory(fragment, sf::Shader::Fragment);
#endif
            if (loaded)
                field_shader_ = std::move(shader);
        }
        if (!field_shader_)
            std::cerr << "Continuous field requires GLSL 1.20 shader support.\n";
    }
    if (!field_shader_)
        return;
    auto &shader = *field_shader_;
    const Vec3 u = context.plane == EditorPlane::YZ ? Vec3{0, 1, 0} : Vec3{1, 0, 0};
    const Vec3 v = context.plane == EditorPlane::XY ? Vec3{0, 1, 0} : Vec3{0, 0, 1};
    const Vec3 normal = plane_normal(context.plane);
    const double logarithm = std::log10(std::max(1e-30, camera.distance / 12));
    const double step = std::pow(10.0, std::floor(logarithm));
    shader.setUniform("viewport", sf::Glsl::Vec2(float(canvas), float(height)));
    shader.setUniform("cameraRight", vector(camera.right()));
    shader.setUniform("cameraUp", vector(camera.up()));
    shader.setUniform("cameraBack", vector(camera.back()));
    shader.setUniform("planeNormal", vector(normal));
    shader.setUniform("planeU", vector(u));
    shader.setUniform("planeV", vector(v));
    // Retain world anchoring while keeping large panning offsets out of float arithmetic.
    shader.setUniform(
        "gridOrigin",
        sf::Glsl::Vec2(float(std::remainder(Vec3::dot(camera.target, u), step * 10) / camera.distance),
                       float(std::remainder(Vec3::dot(camera.target, v), step * 10) / camera.distance)));
    shader.setUniform("planeHeight",
                      float((context.plane_offset - Vec3::dot(camera.target, normal)) / camera.distance));
    shader.setUniform("gridStep", float(step / camera.distance));
    shader.setUniform("gridBlend", float(logarithm - std::floor(logarithm)));
    shader.setUniform("orthographic", camera.orthographic);
    shader.setUniform("deform", deformed);
    // The field is illustrative. Bound fragment cost for very large systems by
    // retaining the 64 most massive sources; the actual solver still uses all bodies.
    std::array<sf::Glsl::Vec4, 64> sources{};
    const auto &bodies = sim.get_bodies();
    std::vector<std::size_t> order(bodies.size());
    std::iota(order.begin(), order.end(), 0);
    const std::size_t count = deformed ? std::min(order.size(), sources.size()) : 0;
    if (count)
        std::partial_sort(order.begin(), order.begin() + std::ptrdiff_t(count), order.end(),
                          [&](std::size_t a, std::size_t b) { return bodies[a].mass > bodies[b].mass; });
    const double maxMass = count ? std::max(1e-30, bodies[order[0]].mass) : 1;
    for (std::size_t i = 0; i < count; ++i) {
        const auto &b = bodies[order[i]];
        const Vec3 p = (b.pos - camera.target) / camera.distance;
        sources[i] = {float(p.x), float(p.y), float(p.z), float(b.mass / maxMass)};
    }
    shader.setUniform("bodyCount", int(count));
    shader.setUniformArray("bodies", sources.data(), sources.size());
    // A full-viewport primitive supplies fragments only; it is not a finite floor.
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDepthMask(GL_FALSE);
    sf::Shader::bind(&shader);
    glBegin(GL_TRIANGLE_STRIP);
    glVertex2f(-1, -1);
    glVertex2f(1, -1);
    glVertex2f(-1, 1);
    glVertex2f(1, 1);
    glEnd();
    sf::Shader::bind(nullptr);
    glDepthMask(GL_TRUE);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}
} // namespace orbitsimlite
