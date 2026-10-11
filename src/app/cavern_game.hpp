#pragma once

// Cavern on GameLib's main loop: the title (the recorded demo playing behind
// the menu), the three save slots, the world with its HUD and minimap,
// signs and tablets in a dialogue box, the map screen, the "ability get"
// scene, death and the return to the last shrine, the escape and the ending
// with the run's time and completion. The rules live in src/game; this is
// input, presentation, sound and saves.

#include "actions.hpp"
#include "effects.hpp"
#include "profile.hpp"
#include "saves.hpp"
#include "sounds.hpp"
#include "view.hpp"

#include "session.hpp"

#include <gamelib/ai/attract.hpp>
#include <gamelib/app/app.hpp>
#include <gamelib/app/arcade_menus.hpp>
#include <gamelib/app/dev_start.hpp>
#include <gamelib/core/replay.hpp>
#include <gamelib/fx/banners.hpp>
#include <gamelib/ui/dialogue_box.hpp>
#include <gamelib/ui/hud.hpp>
#include <gamelib/ui/transition.hpp>
#include <gamelib/ui/world_map.hpp>

#include <optional>
#include <string>

namespace cavern::app {

class CavernGame final : public gamelib::app::Game {
public:
    // demo: the demo plays full screen without the menu (--demo).
    CavernGame(gamelib::app::App& app, ProfileFile profile, bool demo);
    ~CavernGame() override;
    CavernGame(const CavernGame&) = delete("owns the menus that point into its profile");
    CavernGame& operator=(const CavernGame&) = delete("owns the menus that point into its profile");

    void onFocus(gamelib::app::App& app, bool focused) override;
    void onKey(gamelib::app::App& app, const gamelib::platform::KeyEvent& event) override;
    void onGamepad(gamelib::app::App& app, const gamelib::platform::GamepadEvent& event) override;
    void tick(gamelib::app::App& app) override;
    void frame(gamelib::app::App& app, const gamelib::app::FrameInfo& info) override;
    void draw(gamelib::app::App& app, gamelib::draw::DrawList& list, const gamelib::app::FrameInfo& info) override;

private:
    enum class Mode { Title, Slots, Play, Ending };
    enum class Overlay { None, Map, Reading };

    // The demo behind the title: a fresh run fed the recorded input.
    struct DemoRun {
        game::Session session;
        gamelib::core::Playback<game::Input> playback;
    };

    void handleResult(gamelib::app::App& app, const gamelib::app::ArcadeResult& result);
    void slotsCommand(gamelib::app::App& app, gamelib::ui::MenuInput input);
    void startSlot(gamelib::app::App& app, int slot);
    void playTick(gamelib::app::App& app);
    void react(gamelib::app::App& app, const game::Event& event);
    void openMap(gamelib::app::App& app);
    void openSign(gamelib::app::App& app, int station);
    void toTitle(gamelib::app::App& app);
    void startEnding(gamelib::app::App& app);
    void save();
    void updateMusic(gamelib::app::App& app);
    [[nodiscard]] Prefs& prefs() { return m_profile.value().game; }
    [[nodiscard]] const Prefs& prefs() const { return m_profile.value().game; }
    [[nodiscard]] std::string keyText(std::string_view text) const;

    void drawHud(gamelib::draw::DrawList& list, const game::Session& session, float time) const;
    void drawMinimap(gamelib::draw::DrawList& list, const game::Session& session, gamelib::core::Rect area, bool full,
                     float time) const;
    void drawMapScreen(gamelib::draw::DrawList& list, const game::Session& session, float time) const;
    void drawSlots(gamelib::draw::DrawList& list, float time) const;
    void drawAbilityScene(gamelib::draw::DrawList& list, const game::Session& session, float time) const;
    void drawEnding(gamelib::draw::DrawList& list, float time) const;
    void drawPrompts(gamelib::draw::DrawList& list, const game::Session& session) const;

    ProfileFile m_profile; // the menus edit its settings in place: declared first
    gamelib::app::ArcadeMenus m_menus;
    Sounds m_sfx;
    Music m_music;
    Shapes m_shapes;
    Saves m_saves;
    gamelib::core::Size m_canvas;
    Mode m_mode = Mode::Title;
    Overlay m_overlay = Overlay::None;
    int m_slotCursor = 0;
    int m_confirmErase = -1;
    int m_slot = 0;

    std::optional<game::Session> m_session;
    WorldView m_view;
    Effects m_effects;
    gamelib::ui::Transition m_transition;
    gamelib::ui::DialogueBox m_dialogue;
    gamelib::ui::WorldMap m_worldMap;
    gamelib::fx::Banners m_banners;
    gamelib::ui::BarTrail m_bossTrail;
    float m_endingTime = 0.0f;
    std::int64_t m_endTicks = 0;
    int m_endCompletion = 0;
    int m_endDeaths = 0;
    bool m_newBest = false;
    gamelib::app::DevStart m_dev;
    gamelib::core::Playback<game::Input> m_replay;

    gamelib::ai::AttractMode<DemoRun> m_attract;
    WorldView m_demoView;
    Effects m_demoEffects;
    bool m_demoMode = false;
    float m_time = 0.0f;
};

} // namespace cavern::app
