// Cavern - a small Metroidvania on GameLib.
//
// Command line: --demo (the recorded demo plays full screen) plus GameLib's
// self-test switches (--frames N --hidden --require-validation --no-vsync
// --allow-software --resize-at-frame N --minimize-at-frame N
// --focus-loss-at-frame N --keep-running).
#include "cavern_game.hpp"
#include "profile.hpp"
#include "rules.hpp"

#include <gamelib/app/app.hpp>

#include <memory>
#include <utility>

int main(int argc, char** argv)
{
    // Loaded before the window exists, so a saved fullscreen setting is
    // applied while the window is still hidden. CAVERN_DATA_DIR moves the
    // file and the saves (the smoke run keeps its own in the build folder).
    cavern::app::ProfileFile profile = cavern::app::openProfile();
    gamelib::app::AppConfig config{
        .title = "Cavern",
        .windowSize = {1280, 720},
        .canvas = {1920, 1080},
        .tick = std::chrono::nanoseconds{1'000'000'000 / cavern::game::kTickRate},
        .fullscreen = profile.value().settings.fullscreen,
        .f11TogglesFullscreen = false, // F11 goes through ArcadeMenus, so the setting is saved
        .pauseOnFocusLoss = true,      // nothing runs on while the player is away
        .gameFlags = {"--demo"},
    };
    config.renderer.vsync = profile.value().settings.vsync;
    return gamelib::app::runMain(argc, argv, config, [&](gamelib::app::App& app) {
        return std::make_unique<cavern::app::CavernGame>(app, std::move(profile), app.hasFlag("--demo"));
    });
}
