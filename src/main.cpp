#include <cstdio>
#include <print>

#include "control.h"
#include "raylib.h"

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

  DisableCursor();

  SetTargetFPS(60);

  while (!WindowShouldClose()) {
    controller_info.step();

    UpdateCamera(&camera, CAMERA_FREE);

    BeginDrawing();

    ClearBackground(RAYWHITE);

    BeginMode3D(camera);

    DrawModel(model, {}, 1.0f, WHITE);
    DrawGrid(10, 1.0f);

    EndMode3D();

    // DrawText("Congrats! You created your first window!", 190, 200, 20,
    //          LIGHTGRAY);

    EndDrawing();
  }

  CloseWindow();

  return 0;
}
