cmake_minimum_required(VERSION 4.0)
set(CMAKE_CXX_STANDARD 23)

project(DriveSimulator)

option(EMSCRIPTEN "Build for the web" OFF)

if (EMSCRIPTEN)
  file(REAL_PATH release RELEASE_PATH)
  file(REAL_PATH shell/index.html SHELL_PATH)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s GL_ENABLE_GET_PROC_ADDRESS=1 --preload-file ${RELEASE_PATH}@/release -sINITIAL_MEMORY=655360000 -sSTACK_SIZE=655360 --shell-file ${SHELL_PATH} -sEXPORTED_RUNTIME_METHODS=requestFullscreen,Browser -sASYNCIFY")
  set(CMAKE_EXECUTABLE_SUFFIX ".html")
  add_compile_definitions(PLATFORM_WEB)
  set(PLATFORM Web)
endif ()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

execute_process(
        COMMAND git log -1 --format=%h
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        OUTPUT_VARIABLE GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )

add_subdirectory(src)
