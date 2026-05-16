#include <array>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <print>
#include <vector>

#include "config.h"
#include "control.h"
#include "raylib.h"
#include "raymath.h"
#include "rcamera.h"
#include "scene.h"
#include "sqlite3.h"

class MenuScene final : public Scene {
 private:
  struct nk_context* ctx;
  Font font;
  Model map_model;
  Shader& shader;
  struct nk_image logo;
  struct nk_image keyboard;
  struct nk_image joystick;
  struct nk_image touch;
  struct nk_image shovel;
  struct nk_image time;
  struct nk_image play;

  struct nk_image github;
  struct nk_image website;
  struct nk_image donate;
  struct nk_image discord;

  struct nk_image check;

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
    font = LoadFontEx(RELEASE_FOLDER("Lato-Regular.ttf"), 20, NULL, 0);
    ctx = InitNuklearEx(font, font_size);

    logo = LoadNuklearImage(RELEASE_FOLDER("logo.png"));
    GenTextureMipmaps((Texture*)logo.handle.ptr);
    SetTextureFilter(TextureFromNuklear(logo), TEXTURE_FILTER_TRILINEAR);
    keyboard = LoadNuklearImage(RELEASE_FOLDER("keyboard.png"));
    GenTextureMipmaps((Texture*)keyboard.handle.ptr);
    SetTextureFilter(TextureFromNuklear(keyboard), TEXTURE_FILTER_TRILINEAR);
    joystick = LoadNuklearImage(RELEASE_FOLDER("3dpro.png"));
    GenTextureMipmaps((Texture*)joystick.handle.ptr);
    SetTextureFilter(TextureFromNuklear(joystick), TEXTURE_FILTER_TRILINEAR);
    touch = LoadNuklearImage(RELEASE_FOLDER("mobile.png"));
    GenTextureMipmaps((Texture*)touch.handle.ptr);
    SetTextureFilter(TextureFromNuklear(touch), TEXTURE_FILTER_TRILINEAR);
    shovel = LoadNuklearImage(RELEASE_FOLDER("shovel.png"));
    GenTextureMipmaps((Texture*)shovel.handle.ptr);
    SetTextureFilter(TextureFromNuklear(shovel), TEXTURE_FILTER_TRILINEAR);
    time = LoadNuklearImage(RELEASE_FOLDER("time.png"));
    GenTextureMipmaps((Texture*)time.handle.ptr);
    SetTextureFilter(TextureFromNuklear(time), TEXTURE_FILTER_TRILINEAR);
    play = LoadNuklearImage(RELEASE_FOLDER("play.png"));
    GenTextureMipmaps((Texture*)play.handle.ptr);
    SetTextureFilter(TextureFromNuklear(play), TEXTURE_FILTER_TRILINEAR);

    github = LoadNuklearImage(RELEASE_FOLDER("github.png"));
    GenTextureMipmaps((Texture*)github.handle.ptr);
    SetTextureFilter(TextureFromNuklear(github), TEXTURE_FILTER_TRILINEAR);
    website = LoadNuklearImage(RELEASE_FOLDER("website.png"));
    GenTextureMipmaps((Texture*)website.handle.ptr);
    SetTextureFilter(TextureFromNuklear(website), TEXTURE_FILTER_TRILINEAR);
    donate = LoadNuklearImage(RELEASE_FOLDER("donate.png"));
    GenTextureMipmaps((Texture*)donate.handle.ptr);
    SetTextureFilter(TextureFromNuklear(donate), TEXTURE_FILTER_TRILINEAR);
    discord = LoadNuklearImage(RELEASE_FOLDER("discord.png"));
    GenTextureMipmaps((Texture*)discord.handle.ptr);
    SetTextureFilter(TextureFromNuklear(discord), TEXTURE_FILTER_TRILINEAR);

