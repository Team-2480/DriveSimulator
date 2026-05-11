#include "Jolt/Jolt.h"
// on top

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <format>
#include <string>
#include <vector>

#include "Jolt/Math/MathTypes.h"
#include "Jolt/Math/Quat.h"
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
  font = LoadFontEx(RELEASE_FOLDER("Lato-Regular.ttf"), 20, NULL, 0);
  ctx = InitNuklearEx(font, font_size);

  if (state.gamemode == ProgramState::GAMEMODE_ARCADE_TIME) {
    // printf("time trial selected: %d\n", state.time_trial_selected);
    camera_index = state.time_trial_selected;
    start_time = GetTime();
    time_trial_target = 0;
    jolt.get_interface().SetPositionAndRotation(
        player_id,
        JPH::RVec3Arg(tt_teleport_location[state.time_trial_selected]),
        JPH::QuatArg::sEulerAngles(JPH::Vec3(
            0, tt_teleport_rotation[state.time_trial_selected] * DEG2RAD, 0)),
        JPH::EActivation::Activate);
  }
}

NK_API nk_bool nk_filter_caps(const struct nk_text_edit* box, nk_rune unicode) {
  if (unicode >= '0' && unicode <= '9') return nk_true;
  if (unicode >= 'A' && unicode <= 'Z') return nk_true;

  return nk_false;
}

