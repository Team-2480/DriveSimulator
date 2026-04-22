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
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"
#include "swerve.h"

using namespace JPH::literals;

class Scene {
 public:
  Scene() {}
  ~Scene() {}

  void virtual step() {}

 private:
};

class GameScene final : Scene {
 private:
  Shader shader;
  Camera3D camera;

  float speed_modifier = 1;  // slowmode stuff

  bool time_trials = false;
  float start_time = GetTime();
  JPH::BodyID player_id;

  JoltWrapper jolt;
  uint32_t camera_index = 0;
  GamepadControlProxy controller_info;
  bool global_local = false;
  double default_rot = 0;
  std::array<float, 4> module_headings = {0, 0, 0, 0};

  Model wheel_model;
  Model sphere_model;
  Model model;
  bool debug = false;

 public:
  GameScene(Shader shader) : shader(shader), jolt(shader) {
    time_trials = true;  // delete this later and make a function to enable time
                         // trials ingame

    camera.position = Vector3{0.0f, 5.0f, 5.0f};  // Camera position
    camera.target = Vector3{0.0f, 0.0f, 0.0f};    // Camera looking at point
    camera.up = Vector3{0.0f, 1.0f,
                        0.0f};  // Camera up vector (rotation towards target)
    camera.fovy = 90.0f;        // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;  // Camera projection type

    shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");

    int ambientLoc = GetShaderLocation(shader, "ambient");
    float ambient_lighting[4] = {0.1f, 0.1f, 0.1f, 1.0f};
    SetShaderValue(shader, ambientLoc, ambient_lighting, SHADER_UNIFORM_VEC4);

    Light lights[MAX_LIGHTS];
    lights[0] = CreateLight(LIGHT_POINT, Vector3{0, 4, -4}, Vector3Zero(),
                            Color{50, 50, 50, 50}, shader);
    lights[1] = CreateLight(LIGHT_POINT, Vector3{0, 4, 4}, Vector3Zero(),
                            Color{50, 50, 50, 50}, shader);
    lights[2] = CreateLight(LIGHT_POINT, Vector3{-10, 4, 0}, Vector3Zero(),
                            Color{50, 50, 50, 50}, shader);
    lights[3] = CreateLight(LIGHT_POINT, Vector3{10, 4, 0}, Vector3Zero(),
                            Color{50, 50, 50, 50}, shader);

    model = LoadModel((Constants::release_folder + "rebuilt.obj").c_str());
    for (int i = 0; i < model.materialCount; i++) {
      model.materials[i].shader = shader;
    }

    sphere_model = LoadModelFromMesh(GenMeshSphere(0.15f, 20, 20));
    for (int i = 0; i < sphere_model.materialCount; i++) {
      sphere_model.materials[i].shader = shader;
    }

    DisableCursor();

    /// sphere stuff
    for (size_t i = 0; i < 100; i++) {
      jolt.make_ball();
    }

    JPH::BodyCreationSettings player_settings(

        new JPH::BoxShape(JPH::RVec3(Constants::ROBOT_SIZE.x / 2,
                                     Constants::ROBOT_SIZE.y / 2,
                                     Constants::ROBOT_SIZE.z / 2)),
        JPH::RVec3(0, 1, 0), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic,
        Layers::MOVING);

    JPH::MassProperties msp;
    msp.mMass = 30;
    msp.ScaleToMass(1);

    player_settings.mMassPropertiesOverride = msp;
    player_settings.mOverrideMassProperties =
        JPH::EOverrideMassProperties::MassAndInertiaProvided;

    player_id = jolt.get_interface().CreateAndAddBody(
        player_settings, JPH::EActivation::Activate);
    jolt.get_interface().SetMaxLinearVelocity(player_id, 5);
    jolt.get_interface().SetMaxAngularVelocity(player_id, 5);
    jolt.get_interface().SetFriction(player_id, 2);

    auto wheel_mesh = GenMeshCylinder(0.1, 0.1, 10);
    wheel_model = LoadModelFromMesh(wheel_mesh);
    for (int i = 0; i < wheel_model.materialCount; i++) {
      wheel_model.materials[i].shader = shader;
    }

    jolt.physics_system.OptimizeBroadPhase();
  }
  ~GameScene() {}

