#pragma once

#include "Jolt/Jolt.h"
// on top

#include "Jolt/Physics/Collision/Shape/MeshShape.h"
#include "Jolt/Renderer/DebugRendererSimple.h"
namespace raylib {
#include "raylib.h"
}
#define RAYLIB_VECTOR(in_vec) \
  raylib::Vector3 { in_vec.GetX(), in_vec.GetY(), in_vec.GetZ() }
class RaylibDebugRenderer final : public JPH::DebugRendererSimple {
 public:
  raylib::Camera* camera;
  RaylibDebugRenderer(raylib::Camera* camera)
      : JPH::DebugRendererSimple(), camera(camera) {
    Initialize();
  }
  void DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3,
                    JPH::ColorArg inColor,
                    ECastShadow inCastShadow = ECastShadow::Off) {
    raylib::DrawTriangle3D(
        RAYLIB_VECTOR(inV1), RAYLIB_VECTOR(inV2), RAYLIB_VECTOR(inV3),
        raylib::Color{inColor.r, inColor.g, inColor.b, inColor.a});
  }
  void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo,
                JPH::ColorArg inColor) {
    raylib::DrawLine3D(
        RAYLIB_VECTOR(inFrom), RAYLIB_VECTOR(inTo),
        raylib::Color{inColor.r, inColor.g, inColor.b, inColor.a});
  }

  void DrawText3D(JPH::RVec3Arg inPosition, const JPH::string_view& inString,
                  JPH::ColorArg inColor, float inHeight) {
    auto screen = raylib::GetWorldToScreen(RAYLIB_VECTOR(inPosition), *camera);
    raylib::DrawText(inString.data(), screen.x, screen.y, 24,
                     raylib::Color{inColor.r, inColor.g, inColor.b, inColor.a});
  }
};

using namespace raylib;
