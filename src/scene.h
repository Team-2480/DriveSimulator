#pragma once

#include <optional>
#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

#include <Jolt/Jolt.h>
// jolt must be first

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Math/Real.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "config.h"
#include "control.h"
#include "jolt.h"
#include "raylib-nuklear.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "rlights.h"
#include "swerve.h"

using namespace JPH::literals;

struct ProgramState {
  enum ProgramScreen {
    SCREEN_MAIN_MENU,
    SCREEN_GAME
  } screen = SCREEN_MAIN_MENU;
};

class Scene {
 protected:
  ProgramState& state;

 public:
  Scene(ProgramState& state) : state(state) {}
  ~Scene() {}

  void virtual step() {}
  void virtual draw() {}

 private:
};

class GameScene final : public Scene {
 private:
  Shader& shader;
  Camera3D camera;

  float speed_modifier = 1;  // slowmode stuff

  bool time_trials_enabled = false;
  float start_time = GetTime();
  JPH::BodyID player_id;

  JoltWrapper jolt;
  uint32_t camera_index = 0;
  GamepadControlProxy controller_info;
  bool global_local = true;
  double default_rot = 0;
  std::array<float, 4> module_headings = {0, 0, 0, 0};

  Mesh wheel_mesh;
  Model wheel_model;
  Model sphere_model;
  Model model;
  bool debug = false;

 public:
  GameScene(ProgramState& program_state, Shader& shader);
  ~GameScene() {
    UnloadModel(wheel_model);
    UnloadModel(model);
    UnloadModel(sphere_model);
  }

  void step() override;
  void draw() override;

 private:
  std::array<Camera, 4> camera_perspectives = {

      Camera3D{
          .position = Vector3{0.0f, 5.0f, 5.0f},
          .target = Vector3{0.0f, 0.0f, 0.0f},
          .up = Vector3{0.0f, 1.0f, 0.0f},
          .fovy = 90.0f,
          .projection = CAMERA_PERSPECTIVE,
      },
      Camera3D{
          .position = Vector3{9.0f, 1.5f, 3.0f},
          .target = Vector3{0.0f, 1.0f, 0.0f},
          .up = Vector3{0.0f, 1.0f, 0.0f},
          .fovy = 90.0f,
          .projection = CAMERA_PERSPECTIVE,
      },
      Camera3D{
          .position = Vector3{9.0f, 1.5f, 1.0f},
          .target = Vector3{0.0f, 1.0f, 0.0f},
          .up = Vector3{0.0f, 1.0f, 0.0f},
          .fovy = 90.0f,
          .projection = CAMERA_PERSPECTIVE,
      },
      Camera3D{
          .position = Vector3{9.0f, 1.5f, -2.0f},
          .target = Vector3{0.0f, 1.0f, 0.0f},
          .up = Vector3{0.0f, 1.0f, 0.0f},
          .fovy = 90.0f,
          .projection = CAMERA_PERSPECTIVE,
      }};
};