  void step() override {
    controller_info.step();
    jolt.update();
    auto player_pos = jolt.get_interface().GetPosition(player_id);
    auto player_rot = jolt.get_interface().GetRotation(player_id);
    auto player_real_ang_rot =
        jolt.get_interface().GetAngularVelocity(player_id);
    auto player_real_velocity =
        jolt.get_interface().GetLinearVelocity(player_id);

    JPH::Vec3 axis;
    float angle;
    player_rot.GetAxisAngle(axis, angle);

    if (IsKeyPressed(KEY_ONE)) {
      camera_index = 0;
    } else if (IsKeyPressed(KEY_TWO)) {
      camera_index = 1;
    } else if (IsKeyPressed(KEY_THREE)) {
      camera_index = 2;
    } else if (IsKeyPressed(KEY_FOUR)) {
      camera_index = 3;
    } else if (IsKeyPressed(KEY_NINE)) {
      camera_index = 11;
    } else if (IsKeyPressed(KEY_ZERO)) {
      camera_index = 10;
    }

    if (camera_index < camera_perspectives.size()) {
      camera = camera_perspectives[camera_index];
    } else if (camera_index == 10) {
      auto forward = Vector3RotateByAxisAngle(
          {0, 0, -1}, {axis.GetX(), axis.GetY(), axis.GetZ()}, angle);

      auto pos = Vector3{player_pos.GetX(), player_pos.GetY() + 0.3f,
                         player_pos.GetZ()};

      camera = Camera3D{
          .position = pos,
          .target = Vector3Add(pos, forward),
          .up = Vector3{0.0f, 1.0f, 0.0f},
          .fovy = 100.0f,
          .projection = CAMERA_PERSPECTIVE,
      };
    }

    if (camera_index == 11) {
      UpdateCamera(&camera, CAMERA_FREE);
      controller_info.keyboard_overide = false;
    } else {
      controller_info.keyboard_overide = true;
    }

    if (IsKeyDown(KEY_LEFT_SHIFT) && camera_index == 11) {
      camera.position.y -= 0.19;
      camera.target.y -= 0.19;
    }

    float cameraPos[3] = {camera.position.x, camera.position.y,
                          camera.position.z};
    SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos,
                   SHADER_UNIFORM_VEC3);

    BeginMode3D(camera);

    BeginShaderMode(shader);

    if (IsKeyPressed(KEY_P)) {
      debug = !debug;
    }

    if (!debug)
      DrawModel(model, {}, 1.0f, WHITE);
    else
      DrawModel(jolt.convex_model, {}, 1.0f, WHITE);

    Vector3 player_velocity = Vector3Zero();
    float player_rot_velocity = 0;
    if (std::abs(controller_info.joystick_axis[0]) >
        Constants::CONTROLER_DEADBAND) {
      player_velocity.x += std::pow(controller_info.joystick_axis[0], 3.0) * 5;
    }

    if (std::abs(controller_info.joystick_axis[1]) >
        Constants::CONTROLER_DEADBAND) {
      player_velocity.z += std::pow(controller_info.joystick_axis[1], 3.0) * 5;
    }

    if (std::abs(controller_info.joystick_axis[2]) >
        Constants::CONTROLER_DEADBAND) {
      player_rot_velocity -=
          std::pow(controller_info.joystick_axis[2], 3.0) * 3;
    }

    if (controller_info.joystick_inputs[4] &&
        !controller_info.last_joystick_inputs[4]) {
      global_local = !global_local;
    }

    if (controller_info.joystick_inputs[11]) {
      default_rot = angle;
    }

    Vector3 corrected_player_velocity;
    if (global_local) {
      corrected_player_velocity = Vector3RotateByAxisAngle(
          player_velocity, {axis.GetX(), axis.GetY(), axis.GetZ()}, angle);
    } else {
      corrected_player_velocity =
          Vector3RotateByAxisAngle(player_velocity, {0, 1, 0}, default_rot);
    }

    jolt.get_interface().AddLinearAndAngularVelocity(
        player_id,
        {corrected_player_velocity.x - player_real_velocity.GetX() * 0.1f, 0,
         corrected_player_velocity.z - player_real_velocity.GetZ() * 0.1f},
        JPH::Vec3{-player_real_ang_rot.GetX() * 0.01f,
                  player_rot_velocity - player_real_ang_rot.GetY() * 0.8f,
                  -player_real_ang_rot.GetZ() * 0.01f});

