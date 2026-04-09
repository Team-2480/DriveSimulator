#pragma once

#include <GLFW/glfw3.h>

#include <cstddef>
#include <map>
#include <print>

#include "raylib.h"
class GamepadControlProxy {
 public:
  GamepadControlProxy() {}
  ~GamepadControlProxy() {}

  void step() {
    has_gamepad = IsGamepadAvailable(GAMEPAD_ID);
    if (IsGamepadAvailable(GAMEPAD_ID)) {
      int count;
      const float* axes = glfwGetJoystickAxes(GAMEPAD_ID, &count);

      for (int i = 0; i < count; i++) {
        joystick_axis[i] = axes[i];
      }

      for (size_t i = 0; i < 20; i++) {
        gamepad_inputs[i] = IsGamepadButtonDown(GAMEPAD_ID, i);
      }
    }

    has_joystick = IsGamepadAvailable(JOYSTICK_ID);
    if (IsGamepadAvailable(JOYSTICK_ID)) {
      int count;
      const float* axes = glfwGetJoystickAxes(JOYSTICK_ID, &count);

      for (int i = 0; i < count; i++) {
        joystick_axis[i] = axes[i];
      }

      for (size_t i = 0; i < 20; i++) {
        joystick_inputs[i] = IsGamepadButtonDown(JOYSTICK_ID, i);
      }
    }
  }

  bool has_gamepad = false;
  std::map<size_t, bool> gamepad_inputs;
  std::map<size_t, float> gamepad_axis;

  bool has_joystick = false;
  std::map<size_t, bool> joystick_inputs;
  std::map<size_t, float> joystick_axis;

 private:
  static constexpr int GAMEPAD_ID = 1;
  static constexpr int JOYSTICK_ID = 0;
};

