include_guard(GLOBAL)

set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)

set(ICONIQUEST_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(ICONIQUEST_DEPS_PREFIX "${CMAKE_BINARY_DIR}/_deps/install")
file(MAKE_DIRECTORY "${ICONIQUEST_DEPS_PREFIX}")
list(PREPEND CMAKE_PREFIX_PATH "${ICONIQUEST_DEPS_PREFIX}")

function(add_local_submodule_if_present submodule_name submodule_path)
    if(EXISTS "${ICONIQUEST_LIB_DIR}/${submodule_path}/CMakeLists.txt")
        add_subdirectory("${ICONIQUEST_LIB_DIR}/${submodule_path}" "${CMAKE_BINARY_DIR}/_deps/${submodule_name}")
    endif()
endfunction()

# Preferred dependency layout: repo-local git submodules under lib/
# This allows the project to build from submodule checkouts when present,
# while still falling back to installed packages for local dev and CI.
add_local_submodule_if_present(SDL3 "SDL3")
add_local_submodule_if_present(SDL3_image "SDL_image")
add_local_submodule_if_present(SDL3_mixer "SDL_mixer")
add_local_submodule_if_present(pugixml "pugixml")

if(NOT TARGET SDL3::SDL3)
    find_package(SDL3 CONFIG REQUIRED)
endif()
if(NOT TARGET SDL3_image::SDL3_image)
    find_package(SDL3_image CONFIG REQUIRED)
endif()
if(NOT TARGET SDL3_mixer::SDL3_mixer)
    find_package(SDL3_mixer CONFIG REQUIRED)
endif()
if(NOT TARGET pugixml::pugixml)
    find_package(pugixml CONFIG REQUIRED)
endif()
