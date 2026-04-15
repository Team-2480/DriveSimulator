
#include <Jolt/Jolt.h>

// Jolt includes
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Math/Real.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <print>
#include <ranges>

#include "Jolt/Geometry/Triangle.h"
#include "Jolt/Math/Float3.h"
#include "Jolt/Physics/Collision/Shape/MeshShape.h"
#include "Jolt/Physics/Collision/Shape/Shape.h"
#include "control.h"
#include "jolt.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"

#define GLSL_VERSION 330

using namespace JPH::literals;

int main() {
  GamepadControlProxy controller_info;
  const int screenWidth = 800;
  const int screenHeight = 450;

  InitWindow(screenWidth, screenHeight, "EvilAwesomeBagelSimulator");

  Camera3D camera = {0};
  camera.position = (Vector3){10.0f, 10.0f, 10.0f};  // Camera position
  camera.target = (Vector3){0.0f, 0.0f, 0.0f};       // Camera looking at point
  camera.up = (Vector3){0.0f, 1.0f,
                        0.0f};  // Camera up vector (rotation towards target)
  camera.fovy = 45.0f;          // Camera field-of-view Y
  camera.projection = CAMERA_PERSPECTIVE;  // Camera projection type

  Shader shader =
      LoadShader(TextFormat("../release/lighting.vs", GLSL_VERSION),
                 TextFormat("../release/lighting.fs", GLSL_VERSION));

  shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");

  int ambientLoc = GetShaderLocation(shader, "ambient");
  SetShaderValue(shader, ambientLoc, (float[4]){0.1f, 0.1f, 0.1f, 1.0f},
                 SHADER_UNIFORM_VEC4);

  Light lights[MAX_LIGHTS] = {0};
  lights[0] = CreateLight(LIGHT_POINT, (Vector3){0, 4, -4}, Vector3Zero(),
                          Color{50, 50, 50, 50}, shader);
  lights[1] = CreateLight(LIGHT_POINT, (Vector3){0, 4, 4}, Vector3Zero(),
                          Color{50, 50, 50, 50}, shader);
  lights[2] = CreateLight(LIGHT_POINT, (Vector3){-10, 4, 0}, Vector3Zero(),
                          Color{50, 50, 50, 50}, shader);
  lights[3] = CreateLight(LIGHT_POINT, (Vector3){10, 4, 0}, Vector3Zero(),
                          Color{50, 50, 50, 50}, shader);

  Model model = LoadModel("../release/rebuilt.gltf");
  for (size_t i = 0; i < model.materialCount; i++) {
    model.materials[i].shader = shader;
  }

  Model sphere_model = LoadModelFromMesh(GenMeshSphere(0.14986f, 20, 20));
  for (size_t i = 0; i < sphere_model.materialCount; i++) {
    sphere_model.materials[i].shader = shader;
  }

  float robot_rot = 0;
  Vector3 robot_pos = {0, 0.1, 0};

  DisableCursor();

  SetTargetFPS(60);

  JoltWrapper::init();
  JoltWrapper jolt(shader);

  /// sphere stuff

  jolt.physics_system.OptimizeBroadPhase();

  bool debug = false;
  while (!WindowShouldClose()) {
    jolt.make_ball();

    controller_info.step();
    jolt.update();

    UpdateCamera(&camera, CAMERA_FREE);

    float cameraPos[3] = {camera.position.x, camera.position.y,
                          camera.position.z};
    SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos,
                   SHADER_UNIFORM_VEC3);

    BeginDrawing();

    ClearBackground(BLACK);

    BeginMode3D(camera);

    BeginShaderMode(shader);

    if (IsKeyPressed(KEY_P)) {
      debug = !debug;
    }

    if (!debug)
      DrawModel(model, {}, 1.0f, WHITE);
    else
      DrawModel(jolt.convex_model, {}, 1.0f, WHITE);

    if (!(controller_info.joystick_axis[0] >= -0.1 &&
          controller_info.joystick_axis[0] <= 0.1)) {

      robot_pos.x += controller_info.joystick_axis[0] / 2;
    }

    if (!(controller_info.joystick_axis[1] >= -0.1 &&
          controller_info.joystick_axis[1] <= 0.1)) {

      robot_pos.z += controller_info.joystick_axis[1] / 2;
    }

    if (!(controller_info.joystick_axis[2] >= -0.2 &&
          controller_info.joystick_axis[2] <= 0.2)) {

      robot_rot -= controller_info.joystick_axis[2] * 6;
    }

    if (controller_info.joystick_inputs[4]) {
      robot_pos.y += 0.1;
      // printf("fly\n");
    }
    if (controller_info.joystick_inputs[5]) {
      robot_pos.y -= 0.1;
      // printf("no fly\n");
    }

    /*
   robot_pos = Vector3Add(robot_pos, Vector3Scale({sin(robot_rot * DEG2RAD), 0,
                                                   cos(robot_rot * DEG2RAD)},
                                                  0.1));
                                                  */

    rlPushMatrix();
    rlTranslatef(robot_pos.x, robot_pos.y, robot_pos.z);
    rlRotatef(robot_rot, 0.0f, 1.0f, 0.0f);
    DrawCubeV({0.0, 0.0, 0.0}, {0.794f, 0.2f, 0.940f}, GREEN);
    rlPopMatrix();

    auto balls = jolt.get_ball_positions();
    for (auto ball : balls) {
      DrawModel(sphere_model, ball, 1.0f, YELLOW);
    }
    /*
    DrawSphere(
      {position.GetX(), position.GetY(), position.GetZ()}, 0.14986f, GREEN);
      */

    EndShaderMode();
    EndMode3D();

    EndDrawing();
  }

  CloseWindow();

  return 0;
}
