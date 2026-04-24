#include <cinttypes>

#include "config.h"
#include "control.h"
#include "raylib.h"
#include "raymath.h"
#include "rcamera.h"
#include "scene.h"

class MenuScene final : public Scene {
 private:
  struct nk_context* ctx;
  Font font;
  Model map_model;
  Shader& shader;
  struct nk_image logo;
  struct nk_image keyboard;
  struct nk_image joystick;

  Camera ui_camera{
      .position = Vector3{0.0f, -5.0f, 10.0f},
      .target = Vector3{0.0f, 0.0f, 0.0f},
      .up = Vector3{0.0f, 1.0f, 0.0f},
      .fovy = 100.0f,
      .projection = CAMERA_PERSPECTIVE,
  };
  Camera camera{
      .position = Vector3{0.0f, 4.0f, 10.0f},
      .target = Vector3{0.0f, 0.0f, 0.0f},
      .up = Vector3{0.0f, 1.0f, 0.0f},
      .fovy = 100.0f,
      .projection = CAMERA_PERSPECTIVE,
  };

 public:
  MenuScene(ProgramState& program_state, Shader& shader)
      : Scene(program_state), shader(shader) {
    int font_size = 20;
    font = LoadFontEx((Constants::release_folder + "Lato-Regular.ttf").c_str(),
                      20, NULL, 0);
    ctx = InitNuklearEx(font, font_size);

    logo = LoadNuklearImage(RELEASE_FOLDER("logo.png"));
    keyboard = LoadNuklearImage(RELEASE_FOLDER("keyboard.png"));
    joystick = LoadNuklearImage(RELEASE_FOLDER("3dpro.png"));

    map_model = LoadModel(RELEASE_FOLDER("map.glb"));
    for (int i = 0; i < map_model.materialCount; i++) {
      map_model.materials[i].shader = shader;
    }
  }
  ~MenuScene() {
    UnloadNuklearImage(logo);
    UnloadNuklearImage(keyboard);
    UnloadNuklearImage(joystick);

    UnloadNuklear(ctx);
    UnloadFont(font);
    UnloadModel(map_model);
  }

  void draw() override {
    float cameraPos[3] = {camera.position.x, camera.position.y,
                          camera.position.z};
    SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos,
                   SHADER_UNIFORM_VEC3);

    BeginMode3D(camera);

    BeginShaderMode(shader);

    DrawModel(map_model, Vector3Zero(), 1, WHITE);

    EndShaderMode();
    EndMode3D();

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), {0, 0, 0, 100});
    DrawTextEx(font, VERSION_STR " git " GIT_HASH, {10, 10}, 20, 0, GRAY);

    DrawNuklear(ctx);
  }
  void step() override {
    UpdateNuklear(ctx);

    /*
        uint32_t padding = 10;
        auto center_x = GetScreenWidth() / 2;
        auto width_x = GetScreenWidth() - padding * 2;
        auto center_y = GetScreenHeight() / 2;
        auto width_y = GetScreenHeight() - padding * 2;
    */

    float center_x = GetScreenWidth() / 2.0f;
    float width_x = std::min(700, GetScreenWidth());
    float center_y = GetScreenHeight() / 2.0f;
    float width_y = std::min(500, GetScreenHeight());

    float aspect_ratio = (float)logo.h / logo.w;

    ctx->style.window.fixed_background = nk_style_item_color({0, 0, 0, 0});
    ctx->style.button.rounding = 20;

    if (nk_begin(ctx, "Nuklear",
                 nk_rect(center_x - width_x / 2, center_y - width_y / 2,
                         width_x, width_y),
                 NK_WINDOW_BACKGROUND)) {
      switch (state.screen) {
        default:
        case ProgramState::SCREEN_MAIN_MENU:
          nk_layout_row_dynamic(ctx, aspect_ratio * width_x, 1);

          nk_image(ctx, logo);

          nk_layout_row_dynamic(ctx, 50, 1);
          nk_spacer(ctx);
          nk_layout_row_dynamic(ctx, 50, 3);

          nk_spacer(ctx);
          if (nk_button_label(ctx, "Play")) {
            state.screen = ProgramState::SCREEN_CONTROL;
          }
          nk_spacer(ctx);

          nk_spacer(ctx);
          if (nk_button_label(ctx, "Quit")) {
            state.screen = ProgramState::SCREEN_QUIT;
          }
          nk_spacer(ctx);
          break;
        case ProgramState::SCREEN_CONTROL:
          nk_layout_row_dynamic(ctx, width_y - 20, 2);
          float height = width_y - 60;

          ctx->style.window.fixed_background =
              nk_style_item_color({255, 255, 255, 200});
          ctx->style.window.rounding = 20;
          ctx->style.window.group_padding = {20, 20};
          ctx->style.text.color = {50, 50, 50, 255};

          if (nk_group_begin(ctx, "Keyboard", NK_WINDOW_BACKGROUND)) {
            float keyboard_height = ((float)keyboard.h / keyboard.w) *
                                    nk_layout_space_bounds(ctx).w;

            nk_layout_row_dynamic(ctx, keyboard_height, 1);
            nk_image(ctx, keyboard);

            nk_layout_row_dynamic(ctx, height - keyboard_height - 60, 1);

            nk_label(ctx,
                     "WASD lateral & JL rotational \nQ for field vs robot "
                     "releative\nE to reset field forward\nNote: Scores cannot "
                     "be submited to "
                     "\nleaderboards.",
                     NK_TEXT_LEFT);

            nk_layout_row_dynamic(ctx, 50, 1);
            if (nk_button_label(ctx, "Pick")) {
              state.input = INPUT_KEYBOARD;
              state.screen = ProgramState::SCREEN_GAME;
            }
            nk_group_end(ctx);
          }

          if (nk_group_begin(ctx, "Joystick", NK_WINDOW_BACKGROUND)) {
            float joystick_height = ((float)joystick.h / joystick.w) *
                                    nk_layout_space_bounds(ctx).w;
            nk_layout_row_dynamic(ctx, joystick_height, 1);
            nk_image(ctx, joystick);

            nk_layout_row_dynamic(ctx, height - joystick_height - 60, 1);
            nk_label(ctx,
                     "Joystick for movement\nTwist to turn\nButton 12 to "
                     "toggle field vs robot relative\nButton 6 to "
                     "reset field forward",
                     NK_TEXT_LEFT);

            nk_layout_row_dynamic(ctx, 50, 1);
            if (nk_button_label(ctx, "Pick")) {
              state.input = INPUT_JOYSTICK;
              state.screen = ProgramState::SCREEN_GAME;
            }

            nk_group_end(ctx);
          }

          break;
      }
    }
    nk_end(ctx);

    CameraYaw(&camera, -0.1 * DEG2RAD, true);
  }
};

