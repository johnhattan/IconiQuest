cmake_minimum_required(VERSION 3.25)
project(IconiQuest VERSION 1.0.0 LANGUAGES CXX C)

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

# SDL3_gfx sources
set(SDL3_GFX_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/SDL3_gfx")
add_library(SDL3_gfx STATIC
    ${SDL3_GFX_DIR}/SDL3_gfxPrimitives.c
    ${SDL3_GFX_DIR}/SDL3_rotozoom.c
)
target_include_directories(SDL3_gfx PUBLIC ${SDL3_GFX_DIR})
target_link_libraries(SDL3_gfx PUBLIC SDL3::SDL3)

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
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/tweeny/include
)

target_link_libraries(iconiquest PRIVATE
    SDL3::SDL3
    SDL3_image::SDL3_image
    SDL3_mixer::SDL3_mixer
    SDL3_ttf::SDL3_ttf
    pugixml::pugixml
    SDL3_gfx
)

if(APPLE)
    target_link_libraries(iconiquest PRIVATE
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework CoreFoundation"
    )
endif()
