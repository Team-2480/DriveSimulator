#include "control.h"

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
  }
}

