add_executable(bagelsim
 main.cpp
)

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

target_link_libraries(bagelsim PUBLIC raylib glfw Jolt)

add_custom_command(TARGET bagelsim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
    COMMENT "Add clangd to the base directory")

install(TARGETS bagelsim DESTINATION bin)