    if (!debug) {
      rlPushMatrix();
      rlTranslatef(player_pos.GetX(), player_pos.GetY(), player_pos.GetZ());

      rlRotatef(angle * RAD2DEG, axis.GetX(), axis.GetY(), axis.GetZ());

      DrawCubeV({0.0, Constants::ROBOT_SIZE.y, 0.0}, Constants::ROBOT_SIZE,
                global_local ? GREEN : MAGENTA);

      if (camera_index != 10) {
        DrawCubeV(Vector3{0, Constants::ROBOT_SIZE.y / 2,
                          -Constants::ROBOT_SIZE.z / 2} +
                      Vector3{0.0, 0.1 + Constants::ROBOT_SIZE.y / 2, 0.0},
                  {0.1, 0.1, 0.1}, global_local ? RED : GREEN);
      }
      rlPopMatrix();

      auto modules = calculate_swerve_states(
          ChassisSpeeds{{player_velocity.x, player_velocity.z},
                        player_rot_velocity},
          module_headings);

      for (int i = 0; i < 4; i++) {
        rlPushMatrix();

        rlTranslatef(player_pos.GetX(), player_pos.GetY(), player_pos.GetZ());
        rlRotatef(angle * RAD2DEG, axis.GetX(), axis.GetY(), axis.GetZ());
        rlTranslatef(modules_positions[i].x, 0, modules_positions[i].y);
        rlRotatef(modules[i].rot * RAD2DEG, 0, 1, 0);
        rlRotatef(90, 0, 0, -1);
        rlTranslatef(0, -0.1 + 0.1 / 2, 0);

        DrawModel(wheel_model, {0, 0, 0}, 1, MAGENTA);

        rlPopMatrix();
      }
    } else {
      rlPushMatrix();
      rlTranslatef(player_pos.GetX(), player_pos.GetY(), player_pos.GetZ());

      rlRotatef(angle * RAD2DEG, axis.GetX(), axis.GetY(), axis.GetZ());

      DrawCubeV({0.0, 0.0, 0.0}, Constants::ROBOT_SIZE,
                global_local ? GREEN : RED);
      rlPopMatrix();
    }

    auto balls = jolt.get_ball_positions();
    for (auto ball : balls) {
      DrawModel(sphere_model, ball, 1.0f, YELLOW);
    }

    EndShaderMode();
    EndMode3D();

    DrawFPS(10, 10);

    DrawText(  // displaying coordinates of the robot on the field
        TextFormat("X: %f, Z: %f\n", player_pos.GetX(), player_pos.GetZ()), 10,
        420, 20, ORANGE);

    if (IsKeyPressed(KEY_T)) {
      start_time = GetTime();
    }

    if (time_trials) {
      DrawText(  // displaying the timer for the time trials
          TextFormat("Time: %.2f", ((GetTime() - start_time))), 10, 40, 20,
          SKYBLUE);
    }
  }

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

class SceneManager {
 public:
  SceneManager() {
    InitWindow(screenWidth, screenHeight, "EvilAwesomeBagelSimulator");
    SetConfigFlags(FLAG_FULLSCREEN_MODE | FLAG_WINDOW_RESIZABLE);
    SetTraceLogLevel(LOG_ERROR);

    SetTargetFPS(30);
    /*
    is the max fps locked at 30 for you too? (doesn't reach 60
    even when SetTargetFPS(60))
    */

    JoltWrapper::init();

    Shader shader =
        LoadShader((Constants::release_folder + "lighting.vs").c_str(),
                   (Constants::release_folder + "lighting.fs").c_str());

    scene = new GameScene(shader);
  }
  ~SceneManager() {
    if (scene.has_value()) {
      delete scene.value();
    }
  }
  std::optional<GameScene*> scene = {};

  void step() {
    BeginDrawing();

    ClearBackground(BLACK);
    if (scene.has_value()) {
      scene.value()->step();
    }

    EndDrawing();
  }

 private:
  const int screenWidth = 800;
  const int screenHeight = 450;
};

static SceneManager manager;

void step() { manager.step(); }

int main() {
#if defined(PLATFORM_WEB)
  emscripten_set_main_loop(step, 0, 1);
#else
  while (!WindowShouldClose()) {
    step();
  }
#endif

  CloseWindow();

  return 0;
}
