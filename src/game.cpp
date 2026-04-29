#include "Jolt/Jolt.h"
// jolt stay
#include "Jolt/Math/MathTypes.h"
#include "Jolt/Math/Quat.h"
#include "Jolt/Math/Real.h"
#include "Jolt/Math/Vec3.h"
#include "Jolt/Physics/EActivation.h"
#include "raylib.h"
#include "raymath.h"
#include "scene.h"
#include <cstddef>
#include <cstdio>
#include <vector>
GameScene::GameScene(ProgramState &program_state, Shader &shader)
    : Scene(program_state), shader(shader), jolt(shader) {

  camera.position = Vector3{0.0f, 5.0f, 5.0f}; // Camera position
  camera.target = Vector3{0.0f, 0.0f, 0.0f};   // Camera looking at point
  camera.up =
      Vector3{0.0f, 1.0f, 0.0f}; // Camera up vector (rotation towards target)
  camera.fovy = 90.0f;           // Camera field-of-view Y
  camera.projection = CAMERA_PERSPECTIVE; // Camera projection type

  model = LoadModel(RELEASE_FOLDER("map.glb"));
  for (int i = 0; i < model.materialCount; i++) {
    model.materials[i].shader = shader;
  }

  sphere_model = LoadModelFromMesh(GenMeshSphere(0.15f, 20, 20));
  for (int i = 0; i < sphere_model.materialCount; i++) {
    sphere_model.materials[i].shader = shader;
  }

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
}

bool time_trials_enabled =
    false; // delete this later and make a function to enable time trials ingame
float time_trial_selected = 0;
float time_trial_target = 0;
float tt_target_dist;
std::vector<JPH::Vec3> tt_teleport_location = {{0, 0.1, 3.2}};
std::vector<float> tt_teleport_rotation = {270};
std::vector<std::vector<Vector3>> time_trials{
    {{5.87, 0, 2.68}, // time_trials[0] is the loop around the field
     {5.87, 0, -2.68},
     {-5.87, 0, -2.68},
     {-5.87, 0, 2.68}},
};
// (for robot position) field domain is [-7.83, 7.83], field range is
// [-3.57, 3.57]

void GameScene::step() {
  controller_info.step();
  jolt.update();
  auto player_pos = jolt.get_interface().GetPosition(player_id);
  auto player_rot = jolt.get_interface().GetRotation(player_id);

  JPH::Vec3 axis;
  float angle;
  player_rot.GetAxisAngle(axis, angle);

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

  if (camera_index < camera_perspectives.size()) {
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

  if (camera_index == 11) {
    UpdateCamera(&camera, CAMERA_FREE);
    // controller_info.keyboard_overide = false;
  } else {
    // controller_info.keyboard_overide = true;
  }

  if (IsKeyDown(KEY_LEFT_SHIFT) && camera_index == 11) {
    camera.position.y -= 0.19;
    camera.target.y -= 0.19;
  }
}
void GameScene::draw() {
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
      JPH::Vec3{-player_rot.GetX() * 5,
                player_rot_velocity - player_real_ang_rot.GetY() * 0.8f,
                -player_rot.GetZ() * 5});

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

  if (time_trials_enabled) {
    tt_target_dist = Vector3Distance(
        {player_pos.GetX(), player_pos.GetY(), player_pos.GetZ()},
        time_trials[time_trial_selected][time_trial_target]);
    DrawCylinder(time_trials[time_trial_selected][time_trial_target], 0.8, 0.8,
                 0.1, 15, GREEN);
    if (tt_target_dist < 0.7 &&
        time_trial_target != time_trials[time_trial_selected].size() - 1) {
      time_trial_target++;
    } else if (tt_target_dist < 0.7 &&
               time_trial_target ==
                   time_trials[time_trial_selected].size() - 1) {
      time_trials_enabled = false;
      printf("Completed Trial with a time of %.2f seconds\n",
             GetTime() - start_time);
      // will add a visual thing later and more stuff
    }
  }
  EndMode3D();

  if (debug && time_trials_enabled == false) {
    DrawFPS(10, 10);

    DrawText( // displaying coordinates of the robot on the field
        TextFormat("X: %f, Y: %f, Z: %f\n", player_pos.GetX(),
                   player_pos.GetY(), player_pos.GetZ()),
        10, 40, 20, ORANGE);
  } else if (debug && time_trials_enabled == true) {
    DrawFPS(10, 70);

    DrawText( // displaying coordinates of the robot on the field
        TextFormat("X: %f, Z: %f\n", player_pos.GetX(), player_pos.GetZ()), 10,
        100, 20, ORANGE);
  }

  if (IsKeyPressed(KEY_T)) {
    time_trials_enabled = !time_trials_enabled;
    if (time_trials_enabled) {
      start_time = GetTime();
      time_trial_selected = 0;
      time_trial_target = 0;
      jolt.get_interface().SetPositionAndRotation(
          player_id, JPH::RVec3Arg(tt_teleport_location[time_trial_selected]),
          JPH::QuatArg::sEulerAngles(JPH::Vec3(
              0, tt_teleport_rotation[time_trial_selected] * DEG2RAD, 0)),
          JPH::EActivation::Activate);
    }
  }

  if (time_trials_enabled) {
    DrawText( // displaying the timer for the time trials
        TextFormat("Time: %.2f", ((GetTime() - start_time))), 10, 10, 20,
        SKYBLUE);
    DrawText( // displaying the timer for the time trials
        TextFormat("Trial Target Dist: %f\n", tt_target_dist), 10, 40, 20,
        ORANGE);
  }
}
