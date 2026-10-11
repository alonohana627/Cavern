# cavern_app: everything around the rules - the GameLib main-loop glue, the
# profile and the save slots, the world view and its shaders, the HUD, map
# and screens, sounds, music and effects. Menus, settings, the controls
# screen, sound banks, transitions, dialogue boxes, the world map and the
# attract mode come from GameLib (app, ui, audio, fx, ai, storage).
add_library(cavern_app STATIC
    actions.cpp
    cavern_game.cpp
    effects.cpp
    palette.cpp
    profile.cpp
    saves.cpp
    screens.cpp
    sounds.cpp
    view.cpp)
target_include_directories(cavern_app PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(cavern_app PUBLIC cavern_game GameLib::app GameLib::ui GameLib::fx GameLib::ai GameLib::storage)
gamelib_target_warnings(cavern_app)
# Cavern's own procedural shapes, embedded as SPIR-V (see view.hpp, Shapes).
gamelib_add_shaders(cavern_app SHADERS
    ${PROJECT_SOURCE_DIR}/shaders/backdrop.frag
    ${PROJECT_SOURCE_DIR}/shaders/boss.frag
    ${PROJECT_SOURCE_DIR}/shaders/enemy.frag
    ${PROJECT_SOURCE_DIR}/shaders/hero.frag
    ${PROJECT_SOURCE_DIR}/shaders/item.frag
    ${PROJECT_SOURCE_DIR}/shaders/shot.frag
    ${PROJECT_SOURCE_DIR}/shaders/tile.frag)
