#include "Jolt/Jolt.h"
// on top

#include <cstdio>

#include "Jolt/Math/Real.h"
#include "Jolt/Math/Vec3.h"
#include "Jolt/Physics/EActivation.h"
#include "config.h"
#include "control.h"
#include "raylib.h"
#include "raymath.h"
#include "scene.h"
GameScene::GameScene(ProgramState& program_state, Shader& shader)
    : Scene(program_state), shader(shader), jolt(shader) {
  time_trials = true;  // delete this later and make a function to enable time
                       // trials ingame

  camera.position = Vector3{0.0f, 5.0f, 5.0f};  // Camera position
  camera.target = Vector3{0.0f, 0.0f, 0.0f};    // Camera looking at point
  camera.up =
      Vector3{0.0f, 1.0f, 0.0f};  // Camera up vector (rotation towards target)
  camera.fovy = 90.0f;            // Camera field-of-view Y
  camera.projection = CAMERA_PERSPECTIVE;  // Camera projection type

  model = LoadModel(RELEASE_FOLDER("map.glb"));
  for (int i = 0; i < model.materialCount; i++) {
    model.materials[i].shader = shader;
  }

  sphere_model =
      LoadModelFromMesh(GenMeshSphere(Constants::BALL_RADIUS, 20, 20));
  for (int i = 0; i < sphere_model.materialCount; i++) {
    sphere_model.materials[i].shader = shader;
  }

  for (int i = 0; i < player_model.materialCount; i++) {
    player_model.materials[i].shader = shader;
  }

  /// sphere stuff
  for (float x = -1.729069 / 2; x < 1.729069 / 2;
       x += Constants::BALL_RADIUS * 2) {
    for (float y = -4.610849 / 2; y < 4.610849 / 2;
         y += Constants::BALL_RADIUS * 2) {
      jolt.make_ball(x, y);
    }
  }

  auto robot_start_pos = JPH::RVec3(0, 4, 0);

  if (state.gamemode == ProgramState::GAMEMODE_ARCADE_SHOVEL) {
    camera_index = 3;
    robot_start_pos = JPH::RVec3(5, Constants::ROBOT_SIZE.y * 2, -2);
    default_rot = PI / 2;
  }

  JPH::BodyCreationSettings player_settings(
      new JPH::BoxShape(JPH::RVec3(Constants::ROBOT_SIZE.x / 2,
                                   Constants::ROBOT_SIZE.y / 2,
                                   Constants::ROBOT_SIZE.z / 2)),
      robot_start_pos, JPH::Quat::sEulerAngles(JPH::Vec3(0.0, PI / 2, 0.0)),
      JPH::EMotionType::Dynamic, Layers::MOVING);

  JPH::MassProperties msp;
  msp.mMass = 30;
  msp.ScaleToMass(1);

  player_settings.mMassPropertiesOverride = msp;
  player_settings.mOverrideMassProperties =
      JPH::EOverrideMassProperties::MassAndInertiaProvided;

  player_id = jolt.get_interface().CreateAndAddBody(player_settings,
                                                    JPH::EActivation::Activate);
  jolt.get_interface().SetMaxLinearVelocity(player_id, 5);
  jolt.get_interface().SetMaxAngularVelocity(player_id, 5);
  jolt.get_interface().SetFriction(player_id, 2);

  wheel_mesh = GenMeshCylinder(0.1, 0.1, 10);
  wheel_model = LoadModelFromMesh(wheel_mesh);
  for (int i = 0; i < wheel_model.materialCount; i++) {
    wheel_model.materials[i].shader = shader;
  }

  jolt.physics_system.OptimizeBroadPhase();

  int font_size = 20;
  font = LoadFontEx((Constants::release_folder + "Lato-Regular.ttf").c_str(),
                    20, NULL, 0);
  ctx = InitNuklearEx(font, font_size);
}

void GameScene::step() {
  if (IsKeyPressed(KEY_ESCAPE)) {
    paused = !paused;
  }

  if (!paused) {
    game_step();
  } else {
    UpdateNuklear(ctx);

    float center_x = GetScreenWidth() / 2.0f;
    float width_x = std::min(700, GetScreenWidth());
    float center_y = GetScreenHeight() / 2.0f;
    float width_y = std::min(500, GetScreenHeight());

    ctx->style.window.fixed_background = nk_style_item_color({0, 0, 0, 0});
    ctx->style.button.rounding = 20;

    if (nk_begin(ctx, "Nuklear",
                 nk_rect(center_x - width_x / 2, center_y - width_y / 2,
                         width_x, width_y),
                 NK_WINDOW_BACKGROUND)) {
      nk_layout_row_dynamic(ctx, 50, 1);
      nk_spacer(ctx);
      nk_layout_row_dynamic(ctx, 50, 3);

      nk_spacer(ctx);
      if (nk_button_label(ctx, "Return To Home")) {
        state.screen = ProgramState::SCREEN_MAIN_MENU;
      }
    }
    nk_end(ctx);
  }
}

