#pragma once

// WARNING: must be in metric

#include <string>

#include "raylib.h"
namespace Constants {
constexpr double TRACK_WIDTH =
    0.542925;  // Distance between centers of right and left wheels on robot
constexpr double WHEEL_BASE =
    0.669925;  // Distance between centers of front and back wheels on robot

constexpr Vector3 ROBOT_SIZE = {0.794f, 0.3f, 0.940f};

constexpr float CONTROLER_DEADBAND = 0.01;
constexpr float BALL_RADIUS = 0.075;

#ifdef PLATFORM_WEB
#define RELEASE_FOLDER(value) "release/" value
#else
#define RELEASE_FOLDER(value) "../release/" value
#endif

#define VERSION_STR "v0.0.1"

}  // namespace Constants
