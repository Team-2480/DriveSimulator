#pragma once

#include <optional>

#include "sqlite3.h"
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
    SCREEN_QUIT,
    SCREEN_MAIN_MENU,
    SCREEN_CONTROL,
    SCREEN_GAME_MODE,
    SCREEN_GAME,
    SCREEN_SCORE_SUBMIT,
    SCREEN_TRIAL_SELECT,
    SCREEN_LEADERBOARD
  } screen = SCREEN_MAIN_MENU;

  enum GameMode {
    GAMEMODE_ARCADE_SHOVEL,
    GAMEMODE_ARCADE_TIME,
    GAMEMODE_SANDBOX,
  } gamemode = GAMEMODE_SANDBOX;

  enum TimeTrial { TRIAL_LOOP, TRIAL_EIGHT } time_trial_selected;
  InputMethod input = INPUT_KEYBOARD;

  sqlite3* db;
};

class Scene {
 protected:
  ProgramState& state;

 public:
  // time trials stuff that needed to be public
  void selectTimeTrial(enum ::ProgramState::TimeTrial time_trial_id) {
    state.time_trial_selected = time_trial_id;
    state.gamemode = ProgramState::GAMEMODE_ARCADE_TIME;
    state.screen = ProgramState::SCREEN_GAME;
    printf("time trial selected: %d\n", state.time_trial_selected);
  }
  Scene(ProgramState& state) : state(state) {}
  ~Scene() {}

  void virtual step() {}
  void virtual draw() {}

 private:
};

class GameScene final : public Scene {
 private:
  bool paused = false;
  Shader& shader;
  Camera3D camera;

  float speed_modifier = 1;  // slowmode stuff

  // score submit
  char submit_nametag[6] = "NAME\0";
  bool submit_nametag_changed = false;
  char submit_number[6] = "00000";
  bool submit_number_changed = false;
  char submit_email[256] = "name@domain.com";
  bool submit_email_changed = false;

  // time trials
  float time_trials_stopwatch;
  float time_trial_target;
  float tt_target_dist;
  std::vector<JPH::Vec3> tt_teleport_location = {{0, 0.1, 3.2}, {0, 0.1, 3.2}};
  std::vector<float> tt_teleport_rotation = {270, 270};
  std::vector<std::vector<Vector3>> time_trials{
      // time_trials[0] is the loop around the field
      {{5.87, 0, 2.68},    // bottom right
       {5.87, 0, -2.68},   // top right
       {-5.87, 0, -2.68},  // top left
       {-5.87, 0, 2.68},
       {5.87, 0, 2.68}},  // bottom left

      {{5.87, 0, 2.68},
       {5.87, 0, -2.68},
       {0, 0, 0},
       {-5.87, 0, 2.68},
       {-5.87, 0, -2.68},
       {0, 0, 0},
       {5.87, 0, 2.68}}};
  // (for robot position) field domain is [-7.83, 7.83], field range is
  // [-3.57, 3.57]
  float start_time = GetTime();

  // shovel
  float shovel_time_remaining = 60.0;
  size_t shovel_score = 0;
  Font segment_font = LoadFontEx(RELEASE_FOLDER("Lato-Black.ttf"), 80, NULL, 0);
  Font score_font = LoadFontEx(RELEASE_FOLDER("Lato-Bold.ttf"), 30, NULL, 0);

  // always
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

  Font font;
  nk_context* ctx;

  Mesh player_cube =
      GenMeshCube(Constants::ROBOT_SIZE.x, Constants::ROBOT_SIZE.y,
                  Constants::ROBOT_SIZE.z);
  Model player_model = LoadModelFromMesh(player_cube);

  Vector3 player_velocity;
  float player_rot_velocity;

 public:
  GameScene(ProgramState& program_state, Shader& shader);
  ~GameScene() {
    UnloadModel(wheel_model);
    UnloadModel(model);
    UnloadModel(sphere_model);
    UnloadModel(player_model);
    UnloadFont(font);
    UnloadFont(segment_font);
    UnloadFont(score_font);

    UnloadNuklear(ctx);
  }

  void step() override;
  void game_step();
  void draw() override;
  void game_draw();

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
          .position = Vector3{8.5f, 1.5f, 3.0f},
          .target = Vector3{0.0f, 1.0f, 0.0f},
          .up = Vector3{0.0f, 1.0f, 0.0f},
          .fovy = 90.0f,
          .projection = CAMERA_PERSPECTIVE,
      },
      Camera3D{
          .position = Vector3{8.0f, 1.5f, 1.0f},
          .target = Vector3{0.0f, 1.0f, 0.0f},
          .up = Vector3{0.0f, 1.0f, 0.0f},
          .fovy = 90.0f,
          .projection = CAMERA_PERSPECTIVE,
      },
      Camera3D{
          .position = Vector3{8.5f, 1.5f, -2.0f},
          .target = Vector3{0.0f, 1.0f, 0.0f},
          .up = Vector3{0.0f, 1.0f, 0.0f},
          .fovy = 90.0f,
          .projection = CAMERA_PERSPECTIVE,
      }};
};