void GameScene::game_step() {
  controller_info.step(state.input);
  jolt.update();
  auto player_pos = jolt.get_interface().GetPosition(player_id);
  auto player_rot = jolt.get_interface().GetRotation(player_id);
  auto player_real_ang_rot = jolt.get_interface().GetAngularVelocity(player_id);
  auto player_real_velocity = jolt.get_interface().GetLinearVelocity(player_id);

  JPH::Vec3 axis;
  float angle;
  player_rot.GetAxisAngle(axis, angle);

  // if (state.gamemode == ProgramState::GAMEMODE_SANDBOX) {
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
  //}

  if (camera_index < camera_perspectives.size()) {
    camera_perspectives[camera_index].target = Vector3Lerp(
        camera_perspectives[camera_index].target,
        {player_pos.GetX(), player_pos.GetY(), player_pos.GetZ()}, 0.5);

    camera = camera_perspectives[camera_index];
  } else if (camera_index == 10) {
    auto forward = Vector3RotateByAxisAngle(
        {0, 0, -1}, {axis.GetX(), axis.GetY(), axis.GetZ()}, angle);

    auto pos =
        Vector3{player_pos.GetX(), player_pos.GetY() + 0.3f, player_pos.GetZ()};

    camera = Camera3D{
        .position = pos,
        .target = Vector3Add(pos, forward),
        .up = Vector3{0.0f, 1.0f, 0.0f},
        .fovy = 100.0f,
        .projection = CAMERA_PERSPECTIVE,
    };
  }

  // if (state.gamemode == ProgramState::GAMEMODE_SANDBOX) {
  if (camera_index == 11) {
    UpdateCamera(&camera, CAMERA_FREE);
  } else {
  }

  if (IsKeyDown(KEY_LEFT_SHIFT) && camera_index == 11) {
    camera.position.y -= 0.19;
    camera.target.y -= 0.19;
  }

  if (IsKeyPressed(KEY_P)) {
    debug = !debug;
  }
  //}

  player_velocity = Vector3Zero();
  player_rot_velocity = 0;

  if (controller_info.joystick_inputs[0]) {
    speed_modifier = 0.1;
  } else {
    speed_modifier = 1;
  }

  if (std::abs(controller_info.joystick_axis[0]) >
      Constants::CONTROLER_DEADBAND) {
    player_velocity.x +=
        std::pow(controller_info.joystick_axis[0], 3.0) * 5 * speed_modifier;
  }

  if (std::abs(controller_info.joystick_axis[1]) >
      Constants::CONTROLER_DEADBAND) {
    player_velocity.z +=
        std::pow(controller_info.joystick_axis[1], 3.0) * 5 * speed_modifier;
  }

  if (std::abs(controller_info.joystick_axis[2]) >
      Constants::CONTROLER_DEADBAND) {
    player_rot_velocity -=
        std::pow(controller_info.joystick_axis[2], 3.0) * 3 * speed_modifier;
  }

  if (controller_info.joystick_inputs[4] &&
      !controller_info.last_joystick_inputs[4]) {
    global_local = !global_local;
  }

  if (controller_info.joystick_inputs[11] &&
      state.gamemode != ProgramState::GAMEMODE_ARCADE_SHOVEL) {
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
      JPH::Vec3{-player_rot.GetX() * 1,
                player_rot_velocity - player_real_ang_rot.GetY() * 0.8f,
                -player_rot.GetZ() * 1});

  if (state.gamemode == ProgramState::GAMEMODE_ARCADE_SHOVEL) {
    for (auto ball : jolt.balls) {
      JPH::RVec3 position =
          jolt.get_interface().GetCenterOfMassPosition(ball.first);
      if (position.GetX() > 7.5 && position.GetZ() < -2) {
        jolt.balls[ball.first] = false;
        jolt.get_interface().DeactivateBody(ball.first);
        jolt.get_interface().SetPosition(ball.first, JPH::Vec3(0, 10, 0),
                                         JPH::EActivation::DontActivate);
      }
    }
  }
}
void GameScene::draw() {
  game_draw();

  if (paused) {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), {0, 0, 0, 100});

    DrawNuklear(ctx);
  }
}
void GameScene::game_draw() {
  auto player_pos = jolt.get_interface().GetPosition(player_id);
  auto player_rot = jolt.get_interface().GetRotation(player_id);
  auto player_real_ang_rot = jolt.get_interface().GetAngularVelocity(player_id);
  auto player_real_velocity = jolt.get_interface().GetLinearVelocity(player_id);

  JPH::Vec3 axis;
  float angle;
  player_rot.GetAxisAngle(axis, angle);

  float cameraPos[3] = {camera.position.x, camera.position.y,
                        camera.position.z};
  SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos,
                 SHADER_UNIFORM_VEC3);

  BeginBlendMode(BLEND_ALPHA);
  BeginMode3D(camera);
  BeginShaderMode(shader);

  if (!debug) {
    rlPushMatrix();
    rlTranslatef(player_pos.GetX(), player_pos.GetY(), player_pos.GetZ());

    rlRotatef(angle * RAD2DEG, axis.GetX(), axis.GetY(), axis.GetZ());

    DrawModel(player_model, {0.0, Constants::ROBOT_SIZE.y, 0.0}, 1,
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

  for (auto ball : jolt.balls) {
    JPH::RVec3 position =
        jolt.get_interface().GetCenterOfMassPosition(ball.first);
    if (ball.second) {
      DrawModel(sphere_model,
                {position.GetX(), position.GetY(), position.GetZ()}, 1.0f,
                YELLOW);
    }
  }

  if (!debug)
    DrawModel(model, {}, 1.0f, WHITE);
  else
    DrawModel(jolt.convex_model, {}, 1.0f, WHITE);

  EndShaderMode();
  EndMode3D();
  EndBlendMode();

  if (debug) {
    DrawFPS(10, 10);

    DrawText(  // displaying coordinates of the robot on the field
        TextFormat("X: %f, Z: %f\n", player_pos.GetX(), player_pos.GetZ()), 10,
        420, 20, ORANGE);
  }

  if (IsKeyPressed(KEY_T)) {
    start_time = GetTime();
  }

  if (time_trials) {
    DrawText(  // displaying the timer for the time trials
        TextFormat("Time: %.2f", ((GetTime() - start_time))), 10, 40, 20,
        SKYBLUE);
  }

  controller_info.draw(state.input);
}
