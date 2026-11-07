cmake_minimum_required(VERSION 3.25)
project(IconiQuest VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Keep third-party dependencies local to the repo as git submodules.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib)

# ImGui sources
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui")
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer3.cpp
)

# Engine sources
file(GLOB_RECURSE ENGINE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(iconiquest
    ${IMGUI_SOURCES}
    ${ENGINE_SOURCES}
)

target_include_directories(iconiquest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_link_libraries(iconiquest PRIVATE
    SDL3::SDL3
    SDL3_image::SDL3_image
    SDL3_mixer::SDL3_mixer
    pugixml::pugixml
)

if(APPLE)
    target_link_libraries(iconiquest PRIVATE
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework CoreFoundation"
    )
endif()
