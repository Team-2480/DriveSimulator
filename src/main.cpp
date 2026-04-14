#include <cmath>
#include <cstdio>
#include <print>

#include "control.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"

#define GLSL_VERSION 330

int main() {
  GamepadControlProxy controller_info;
  const int screenWidth = 800;
  const int screenHeight = 450;

  InitWindow(screenWidth, screenHeight, "EvilAwesomeBagleSimulator");

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

  float robot_rot = 0;
  Vector3 robot_pos = {0, 0.1, 0};

  DisableCursor();

  SetTargetFPS(60);

  while (!WindowShouldClose()) {
    controller_info.step();

    UpdateCamera(&camera, CAMERA_FREE);

    float cameraPos[3] = {camera.position.x, camera.position.y,
                          camera.position.z};
    SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos,
                   SHADER_UNIFORM_VEC3);

    BeginDrawing();

    ClearBackground(BLACK);

    BeginMode3D(camera);

    BeginShaderMode(shader);

    DrawModel(model, {}, 1.0f, WHITE);

    robot_rot -= controller_info.joystick_axis[2];

    robot_pos.x += controller_info.joystick_axis[0];
    robot_pos.z += controller_info.joystick_axis[1];

    rlPushMatrix();
    rlTranslatef(robot_pos.x, robot_pos.y, robot_pos.z);
    rlRotatef(robot_rot, 0.0f, 1.0f, 0.0f);
    DrawCubeV({0.0, 0.0, 0.0}, {0.794f, 0.2f, 0.940f}, GREEN);
    rlPopMatrix();

    EndShaderMode();
    EndMode3D();

    EndDrawing();
  }

  CloseWindow();

  return 0;
}
