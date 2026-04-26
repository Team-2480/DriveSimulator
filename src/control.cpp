#include "control.h"

#include "raylib.h"
#include "raymath.h"

void GamepadControlProxy::step(InputMethod method) {
  last_gamepad_inputs = gamepad_inputs;
  last_gamepad_axis = gamepad_axis;
  last_joystick_inputs = joystick_inputs;
  last_joystick_axis = joystick_axis;

  switch (method) {
    case INPUT_KEYBOARD:

      has_joystick = true;
      joystick_axis[0] = IsKeyDown(KEY_D) + -1 * IsKeyDown(KEY_A);
      joystick_axis[1] = IsKeyDown(KEY_S) + -1 * IsKeyDown(KEY_W);
      joystick_axis[2] = IsKeyDown(KEY_L) + -1 * IsKeyDown(KEY_J);

      joystick_inputs[4] = IsKeyDown(KEY_Q);
      joystick_inputs[11] = IsKeyDown(KEY_E);
      joystick_inputs[0] = IsKeyDown(KEY_C);

      break;
    case INPUT_JOYSTICK:
      has_gamepad = IsGamepadAvailable(GAMEPAD_ID);
      if (IsGamepadAvailable(GAMEPAD_ID)) {
        int count;
        const float* axes = glfwGetJoystickAxes(GAMEPAD_ID, &count);

        for (int i = 0; i < count; i++) {
          gamepad_axis[i] = axes[i];
        }

        auto buttons = glfwGetJoystickButtons(GAMEPAD_ID, &count);
        for (int i = 0; i < count; i++) {
          gamepad_inputs[i] = buttons[i] == GLFW_PRESS;
        }
      }

      has_joystick = IsGamepadAvailable(JOYSTICK_ID);
      if (IsGamepadAvailable(JOYSTICK_ID)) {
        int count;
        const float* axes = glfwGetJoystickAxes(JOYSTICK_ID, &count);

        for (int i = 0; i < count; i++) {
          joystick_axis[i] = axes[i];
        }

        auto buttons = glfwGetJoystickButtons(JOYSTICK_ID, &count);
        for (int i = 0; i < count; i++) {
          joystick_inputs[i] = buttons[i] == GLFW_PRESS;
        }
      }
#ifdef PLATFORM_WEB
      joystick_axis[2] += joystick_axis[5];
#endif  // PLATFORM_WEB
      break;
    case INPUT_TOUCH:
      auto joystick_x = GetScreenWidth() / 4;
      auto joystick_y = GetScreenHeight() - 200;

      auto twist_x = (GetScreenWidth() * 3) / 4;
      auto twist_y = GetScreenHeight() - 200;

      auto divider = GetScreenWidth() / 2;

      Vector2 lateral_control = Vector2Zero();
      Vector2 rotational_control = Vector2Zero();

      for (int i = GetTouchPointCount() - 1; i >= 0; i--) {
        auto pos = GetTouchPosition(i);
        if (pos.x < divider) {
          lateral_control =
              Vector2{(pos.x - joystick_x) / 100, (pos.y - joystick_y) / 100};
          if (Vector2Length(lateral_control) > 1) {
            lateral_control = Vector2Normalize(lateral_control);
          }
        }

        if (pos.x > divider) {
          rotational_control =
              Vector2{(pos.x - twist_x) / 100, (pos.y - twist_y) / 100};
          if (Vector2Length(rotational_control) > 1) {
            rotational_control = Vector2Normalize(rotational_control);
          }
        }
      }
      joystick_axis[0] = lateral_control.x;
      joystick_axis[1] = lateral_control.y;
      joystick_axis[2] = rotational_control.x;

      break;
  }
}

void GamepadControlProxy::draw(InputMethod method) {
  switch (method) {
    default:
      break;
    case INPUT_TOUCH:
      auto joystick_x = GetScreenWidth() / 4;
      auto joystick_y = GetScreenHeight() - 200;
      DrawCircle(joystick_x, joystick_y, 100, Fade(WHITE, 0.5));

      auto twist_x = (GetScreenWidth() * 3) / 4;
      auto twist_y = GetScreenHeight() - 200;
      DrawCircle(twist_x, twist_y, 100, Fade(WHITE, 0.5));

      DrawCircle(joystick_x + joystick_axis[0] * 100,
                 joystick_y + joystick_axis[1] * 100, 50, Fade(BLACK, 0.5));

      DrawCircle(twist_x + joystick_axis[2] * 100, twist_y, 50,
                 Fade(BLACK, 0.5));
      break;
  }
}

