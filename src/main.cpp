#include <cmath>
#include <cstdio>
#include <print>

#include "control.h"
#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"

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

  Shader shader = LoadShader(
      TextFormat("resources/shaders/glsl%i/lighting_instancing.vs",
                 GLSL_VERSION),
      TextFormat("resources/shaders/glsl%i/lighting.fs", GLSL_VERSION));

  Model model = LoadModel("../release/rebuilt.gltf");
  Texture2D texture = LoadTexture("../release/rebuilt.gltf");
  model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;

  float robot_rot = 0;
  Vector3 robot_pos = {0,0,0};

  DisableCursor();

  SetTargetFPS(60);

  while (!WindowShouldClose()) {
    controller_info.step();

    UpdateCamera(&camera, CAMERA_FREE);

    BeginDrawing();

    ClearBackground(RAYWHITE);

    BeginMode3D(camera);

    DrawModel(model, {}, 1.0f, WHITE);

    robot_rot -= controller_info.joystick_axis[2];
    robot_pos= Vector3Add(robot_pos, Vector3Scale({sin(robot_rot * DEG2RAD), 0, cos(robot_rot * DEG2RAD)}, 0.1));
    
    rlPushMatrix();
    rlRotatef(robot_rot, 0.0f, 1.0f, 0.0f);
    DrawCubeV(robot_pos, {0.794f, 0.2f, 0.940f}, GREEN);
    rlPopMatrix();


    EndMode3D();

    EndDrawing();
  }

  CloseWindow();

  return 0;
}
