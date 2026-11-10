include_guard(GLOBAL)

set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)

function(require_local_submodule submodule_name submodule_path)
    set(submodule_dir "${CMAKE_CURRENT_SOURCE_DIR}/${submodule_path}")
    if(NOT EXISTS "${submodule_dir}/CMakeLists.txt")
        message(FATAL_ERROR
            "Missing required git submodule '${submodule_path}'.\n"
            "Run: git submodule update --init --recursive"
        )
    endif()
    add_subdirectory("${submodule_dir}" "${CMAKE_BINARY_DIR}/_deps/${submodule_name}")
endfunction()

# All third-party dependencies are expected to live under lib/ as git submodules.
# Building from downloaded sources is intentionally disabled here.
require_local_submodule(SDL3 "SDL3")
require_local_submodule(SDL3_image "SDL_image")
require_local_submodule(SDL3_mixer "SDL_mixer")
require_local_submodule(SDL3_ttf "SDL_ttf")
require_local_submodule(pugixml "pugixml")

if(NOT TARGET SDL3::SDL3)
    message(FATAL_ERROR "SDL3 submodule is present but did not define SDL3::SDL3.")
endif()
if(NOT TARGET SDL3_image::SDL3_image)
    message(FATAL_ERROR "SDL_image submodule is present but did not define SDL3_image::SDL3_image.")
endif()
if(NOT TARGET SDL3_mixer::SDL3_mixer)
    message(FATAL_ERROR "SDL_mixer submodule is present but did not define SDL3_mixer::SDL3_mixer.")
endif()
if(NOT TARGET SDL3_ttf::SDL3_ttf)
    message(FATAL_ERROR "SDL_ttf submodule is present but did not define SDL3_ttf::SDL3_ttf.")
endif()
if(NOT TARGET pugixml::pugixml)
    message(FATAL_ERROR "pugixml submodule is present but did not define pugixml::pugixml.")
endif()
