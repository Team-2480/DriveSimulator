#pragma once

#include <optional>
#include <string>

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

  enum TimeTrial { TRIAL_LOOP, TRIAL_EIGHT, TRIAL_EVIL } time_trial_selected;
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
  Shader gradient =
      LoadShader(RELEASE_FOLDER("lighting.vs"), RELEASE_FOLDER("gradient.fs"));
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
  std::vector<JPH::Vec3> tt_teleport_location = {
      {4.678705, 0.099892, 0.016103},
      {4.678705, 0.099892, 0.016103},
      {4.678705, 0.099892, 0.016103}};
  std::vector<float> tt_teleport_rotation = {90, 90, 90};
  std::vector<uint32_t> tt_camera_angle = {1, 1, 1};
  std::vector<std::vector<Vector3>> time_trials{
      // time_trials[0] is the loop around the field
      {{5.87, 0, 2.68},    // bottom right
       {5.87, 0, -2.68},   // top right
       {-5.87, 0, -2.68},  // top left
       {-5.87, 0, 2.68},
       {5.87, 0, 2.68}},  // bottom left

      {{5.87, 0, 2.68},  // figure eight
       {5.87, 0, -2.68},
       {0, 0, 0},
       {-5.87, 0, 2.68},
       {-5.87, 0, -2.68},
       {0, 0, 0},
       {5.87, 0, 2.68}},
      {
          // evil trial
          {4.678700, 0, -1.520016},  {2.106675, 0, -1.551985},
          {2.012209, 0, 3.305091},   {-5.806787, 0, 3.357301},
          {-5.738602, 0, -3.395988}, {1.444076, 0, -3.247883},
          {1.380765, 0, -0.063132},  {-1.623889, 0, -0.125549},
          {-1.560964, 0, -3.292732}, {-5.724292, 0, -3.379315},
          {-3.869115, 0, 1.323412},  {-1.510248, 0, 1.226238},
          {-1.347372, 0, 3.382130},  {-5.002022, 0, 3.382130},
          {-5.002022, 0, 1.581396},  {-2.548198, 0, 1.549455},
          {0.741614, 0, -1.497310},  {2.721466, 0, -3.364861},
          {6.213226, 0, -3.425054},  {6.257290, 0, 0.010247},
          {4.678782, 0, -0.046586},
      }};
  // (for robot position) field domain is [-7.83, 7.83], field range is
  // [-3.57, 3.57]
  std::string trial_creation = "";
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

  Mesh trial_target_mesh;
  Model trial_target_model;
  Mesh wheel_mesh;
  Model wheel_model;
  Model sphere_model;
  Model model;
  bool debug = false;

  Font font;
  nk_context* ctx;

  Model player_model = LoadModel(RELEASE_FOLDER("robotmodel.glb"));

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
          .position = Vector3{.x = 0.0f, .y = 6.0f, .z = 0.0f},
          .target = Vector3{.x = 0.0f, .y = 0.001f, .z = 0.0f},
          .up = Vector3{.x = 0.0f, .y = 0.0f, .z = -1.0f},
          .fovy = 90.0f,
          .projection = CAMERA_PERSPECTIVE,
      },
      Camera3D{
          .position = Vector3{.x = 8.5f, .y = 1.5f, .z = 3.0f},
          .target = Vector3{.x = 0.0f, .y = 1.0f, .z = 0.0f},
          .up = Vector3{.x = 0.0f, .y = 1.0f, .z = 0.0f},
          .fovy = 90.0f,
          .projection = CAMERA_PERSPECTIVE,
      },
      Camera3D{
          .position = Vector3{.x = 8.0f, .y = 1.5f, .z = 1.0f},
          .target = Vector3{.x = 0.0f, .y = 1.0f, .z = 0.0f},
          .up = Vector3{.x = 0.0f, .y = 1.0f, .z = 0.0f},
          .fovy = 90.0f,
          .projection = CAMERA_PERSPECTIVE,
      },
      Camera3D{
          .position = Vector3{.x = 8.5f, .y = 1.5f, .z = -2.0f},
          .target = Vector3{.x = 0.0f, .y = 1.0f, .z = 0.0f},
          .up = Vector3{.x = 0.0f, .y = 1.0f, .z = 0.0f},
          .fovy = 90.0f,
          .projection = CAMERA_PERSPECTIVE,
      }};
};