static Light lights[MAX_LIGHTS];
static Shader shader;

class SceneManager {
 private:
  ProgramState state;

 public:
  SceneManager() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_MAXIMIZED);
    InitWindow(screenWidth, screenHeight, "BagelSim");

    shader = LoadShader((Constants::release_folder + "lighting.vs").c_str(),
                        (Constants::release_folder + "lighting.fs").c_str());
#ifdef PLATFORM_WEB
    SetTraceLogLevel(LOG_ERROR);
#endif

    SetTargetFPS(30);
    /*
    is the max fps locked at 30 for you too? (doesn't reach 60
    even when SetTargetFPS(60))
    */

    JoltWrapper::init();

    game_scene = new GameScene(state, shader);
    menu_scene = new MenuScene(state, shader);
  }
  ~SceneManager() {
    JoltWrapper::free();
    if (game_scene.has_value()) {
      delete game_scene.value();
    }
    if (menu_scene.has_value()) {
      delete menu_scene.value();
    }
    UnloadShader(shader);
  }
  std::optional<GameScene*> game_scene = std::nullopt;
  std::optional<MenuScene*> menu_scene = std::nullopt;
  std::optional<Scene*> scene = std::nullopt;

  bool step() {
    if (WindowShouldClose()) {
      return false;
    }

    switch (state.screen) {
      case ProgramState::SCREEN_CONTROL:
        [[fallthrough]];
      case ProgramState::SCREEN_MAIN_MENU:
        scene = static_cast<Scene*>(menu_scene.value());
        break;
      case ProgramState::SCREEN_GAME:
        scene = static_cast<Scene*>(game_scene.value());
        break;
      case ProgramState::SCREEN_QUIT:
        return false;
        break;
    }
    if (scene.has_value()) {
      scene.value()->step();
    }
    BeginDrawing();

    ClearBackground(BLACK);
    if (scene.has_value()) {
      scene.value()->draw();
    }

    EndDrawing();
    return true;
  }

 private:
  const int screenWidth = 800;
  const int screenHeight = 450;
};

static SceneManager* manager;

bool step() { return manager->step(); }

int main() {
  manager = new SceneManager;

  shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");

  int ambient_loc = GetShaderLocation(shader, "ambient");
  float ambient_lighting[4] = {0.1f, 0.1f, 0.1f, 1.0f};
  SetShaderValue(shader, ambient_loc, ambient_lighting, SHADER_UNIFORM_VEC4);

  lights[0] = CreateLight(LIGHT_POINT, Vector3{0, 4, -4}, Vector3Zero(),
                          Color{50, 50, 50, 50}, shader, 0);
  lights[1] = CreateLight(LIGHT_POINT, Vector3{0, 4, 4}, Vector3Zero(),
                          Color{50, 50, 50, 50}, shader, 1);
  lights[2] = CreateLight(LIGHT_POINT, Vector3{-10, 4, 0}, Vector3Zero(),
                          Color{50, 50, 50, 50}, shader, 2);
  lights[3] = CreateLight(LIGHT_POINT, Vector3{10, 4, 0}, Vector3Zero(),
                          Color{50, 50, 50, 50}, shader, 3);

#if defined(PLATFORM_WEB)
  emscripten_set_main_loop(step, 0, 1);
#else
  while (step()) {
  }
#endif

  CloseWindow();

  delete manager;

  return 0;
}
