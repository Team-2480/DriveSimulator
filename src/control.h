#pragma once

#include <GLFW/glfw3.h>

#include <cstddef>
#include <map>

#include "raylib.h"
class GamepadControlProxy {
 public:
  GamepadControlProxy() {}
  ~GamepadControlProxy() {}

  void step();

  bool keyboard_overide = true;

  bool has_gamepad = false;
  std::map<size_t, bool> gamepad_inputs;
  std::map<size_t, float> gamepad_axis;
  std::map<size_t, bool> last_gamepad_inputs;
  std::map<size_t, float> last_gamepad_axis;

  bool has_joystick = false;
  std::map<size_t, bool> joystick_inputs;
  std::map<size_t, float> joystick_axis;
  std::map<size_t, bool> last_joystick_inputs;
  std::map<size_t, float> last_joystick_axis;

 private:
  static constexpr int GAMEPAD_ID = 1;
  static constexpr int JOYSTICK_ID = 0;
};

