add_executable(bagelsim
 main.cpp
 jolt.cpp
 control.cpp
 swerve.cpp
 nuklear.cpp
 rlights.cpp
 game.cpp
 nkutils.cpp
)

if (EMSCRIPTEN)
  add_compile_options(-O3)
  set_target_properties(bagelsim PROPERTIES OUTPUT_NAME "index")

  add_custom_command(TARGET bagelsim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_SOURCE_DIR}/shell/*"
        ${CMAKE_SOURCE_DIR}/webport/ 
        && ${CMAKE_COMMAND} -E copy
        "${CMAKE_BINARY_DIR}/src/index.*"
        ${CMAKE_SOURCE_DIR}/webport/
        COMMENT "Add files to Github pages")
endif()

target_compile_options(bagelsim PRIVATE -Wall -Wextra -Wpedantic -Wuninitialized -Wno-missing-field-initializers)
target_link_options(bagelsim PRIVATE -Wall -Wextra -Wpedantic -Wuninitialized)

set(CMAKE_CXX_FLAGS_DEBUG  "${CMAKE_CXX_FLAGS_DEBUG} -g -pthread ")
set(CMAKE_C_FLAGS_DEBUG  "${CMAKE_C_FLAGS_DEBUG} -g -pthread ")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -pthread ")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -pthread ")
set(CMAKE_CXX_FLAGS_DISTRIBUTION "${CMAKE_CXX_FLAGS_DISTRIBUTION} -O3 -pthread ")
set(CMAKE_CXX_FLAGS_DISTRIBUTION  "${CMAKE_CXX_FLAGS_DISTRIBUTION} -O3 -pthread ")

include(FetchContent)

FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
)
FetchContent_MakeAvailable(raylib)

FetchContent_Declare(
    jolt 
    SOURCE_SUBDIR Build
    URL https://github.com/jrouwe/JoltPhysics/archive/refs/tags/v5.5.0.tar.gz
)
FetchContent_MakeAvailable(jolt)

FetchContent_Declare(
    eigen
    URL https://gitlab.com/libeigen/eigen/-/archive/5.0.1/eigen-5.0.1.tar.gz
)

FetchContent_MakeAvailable(eigen)

FetchContent_Declare(
    raylib-nuklear
    URL https://github.com/RobLoach/raylib-nuklear/archive/refs/tags/v5.5.1.tar.gz
)

FetchContent_MakeAvailable(raylib-nuklear)

include(sqlite.cmake)


target_link_libraries(bagelsim PUBLIC raylib glfw Jolt Eigen3::Eigen raylib_nuklear SQLite3)

target_compile_definitions(bagelsim PRIVATE GIT_HASH="${GIT_HASH}" NK_BUTTON_TRIGGER_ON_RELEASE)

add_custom_command(TARGET bagelsim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
    COMMENT "Add clangd to the base directory")

install(TARGETS bagelsim DESTINATION bin)
