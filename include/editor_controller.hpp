// Apsilume: Central scene mutations shared by text fields, shortcuts and axis dragging.
// Numeric field input is validated here before changing the document.
#pragma once
#include "scene_document.hpp"
namespace orbitsimlite {
// XYZ position and velocity members must each stay contiguous: the inspector builds axis rows by offset.
enum class Property {
    Name,
    Mass,
    Radius,
    PositionX,
    PositionY,
    PositionZ,
    VelocityX,
    VelocityY,
    VelocityZ,
    Timestep,
    Substeps,
    Gravity
};
// All UI fields, shortcuts and drag handles share these mutation operations.
class EditorController {
  public:
    // Empty result means success; otherwise return a user-facing error and leave the value unchanged.
    std::string set_property(SceneDocument &document, Property property, const std::string &value);
    // Deltas are in world position/velocity units, never screen pixels.
    void move(SceneDocument &document, const Vec3 &delta);
    void change_velocity(SceneDocument &document, const Vec3 &delta);
    void add(SceneDocument &document, const Vec3 &point);
    void remove(SceneDocument &document);
};
} // namespace orbitsimlite