void GameScene::step() {
  if (state.screen == ProgramState::SCREEN_GAME) {
    if (IsKeyPressed(KEY_ESCAPE)) {
      paused = !paused;
    }

    game_step();

    if (!paused) {
      if (state.gamemode == ProgramState::GAMEMODE_ARCADE_SHOVEL) {
        shovel_time_remaining -= 1.0 / 30.0;
        if (shovel_time_remaining < 0.0) {
          shovel_time_remaining = 0.0f;
          state.screen = ProgramState::SCREEN_SCORE_SUBMIT;
        }
      }

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
  if (state.screen == ProgramState::SCREEN_SCORE_SUBMIT) {
    UpdateNuklear(ctx);

    float center_x = GetScreenWidth() / 2.0f;
    float width_x = std::min(700, GetScreenWidth());
    float center_y = GetScreenHeight() / 2.0f;
    float width_y = std::min(500, GetScreenHeight());

    ctx->style.window.fixed_background = nk_style_item_color({0, 0, 0, 0});
    ctx->style.button.rounding = 20;

    ctx->style.window.fixed_background = nk_style_item_color({0, 0, 0, 50});
    ctx->style.window.rounding = 20;
    ctx->style.window.border = 2;
    ctx->style.window.padding = {20, 20};
    ctx->style.window.border_color = {255, 255, 255, 255};
    ctx->style.text.color = {255, 255, 255, 255};
    ctx->style.edit.cursor_size = 2.0;

    if (nk_begin(ctx, "Score Submit",
                 nk_rect(center_x - width_x / 2, center_y - width_y / 2,
                         width_x, width_y),
                 NK_WINDOW_BACKGROUND | NK_WINDOW_BORDER)) {
      nk_layout_row_dynamic(ctx, 50, 1);
      if (state.gamemode == ProgramState::GAMEMODE_ARCADE_SHOVEL) {
        nk_label(
            ctx,
            std::format("You scored an {} on Shovel.", shovel_score).c_str(),
            NK_TEXT_CENTERED);
      } else if (state.gamemode == ProgramState::GAMEMODE_ARCADE_TIME) {
        nk_label(ctx,
                 std::format("You completed the trial in {:.2f} seconds",
                             time_trials_stopwatch)
                     .c_str(),
                 NK_TEXT_CENTERED);
      }

      nk_layout_row_dynamic(ctx, 25, 1);
      nk_spacer(ctx);

      nk_label(ctx, "Leaderboard Info", NK_TEXT_CENTERED);
      nk_layout_row_dynamic(ctx, 50, 2);
      nk_flags name_tag_event = nk_edit_string_zero_terminated(
          ctx, NK_EDIT_FIELD | NK_EDIT_AUTO_SELECT, submit_nametag,
          sizeof(submit_nametag), nk_filter_caps);

      if (name_tag_event == NK_EDIT_ACTIVE) {
        submit_nametag_changed = true;
      }

      nk_flags number_event = nk_edit_string_zero_terminated(
          ctx, NK_EDIT_FIELD, submit_number, sizeof(submit_number),
          nk_filter_decimal);
      if (number_event == NK_EDIT_ACTIVE) {
        submit_number_changed = true;
      }

      nk_layout_row_dynamic(ctx, 50, 1);
      nk_label(ctx, "Not for display purposes:", NK_TEXT_CENTERED);
      nk_flags email_event =
          nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, submit_email,
                                         sizeof(submit_email), nk_filter_ascii);

      if (email_event == NK_EDIT_ACTIVE) {
        submit_email_changed = true;
      }

      nk_layout_row_dynamic(ctx, 50, 3);
      nk_spacer(ctx);

      if (!submit_nametag_changed || !submit_number_changed) {
        nk_widget_disable_begin(ctx);
      }

      std::string score = "---";
      std::string mode = "---";

      // NOTICE: bump these version numbers if you update the game
      if (state.gamemode == ProgramState::GAMEMODE_ARCADE_SHOVEL) {
        score = std::format("{}", shovel_score);
        mode = "shovel-v1";
      } else if (state.gamemode ==
                 ProgramState::GAMEMODE_ARCADE_TIME) {  // Time Trial Completion
        score = std::format("{:.2f}", time_trials_stopwatch);
        mode = "time-trial-v1";
      }

      if (nk_button_label(ctx, "Submit")) {
        char* query = sqlite3_mprintf(
            "REPLACE INTO leaderboard VALUES ('%q', '%q', '%q', '%q', '%q');",
            submit_nametag, submit_number, std::format("{}", score).c_str(),
            mode.c_str(), submit_email);

        char* err_msg;
        if (sqlite3_exec(state.db, query, NULL, NULL, &err_msg) != SQLITE_OK) {
          printf("%s\n", err_msg);
          sqlite3_free(err_msg);
        }
        sqlite3_free(query);

        state.screen = ProgramState::SCREEN_LEADERBOARD;
      }

      if (!submit_nametag_changed || !submit_number_changed) {
        nk_widget_disable_end(ctx);
      }

      nk_spacer(ctx);

      nk_spacer(ctx);
      nk_spacer(ctx);
      nk_spacer(ctx);

      nk_spacer(ctx);
      if (nk_button_label(ctx, "Nope! Just quit.")) {
        state.screen = ProgramState::SCREEN_MAIN_MENU;
      }
      nk_spacer(ctx);
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
  auto euler = player_rot.GetEulerAngles();
  float angle = euler.GetY();

  if (state.gamemode == ProgramState::GAMEMODE_SANDBOX) {
    if (IsKeyPressed(KEY_ONE)) {
      camera_index = 0;
      EnableCursor();
    } else if (IsKeyPressed(KEY_TWO)) {
      camera_index = 1;
      EnableCursor();
    } else if (IsKeyPressed(KEY_THREE)) {
      camera_index = 2;
      EnableCursor();
    } else if (IsKeyPressed(KEY_FOUR)) {
      camera_index = 3;
      EnableCursor();
    } else if (IsKeyPressed(KEY_NINE)) {
      camera_index = 11;
      DisableCursor();
    } else if (IsKeyPressed(KEY_ZERO)) {
      camera_index = 10;
      EnableCursor();
    }
  }

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

  if (state.gamemode == ProgramState::GAMEMODE_SANDBOX) {
    if (camera_index == 11) {
      UpdateCamera(&camera, CAMERA_FREE);
    } else {
    }

    if (IsKeyDown(KEY_LEFT_SHIFT) && camera_index == 11) {
      camera.position.y -= 0.19;
      camera.target.y -= 0.19;
    }
  }

  if (IsKeyPressed(KEY_P)) {
    debug = !debug;
  }

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
    player_rot_velocity -= std::pow(controller_info.joystick_axis[2], 3.0) * 3 *
                           speed_modifier * 4;
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
        player_velocity, {0, 1, 0}, angle);
  } else {
    corrected_player_velocity =
        Vector3RotateByAxisAngle(player_velocity, {0, 1, 0}, default_rot);
  }

  jolt.get_interface().AddLinearAndAngularVelocity(
      player_id,
      {std::clamp(corrected_player_velocity.x - player_real_velocity.GetX() * 0.1f, -0.1f, 0.1f), 0,
       std::clamp(corrected_player_velocity.z - player_real_velocity.GetZ() * 0.1f, -0.1f, 0.1f)},
      JPH::Vec3{-player_rot.GetX() * 1,
                player_rot_velocity - player_real_ang_rot.GetY() * 0.8f,
                -player_rot.GetZ() * 1});

  if (state.gamemode == ProgramState::GAMEMODE_ARCADE_SHOVEL) {
    for (auto ball : jolt.balls) {
      JPH::RVec3 position =
          jolt.get_interface().GetCenterOfMassPosition(ball.first);
      if (position.GetX() > 7.5 && position.GetZ() < -2) {
        // ball scored
        shovel_score++;
        jolt.balls[ball.first] = false;
        jolt.get_interface().DeactivateBody(ball.first);
        jolt.get_interface().SetPosition(ball.first, JPH::Vec3(0, 10, 0),
                                         JPH::EActivation::DontActivate);
      }
    }
  } else if (state.gamemode == ProgramState::GAMEMODE_ARCADE_TIME) {
    time_trials_stopwatch = GetTime() - start_time;
  }
}
void GameScene::draw() {
  game_draw();

  if (state.gamemode == ProgramState::GAMEMODE_ARCADE_SHOVEL) {
    auto time_str =
        std::format("{:02.0f}:{:02.0f}", std::roundf(shovel_time_remaining),
                    std::fmod(shovel_time_remaining, 1.0) * 100);
    auto text_size = MeasureTextEx(segment_font, time_str.c_str(), 80, 1.0);
    DrawTextEx(segment_font, time_str.c_str(),
               {GetScreenWidth() / 2.0f - text_size.x / 2.0f, 10}, 80, 1.0,
               WHITE);

    auto score_str = std::format("SCORE: {}", shovel_score);
    DrawTextEx(score_font, score_str.c_str(),
               {GetScreenWidth() / 2.0f - text_size.x / 2.0f, 10 + text_size.y},
               30, 1.0, WHITE);
  }
  if (state.gamemode == ProgramState::GAMEMODE_ARCADE_TIME) {
    auto time_str =
        std::format("{:02.0f}:{:02.0f}", std::roundf(time_trials_stopwatch),
                    std::fmod((time_trials_stopwatch), 1.0) * 100);
    auto text_size = MeasureTextEx(segment_font, time_str.c_str(), 80, 1.0);
    DrawTextEx(segment_font, time_str.c_str(),
               {GetScreenWidth() / 2.0f - text_size.x / 2.0f, 10}, 80, 1.0,
               WHITE);
  }

  if (paused && state.screen == ProgramState::SCREEN_GAME) {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), {0, 0, 0, 100});

    DrawNuklear(ctx);
  }

  if (state.screen == ProgramState::SCREEN_SCORE_SUBMIT) {
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

  if (state.gamemode == ProgramState::GAMEMODE_ARCADE_TIME &&
      state.screen == ProgramState::SCREEN_GAME) {
    tt_target_dist = Vector3Distance(
        {player_pos.GetX(), player_pos.GetY(), player_pos.GetZ()},
        time_trials[state.time_trial_selected][time_trial_target]);
    DrawCylinder(time_trials[state.time_trial_selected][time_trial_target], 0.8,
                 0.8, 0.1, 20, GREEN);
    if (tt_target_dist < 0.8 &&
        time_trial_target !=
            time_trials[state.time_trial_selected].size() - 1) {
      time_trial_target++;
    } else if (tt_target_dist < 0.8 &&
               time_trial_target ==
                   time_trials[state.time_trial_selected].size() - 1) {
      state.screen = ProgramState::SCREEN_SCORE_SUBMIT;
      /* printf("Completed Trial with a time of %.2f seconds\n",
       * time_trials_stopwatch); */
      state.gamemode = ProgramState::GAMEMODE_SANDBOX;
    }
  }
  EndMode3D();
  EndBlendMode();

  if (debug) {
    DrawFPS(10, 10);

    if (IsKeyPressed(KEY_N)) {
      printf("{%f, 0, %f}\n", player_pos.GetX(),
             player_pos.GetZ());
      trial_creation += "{" + std::to_string(player_pos.GetX()) + ", " +
                        std::to_string(player_pos.GetY()) + ", " +
                        std::to_string(player_pos.GetZ()) + "}, ";
    }
    if (IsKeyPressed(KEY_V)) {
      printf("%s\n", trial_creation.c_str());
    }
      DrawText( // displaying coordinates of the robot on the field
          TextFormat("X: %f, Y: %f, Z: %f\n", player_pos.GetX(),
                     player_pos.GetY(), player_pos.GetZ()),
          10, 40, 20, ORANGE);
      if (state.gamemode == ProgramState::GAMEMODE_ARCADE_TIME) {
        DrawText( // displaying the timer for the time trials
            TextFormat("Trial Target Dist: %f\n", tt_target_dist), 10, 70, 20,
            ORANGE);
      }
    }

    controller_info.draw(state.input);
  }