    check = LoadNuklearImage(RELEASE_FOLDER("check.png"));
    GenTextureMipmaps((Texture*)check.handle.ptr);
    SetTextureFilter(TextureFromNuklear(check), TEXTURE_FILTER_TRILINEAR);

    map_model = LoadModel(RELEASE_FOLDER("map.glb"));
    for (int i = 0; i < map_model.materialCount; i++) {
      map_model.materials[i].shader = shader;
    }
  }
  ~MenuScene() {
    UnloadNuklearImage(logo);
    UnloadNuklearImage(keyboard);
    UnloadNuklearImage(joystick);
    UnloadNuklearImage(touch);
    UnloadNuklearImage(shovel);
    UnloadNuklearImage(play);

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

    BeginBlendMode(BLEND_ALPHA);
    DrawNuklear(ctx);
    EndBlendMode();
  }
  void step() override {
    UpdateNuklear(ctx);

    if (std::min(GetScreenWidth(), GetScreenHeight()) < 700) {
      SetNuklearScaling(ctx,
                        std::min(GetScreenWidth(), GetScreenHeight()) / 700.0);
    } else {
      SetNuklearScaling(ctx, 1);
    }

    /*
        uint32_t padding = 10;
        auto center_x = GetScreenWidth() / 2;
        auto width_x = GetScreenWidth() - padding * 2;
        auto center_y = GetScreenHeight() / 2;
        auto width_y = GetScreenHeight() - padding * 2;
    */

    float center_x = GetScreenWidth() / 2.0f;
    float width_x = std::min(700.0f, GetScreenWidth() / GetNuklearScaling(ctx));
    float center_y = GetScreenHeight() / 2.0f;
    float width_y =
        std::min(700.0f, GetScreenHeight() / GetNuklearScaling(ctx));

    center_x /= GetNuklearScaling(ctx);
    // width_x = GetNuklearScaling(ctx);
    center_y /= GetNuklearScaling(ctx);
    // width_y = GetNuklearScaling(ctx);

    float aspect_ratio = (float)logo.h / logo.w;

    ctx->style.window.fixed_background = nk_style_item_color({0, 0, 0, 0});
    ctx->style.button.rounding = 20;

    if (nk_begin(ctx, "Nuklear",
                 nk_rect(center_x - width_x / 2, center_y - width_y / 2,
                         width_x, width_y),
                 NK_WINDOW_BACKGROUND)) {
      switch (state.screen) {
        default:
        case ProgramState::SCREEN_IMAGE_SHOW: {
          struct nk_image image;
          switch (state.image_shown) {
            case ProgramState::IMAGE_GITHUB:
              image = github;
              break;
            case ProgramState::IMAGE_DONATE:
              image = donate;
              break;
            case ProgramState::IMAGE_WEBSITE:
              image = website;
              break;
            case ProgramState::IMAGE_DISCORD:
              image = discord;
              break;
          }
          float image_height =
              ((float)image.h / (float)image.w) * nk_layout_space_bounds(ctx).w;

          float scale_factor = 0.75;
          std::array<float, 3> dynamic_widths = {
              (1 - scale_factor) / 2, scale_factor, (1 - scale_factor) / 2};
          nk_layout_row(ctx, NK_DYNAMIC, image_height * scale_factor, 3,
                        dynamic_widths.data());
          nk_spacer(ctx);
          nk_image(ctx, image);
          nk_spacer(ctx);

          nk_layout_row_dynamic(ctx, 50, 1);
          nk_spacer(ctx);

          nk_layout_row_dynamic(ctx, 50, 3);
          nk_spacer(ctx);
          if (nk_button_label(ctx, "Back")) {
            state.screen = ProgramState::SCREEN_MAIN_MENU;
          }
          nk_spacer(ctx);
        } break;

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
          if (nk_button_label(ctx, "Leaderboard")) {
            state.screen = ProgramState::SCREEN_LEADERBOARD;
          }
          nk_spacer(ctx);

          nk_spacer(ctx);
          if (nk_button_label(ctx, "Quit")) {
            state.screen = ProgramState::SCREEN_QUIT;
          }
          nk_spacer(ctx);

          nk_layout_row_dynamic(ctx, 25, 1);
          nk_spacer(ctx);
          nk_layout_row_dynamic(ctx, 50, 5);

          nk_spacer(ctx);
          if (nk_button_label(ctx, "Github")) {
            state.screen = ProgramState::SCREEN_IMAGE_SHOW;
            state.image_shown = ProgramState::IMAGE_GITHUB;
          }
          if (nk_button_label(ctx, "Discord")) {
            state.screen = ProgramState::SCREEN_IMAGE_SHOW;
            state.image_shown = ProgramState::IMAGE_DISCORD;
          }
          if (nk_button_label(ctx, "Website")) {
            state.screen = ProgramState::SCREEN_IMAGE_SHOW;
            state.image_shown = ProgramState::IMAGE_WEBSITE;
          }
          nk_spacer(ctx);

          break;
        case ProgramState::SCREEN_CONTROL: {
#ifdef NOT_KIOSK
          nk_layout_row_dynamic(ctx, 250, 2);
#else
          std::array<float, 3> center_half = {0.25, 0.5, 0.25};
          nk_layout_row(ctx, NK_DYNAMIC, 250, 3, center_half.data());
          nk_spacer(ctx);
#endif
          float height = 210;

          ctx->style.window.fixed_background =
              nk_style_item_color({.r = 0, .g = 0, .b = 0, .a = 50});
          ctx->style.window.rounding = 20;
          ctx->style.window.group_border = 2;
          ctx->style.window.group_border_color = {
              .r = 255, .g = 255, .b = 255, .a = 255};
          ctx->style.window.group_padding = {.x = 20, .y = 20};
          ctx->style.text.color = {.r = 255, .g = 255, .b = 255, .a = 255};
          ctx->style.checkbox.cursor_normal = nk_style_item_image(check);
          ctx->style.checkbox.cursor_hover = nk_style_item_image(check);

#ifdef NOT_KIOSK
          if (nk_group_begin(ctx, "Keyboard",
                             NK_WINDOW_BACKGROUND | NK_WINDOW_BORDER)) {
            float keyboard_height = ((float)keyboard.h / (float)keyboard.w) *
                                    nk_layout_space_bounds(ctx).w;

            nk_layout_row_dynamic(ctx, keyboard_height, 1);
            nk_image(ctx, keyboard);

            nk_layout_row_dynamic(ctx, (height - keyboard_height) - 60, 1);

            nk_label(ctx,
                     "WASD lateral & JL rotational \nQ for field vs robot "
                     "releative\nE to reset field forward",
                     NK_TEXT_ALIGN_TOP | NK_TEXT_ALIGN_CENTERED);

            nk_layout_row_dynamic(ctx, 50, 1);
            if (nk_button_label(ctx, "Pick")) {
              state.input = INPUT_KEYBOARD;
              state.screen = ProgramState::SCREEN_GAME_MODE;
            }
            nk_group_end(ctx);
          }
#endif

          if (nk_group_begin(ctx, "Joystick",
                             NK_WINDOW_BACKGROUND | NK_WINDOW_BORDER)) {
            float joystick_height = ((float)joystick.h / (float)joystick.w) *
                                    nk_layout_space_bounds(ctx).w;
            nk_layout_row_dynamic(ctx, joystick_height, 1);
            nk_image(ctx, joystick);

            nk_layout_row_dynamic(ctx, (height - joystick_height) - 60, 1);

            nk_label(ctx,
                     "Joystick for movement\nButton 12 to "
                     "toggle positioning\nButton 5 to "
                     "reset field forward",
                     NK_TEXT_ALIGN_TOP | NK_TEXT_ALIGN_CENTERED);

            nk_layout_row_dynamic(ctx, 50, 1);
            if (nk_button_label(ctx, "Pick")) {
              state.input = INPUT_JOYSTICK;
              state.screen = ProgramState::SCREEN_GAME_MODE;
            }
            nk_group_end(ctx);
          }

#ifdef NOT_KIOSK

          if (nk_group_begin(ctx, "Touch",
                             NK_WINDOW_BACKGROUND | NK_WINDOW_BORDER)) {
            float touch_height = ((float)joystick.h / (float)joystick.w) *
                                 nk_layout_space_bounds(ctx).w;
            nk_layout_row_dynamic(ctx, touch_height, 1);
            nk_image(ctx, touch);

            nk_layout_row_dynamic(ctx, height - touch_height - 60, 1);
            nk_label(ctx, "Thumbs to move and rotate.",
                     NK_TEXT_ALIGN_TOP | NK_TEXT_ALIGN_CENTERED);

            nk_layout_row_dynamic(ctx, 50, 1);
            if (nk_button_label(ctx, "Pick")) {
              state.input = INPUT_TOUCH;
              state.screen = ProgramState::SCREEN_GAME_MODE;
            }

            nk_group_end(ctx);
          }
#endif

#ifndef NOT_KIOSK
          nk_spacer(ctx);
          nk_layout_row_dynamic(ctx, 100, 1);
          nk_spacer(ctx);
#endif

          nk_layout_row_dynamic(ctx, 50, 3);
          nk_spacer(ctx);
          nk_checkbox_label(ctx, "Show cheatsheet", &state.show_cheatsheet);
          nk_spacer(ctx);
          nk_spacer(ctx);
          if (nk_button_label(ctx, "Back")) {
            state.screen = ProgramState::SCREEN_MAIN_MENU;
          }
          nk_spacer(ctx);
          break;
        }
        case ProgramState::SCREEN_GAME_MODE: {
          nk_layout_row_dynamic(ctx, 250, 2);
          float height = 210;

          ctx->style.window.fixed_background =
              nk_style_item_color({.r = 0, .g = 0, .b = 0, .a = 50});
          ctx->style.window.rounding = 20;
          ctx->style.window.group_border = 2;
          ctx->style.window.group_border_color = {
              .r = 255, .g = 255, .b = 255, .a = 255};
          ctx->style.window.group_padding = {.x = 20, .y = 20};
          ctx->style.text.color = {.r = 255, .g = 255, .b = 255, .a = 255};

#ifdef NOT_KIOSK
          if (nk_group_begin(ctx, "Sandbox Mode",
                             NK_WINDOW_BACKGROUND | NK_WINDOW_BORDER)) {
            float play_height = ((float)keyboard.h / keyboard.w) *
                                nk_layout_space_bounds(ctx).w;

            nk_layout_row_dynamic(ctx, play_height, 1);
            nk_image(ctx, play);

            nk_layout_row_dynamic(ctx, (height - play_height) - 60, 1);

            nk_label(ctx, "Freeplay! No score just bliss.",
                     NK_TEXT_ALIGN_TOP | NK_TEXT_ALIGN_CENTERED);

            nk_layout_row_dynamic(ctx, 50, 1);
            if (nk_button_label(ctx, "Pick")) {
              state.gamemode = ProgramState::GAMEMODE_SANDBOX;
              state.screen = ProgramState::SCREEN_GAME;
            }
            nk_group_end(ctx);
          }
#endif

          if (nk_group_begin(ctx, "Time Trials",
                             NK_WINDOW_BACKGROUND | NK_WINDOW_BORDER)) {
            float time_height =
                ((float)time.h / time.w) * nk_layout_space_bounds(ctx).w;
            nk_layout_row_dynamic(ctx, time_height, 1);
            nk_image(ctx, time);

            nk_layout_row_dynamic(ctx, (height - time_height) - 60, 1);

            nk_label(ctx, "Complete routes with the fastest time.",
                     NK_TEXT_ALIGN_TOP | NK_TEXT_ALIGN_CENTERED);

            nk_layout_row_dynamic(ctx, 50, 1);
            if (nk_button_label(ctx, "Pick")) {
              state.screen = ProgramState::SCREEN_TRIAL_SELECT;
              // state.gamemode = ProgramState::GAMEMODE_ARCADE_TIME;
              // state.screen = ProgramState::SCREEN_GAME;
            }
            nk_group_end(ctx);
          }

          if (nk_group_begin(ctx, "Shovel Mode",
                             NK_WINDOW_BACKGROUND | NK_WINDOW_BORDER)) {
            float shovel_height =
                ((float)shovel.h / shovel.w) * nk_layout_space_bounds(ctx).w;
            nk_layout_row_dynamic(ctx, shovel_height, 1);
            nk_image(ctx, shovel);

            nk_layout_row_dynamic(ctx, (height - shovel_height) - 60, 1);

            nk_label(ctx,
                     "Shovel the most fuel in 1 minute\nto the human player "
                     "station.",
                     NK_TEXT_ALIGN_TOP | NK_TEXT_ALIGN_CENTERED);

            nk_layout_row_dynamic(ctx, 50, 1);
            if (nk_button_label(ctx, "Pick")) {
              state.gamemode = ProgramState::GAMEMODE_ARCADE_SHOVEL;
              state.screen = ProgramState::SCREEN_GAME;
            }
            nk_group_end(ctx);
          }

#ifndef NOT_KIOSK
          nk_layout_row_dynamic(ctx, 100, 1);
          nk_spacer(ctx);
#endif

          nk_layout_row_dynamic(ctx, 50, 3);
          nk_spacer(ctx);
          nk_spacer(ctx);
          nk_spacer(ctx);
          nk_spacer(ctx);
          if (nk_button_label(ctx, "Back")) {
            state.screen = ProgramState::SCREEN_CONTROL;
          }
          nk_spacer(ctx);
          break;
        }
        case ProgramState::SCREEN_TRIAL_SELECT:
          nk_layout_row_dynamic(ctx, 50, 1);
          nk_spacer(ctx);
          nk_layout_row_dynamic(ctx, 50, 3);

          nk_spacer(ctx);
          nk_label(ctx, "Select a Trial:", NK_TEXT_CENTERED);
          nk_spacer(ctx);

          nk_spacer(ctx);
          if (nk_button_label(ctx, "Figure Eight")) {
            selectTimeTrial(ProgramState::TRIAL_EIGHT);
          }
          nk_spacer(ctx);

          nk_spacer(ctx);
          if (nk_button_label(ctx, "Evil Trial")) {
            selectTimeTrial(ProgramState::TRIAL_EVIL);
          }
          nk_spacer(ctx);

          nk_spacer(ctx);
          if (nk_button_label(ctx, "Back")) {
            state.screen = ProgramState::SCREEN_GAME_MODE;
          }
          nk_spacer(ctx);
          break;

        case ProgramState::SCREEN_LEADERBOARD: {
          ctx->style.window.fixed_background =
              nk_style_item_color({.r = 0, .g = 0, .b = 0, .a = 50});
          ctx->style.window.rounding = 20;
          ctx->style.window.group_border = 2;
          ctx->style.window.group_border_color = {
              .r = 255, .g = 255, .b = 255, .a = 255};
          ctx->style.window.group_padding = {.x = 20, .y = 20};
          ctx->style.text.color = {.r = 255, .g = 255, .b = 255, .a = 255};

#ifndef NOT_KIOSK
          state.idle_timer += 1.0f / 30.0f;
#endif

          nk_layout_row_dynamic(ctx, 600, 1);

          if (nk_group_begin(ctx, "Leaderboard",
                             NK_WINDOW_BACKGROUND | NK_WINDOW_BORDER)) {
            nk_layout_row_dynamic(ctx, 20, 2);

            nk_label(ctx, "Leaderboards for: ",
                     NK_TEXT_ALIGN_MIDDLE | NK_TEXT_ALIGN_CENTERED);

            struct nk_vec2 size = {300, 300};
            nk_combobox(ctx, state.leaderboard_pretty_names.data(),
                        state.leaderboard_pretty_names.size(),
                        &state.selected_leaderboard, 20, size);
            state.leaderboard_name =
                state.leaderboard_reverse_map[state.leaderboard_pretty_names
                                                  [state.selected_leaderboard]];

            std::string leaderboard_name = state.leaderboard_name;
            if (state.leaderboard_map.contains(state.leaderboard_name)) {
              leaderboard_name = state.leaderboard_map[state.leaderboard_name];
            }
            std::string leaderboard_ordering = "DESC";
            if (state.leaderboard_ordering_map.contains(
                    state.leaderboard_name)) {
              leaderboard_ordering =
                  state.leaderboard_ordering_map[state.leaderboard_name];
            }

            query = std::format(
                "SELECT * FROM leaderboard WHERE mode = \'{}\' ORDER BY score "
                "{} LIMIT 10",
                state.leaderboard_name, leaderboard_ordering);
            nk_spacer(ctx);
            nk_spacer(ctx);
            nk_spacer(ctx);
            nk_spacer(ctx);

            nk_layout_row_dynamic(ctx, 35, 3);

            nk_label(ctx, "NAME", NK_TEXT_ALIGN_TOP | NK_TEXT_ALIGN_CENTERED);
            nk_label(ctx, "TEAM", NK_TEXT_ALIGN_TOP | NK_TEXT_ALIGN_CENTERED);
            nk_label(ctx, "SCORE", NK_TEXT_ALIGN_TOP | NK_TEXT_ALIGN_CENTERED);

            for (size_t i = 0; i < 10; i++) {
              if (i < leaderboard_cache.size()) {
                nk_label(ctx, std::get<0>(leaderboard_cache[i]).c_str(),
                         NK_TEXT_ALIGN_TOP | NK_TEXT_ALIGN_CENTERED);
                nk_label(ctx, std::get<1>(leaderboard_cache[i]).c_str(),
                         NK_TEXT_ALIGN_TOP | NK_TEXT_ALIGN_CENTERED);
                nk_label(ctx, std::get<2>(leaderboard_cache[i]).c_str(),
                         NK_TEXT_ALIGN_TOP | NK_TEXT_ALIGN_CENTERED);
              } else {
                nk_label(ctx, "---",
                         NK_TEXT_ALIGN_TOP | NK_TEXT_ALIGN_CENTERED);
                nk_label(ctx, "---",
                         NK_TEXT_ALIGN_TOP | NK_TEXT_ALIGN_CENTERED);
                nk_label(ctx, "---",
                         NK_TEXT_ALIGN_TOP | NK_TEXT_ALIGN_CENTERED);
              }
            }

            nk_layout_row_dynamic(ctx, 50, 3);

            nk_spacer(ctx);
            if (nk_button_label(ctx, "Home")) {
              state.screen = ProgramState::SCREEN_MAIN_MENU;
            }
            nk_spacer(ctx);

            nk_group_end(ctx);

#ifndef NOT_KIOSK
            if (state.idle_timer >= 60) {
              state.screen = ProgramState::SCREEN_MAIN_MENU;
            }
#endif
          }

          break;
        }
      }
    }
    nk_end(ctx);

    CameraYaw(&camera, -0.1 * DEG2RAD, true);

    if ((last_screen != state.screen &&
         state.screen == ProgramState::SCREEN_LEADERBOARD) ||
        last_query != query) {
#ifndef NOT_KIOSK
      state.idle_timer = 0;
#endif
      last_query = query;
      leaderboard_cache.clear();
      char* err_msg;
      if (sqlite3_exec(state.get_db(), query.c_str(),
                       sqlite_leaderboard_callback, (void*)this,
                       &err_msg) != SQLITE_OK) {
        printf("%s\n", err_msg);
        sqlite3_free(err_msg);
      }
    }
    last_screen = state.screen;
  }

  static int sqlite_leaderboard_callback(void* data, int argc, char** argv,
                                         char** az_col_name) {
    MenuScene* usable_data = (MenuScene*)data;

    std::string tag = "---";
    std::string team = "---";
    std::string score = "---";
    for (int i = 0; i < argc; i++) {
      if (!argv[i]) {
        continue;
      }
      if (strcmp(az_col_name[i], "tag") == 0) {
        tag = argv[i];
      } else if (strcmp(az_col_name[i], "team") == 0) {
        team = argv[i];
      } else if (strcmp(az_col_name[i], "score") == 0) {
        score = argv[i];
      }
    }
    usable_data->leaderboard_cache.push_back({tag, team, score});

    return 0;
  }

  std::string last_query;
  std::string query;
  ProgramState::ProgramScreen last_screen;
  std::vector<std::tuple<std::string, std::string, std::string>>
      leaderboard_cache;
};

