# cavern_game: the rules - the world and its rooms, the hero (GameLib's
# platformer controller with the abilities, a slash with frame data, the
# shot and the ground pound), enemies, guardians, items, doors, saves'
# contents and the demo recording. Depends only on GameLib::core and
# GameLib::ai: no window, GPU, audio or clock, deterministic for the
# per-tick input.
add_library(cavern_game STATIC
    bosses.cpp
    demo.cpp
    enemies.cpp
    hero.cpp
    rules.cpp
    session.cpp
    world.cpp
    world_core.cpp
    world_crystal.cpp
    world_flooded.cpp
    world_fungal.cpp
    world_ruins.cpp
    world_text.cpp)
target_include_directories(cavern_game PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(cavern_game PUBLIC GameLib::core GameLib::ai)
gamelib_target_warnings(cavern_game)
