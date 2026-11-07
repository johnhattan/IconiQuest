cmake_minimum_required(VERSION 3.25)
project(IconiQuest VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Keep third-party dependencies local to the repo instead of relying on Homebrew installs.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib)

set(ICONIQUEST_DEPS_PREFIX "${CMAKE_BINARY_DIR}/_deps/install")
list(PREPEND CMAKE_PREFIX_PATH "${ICONIQUEST_DEPS_PREFIX}")

if(EXISTS "${ICONIQUEST_DEPS_PREFIX}/lib/cmake/SDL3/SDL3Config.cmake")
    set(SDL3_DIR "${ICONIQUEST_DEPS_PREFIX}/lib/cmake/SDL3")
endif()
if(EXISTS "${ICONIQUEST_DEPS_PREFIX}/lib/cmake/SDL3_image/SDL3_imageConfig.cmake")
    set(SDL3_image_DIR "${ICONIQUEST_DEPS_PREFIX}/lib/cmake/SDL3_image")
endif()
if(EXISTS "${ICONIQUEST_DEPS_PREFIX}/lib/cmake/SDL3_mixer/SDL3_mixerConfig.cmake")
    set(SDL3_mixer_DIR "${ICONIQUEST_DEPS_PREFIX}/lib/cmake/SDL3_mixer")
endif()

find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_image CONFIG REQUIRED)
find_package(SDL3_mixer CONFIG REQUIRED)
find_package(pugixml CONFIG REQUIRED)

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