static Light lights[MAX_LIGHTS];
static Shader shader;

class SceneManager {
 private:
  ProgramState state;

 public:
  SceneManager() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_MAXIMIZED |
                   FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "BagelSim");
    SetWindowMinSize(screenWidth, screenHeight);

    rlEnableColorBlend();
    rlSetBlendMode(RL_BLEND_ALPHA);

    shader = LoadShader(RELEASE_FOLDER("lighting.vs"),
                        RELEASE_FOLDER("lighting.fs"));

    shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");

    int ambient_loc = GetShaderLocation(shader, "ambient");
    float ambient_lighting[4] = {0.1f, 0.1f, 0.1f, 1.0f};
    SetShaderValue(shader, ambient_loc, ambient_lighting, SHADER_UNIFORM_VEC4);

#ifdef PLATFORM_WEB
    SetTraceLogLevel(LOG_ERROR);
#endif

    SetTargetFPS(30);
    /*
    is the max fps locked at 30 for you too? (doesn't reach 60
    even when SetTargetFPS(60))
    */

    JoltWrapper::init();

    menu_scene = std::make_shared<MenuScene>(state, shader);
  }

  ~SceneManager() {
    JoltWrapper::free();
    UnloadShader(shader);
    sqlite3_close(state.get_db());
  }
  std::optional<std::shared_ptr<GameScene>> game_scene = std::nullopt;
  std::optional<std::shared_ptr<MenuScene>> menu_scene = std::nullopt;
  std::optional<std::shared_ptr<Scene>> scene = std::nullopt;

  bool step() {
    switch (state.screen) {
      case ProgramState::SCREEN_IMAGE_SHOW:
        [[fallthrough]];
      case ProgramState::SCREEN_LEADERBOARD:
        [[fallthrough]];
      case ProgramState::SCREEN_TRIAL_SELECT:
        [[fallthrough]];
      case ProgramState::SCREEN_GAME_MODE:
        [[fallthrough]];
      case ProgramState::SCREEN_CONTROL:
        [[fallthrough]];
      case ProgramState::SCREEN_MAIN_MENU:
        if (game_scene.has_value()) {
          game_scene = {};
        }
        scene = std::static_pointer_cast<Scene>(menu_scene.value());
        break;
      case ProgramState::SCREEN_SCORE_SUBMIT:
        [[fallthrough]];
      case ProgramState::SCREEN_GAME:
        if (!game_scene.has_value()) {
          game_scene = std::make_unique<GameScene>(state, shader);
        }
        scene = std::static_pointer_cast<Scene>(game_scene.value());
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
void step_void() { manager->step(); }

int main() {
  manager = new SceneManager();

#if defined(PLATFORM_WEB)
  emscripten_set_main_loop(step_void, 0, 1);
#else
  while (step()) {
  }
#endif

  CloseWindow();

  delete manager;

  return 0;
}
