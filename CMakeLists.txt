# Cavern - a small Metroidvania built on GameLib: one connected world of
# rooms in five areas that opens up as the hero gains abilities; every visual
# a shader, every sound synthesized, a deterministic fixed-tick simulation.
cmake_minimum_required(VERSION 3.28)

# C is enabled too because GameLib's dependencies (GLFW, miniaudio) are C.
project(Cavern VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# --- GameLib -----------------------------------------------------------------
# GameLib is consumed as a library target, never copied: downloaded from
# GitHub at a pinned commit (with its GLFW/miniaudio submodules) into the
# build tree. GameLib keeps no backwards compatibility, so the pin is a full
# commit hash, never a branch or a tag; bump it together with
# docs/GAMELIB_REVISION.md. To develop against a local checkout instead:
#   cmake --preset debug -DFETCHCONTENT_SOURCE_DIR_GAMELIB=<path to GameLib>
# (best kept in CMakeUserPresets.json, which is not committed).
include(FetchContent)
FetchContent_Declare(GameLib
    GIT_REPOSITORY https://github.com/alonohana627/GameLib.git
    GIT_TAG        425aad145b7ea431cd1a945321e1fdb1ea775fb8
    GIT_SHALLOW    FALSE)  # a shallow clone cannot check out an arbitrary commit
FetchContent_MakeAvailable(GameLib)

# Print the GameLib revision being built against, for bug reports and for the
# known-good combination recorded in docs/GAMELIB_REVISION.md.
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${gamelib_SOURCE_DIR}" rev-parse --short HEAD
                    OUTPUT_VARIABLE _gamelib_revision OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    message(STATUS "Cavern: using GameLib ${_gamelib_revision} from ${gamelib_SOURCE_DIR}")
endif()

add_subdirectory(src)

# Smoke test: launch, render 120 frames with the validation layer required,
# exit cleanly. Exit code 77 (no Vulkan GPU) counts as skipped. The profile
# file goes to the build folder, not the player's. Cavern has no other tests.
enable_testing()
gamelib_validation_layer_dir(_layer_dir)
add_test(NAME cavern_smoke COMMAND cavern --hidden --require-validation --frames 120)
set_tests_properties(cavern_smoke PROPERTIES LABELS "gpu;smoke" TIMEOUT 120 SKIP_RETURN_CODE 77
    ENVIRONMENT "CAVERN_DATA_DIR=${CMAKE_BINARY_DIR}/smoke_data")
if(_layer_dir)
    set_tests_properties(cavern_smoke PROPERTIES
        ENVIRONMENT_MODIFICATION "VK_ADD_LAYER_PATH=path_list_prepend:${_layer_dir}")
endif()
