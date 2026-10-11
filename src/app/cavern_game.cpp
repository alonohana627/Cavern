#include "cavern_game.hpp"

#include "demo.hpp"
#include "palette.hpp"

#include <gamelib/app/settings.hpp>
#include <gamelib/core/timers.hpp>
#include <gamelib/draw/vector_text.hpp>
#include <gamelib/fx/juice.hpp>
#include <gamelib/ui/menu.hpp>
#include <gamelib/ui/panel.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <string_view>
#include <utility>

namespace cavern::app {

namespace {

// helpers

using gamelib::app::ArcadeAction;
using gamelib::app::ArcadeScreen;
using gamelib::core::Rect;
using gamelib::core::Vec2;
using gamelib::platform::Key;
using gamelib::ui::MenuInput;

constexpr int kMapCommand = 0;     // the pause menu's MAP
constexpr int kControlsCommand = 7;
constexpr float kEndingInputDelay = 4.0f;

gamelib::ui::PanelStyle _panelStyle()
{
    return {.fill = srgb(0x0A0E16, 0.94f), .border = srgb(0x3E5A6A), .borderWidth = 3.0f, .cornerRadius = 16.0f};
}

gamelib::ui::WorldMap _buildWorldMap()
{
    gamelib::ui::WorldMap map;
    for (const game::RoomInfo& r : game::world().rooms()) {
        map.rooms.push_back({.corner = r.def->corner, .size = r.def->size, .color = areaMapColor(r.def->area)});
    }
    for (const game::Passage& p : game::world().passages()) {
        map.links.push_back({.from = p.from, .to = p.to});
    }
    return map;
}

std::string _itemTitle(const game::Event& e)
{
    switch (static_cast<game::ItemKind>(e.value)) {
    case game::ItemKind::Health: return "VITALITY VESSEL";
    case game::ItemKind::Ammo: return "SPARK SHARD";
    case game::ItemKind::Key: return "";
    case game::ItemKind::Ability: return std::string{game::abilityName(e.ability)};
    }
    return "";
}

} // namespace

CavernGame::CavernGame(gamelib::app::App& app, ProfileFile profile, const bool demo)
    : m_profile{std::move(profile)}
    , m_menus{"CAVERN", m_profile.value().settings, {}, {.music = true, .vsync = true, .controls = kControlsCommand},
              [](gamelib::ui::Menu& paused) { paused.action("MAP", kMapCommand); }}
    , m_sfx{app.audio(), sfxRecipe}
    , m_music{app.audio(), [](const Tune t) { return tune(t); }}
    , m_shapes{Shapes::registerWith(app.renderer())}
    , m_canvas{app.canvas()}
    , m_view{app.canvas()}
    , m_dialogue{{260.0f, 720.0f, 1400.0f, 290.0f},
                 {.panel = _panelStyle(), .pixelSize = 5.0f, .lineGap = 4.0f, .padding = 36.0f, .text = kText,
                  .speaker = kGold, .marker = kGold, .charsPerSecond = 60.0f}}
    , m_worldMap{_buildWorldMap()}
    , m_dev{gamelib::app::readDevStart("CAVERN")}
    , m_replay{m_dev.playback<game::Input>(game::inputCodec().decoder())}
    , m_attract{[](std::uint64_t) {
                    return DemoRun{.session = game::Session{game::Progress::fresh()},
                                   .playback = gamelib::core::Playback<game::Input>{game::demoRecording()}};
                },
                [](DemoRun& run, float) {
                    if (const std::optional<game::Input> input = run.playback.next()) {
                        run.session.tick(*input);
                    }
                    (void)run.session.events().take();
                },
                [](const DemoRun& run) {
                    return run.playback.done() || run.session.phase() == game::Phase::Dead;
                },
                1}
    , m_demoView{app.canvas()}
    , m_demoMode{demo}
{
    m_menus.setSounds({.moved = gamelib::app::MenuSound{m_sfx.id(Sfx::MenuMove)},
                       .changed = gamelib::app::MenuSound{m_sfx.id(Sfx::MenuMove)},
                       .selected = gamelib::app::MenuSound{m_sfx.id(Sfx::MenuSelect)},
                       .back = gamelib::app::MenuSound{m_sfx.id(Sfx::MenuBack)}});
    m_menus.hostControls(prefs().actions);
    m_profile.save(); // a missing file is written with the defaults, so it can be found and edited
    gamelib::app::applySettings(app, m_profile.value().settings);
    m_demoView.snap(m_attract.sim().session);
    m_music.play(app.audio(), Tune::Title, 0.6f);
}

CavernGame::~CavernGame()
{
    save();
}

void CavernGame::save()
{
    m_profile.save();
}

std::string CavernGame::keyText(const std::string_view text) const
{
    return gamelib::platform::withKeys(text, prefs().actions);
}

void CavernGame::onFocus(gamelib::app::App& app, const bool focused)
{
    if (!m_demoMode && m_mode == Mode::Play) {
        m_menus.onFocus(app, focused, m_time);
    }
}

void CavernGame::onKey(gamelib::app::App& app, const gamelib::platform::KeyEvent& event)
{
    if (m_demoMode) {
        if (event.pressed && event.key != Key::F11) {
            m_demoMode = false; // any key leaves the full-screen demo for the title
            app.input().releaseAll();
        }
        return;
    }
    const auto input = gamelib::ui::menuInput(prefs().actions, event, menuBindings());
    if (m_mode == Mode::Ending) {
        if (event.pressed && m_endingTime > kEndingInputDelay && input
            && (*input == MenuInput::Confirm || *input == MenuInput::Back)) {
            toTitle(app);
        }
        return;
    }
    if (m_overlay == Overlay::Reading) {
        if (input && event.pressed) {
            const gamelib::ui::DialogueResult r = m_dialogue.handle(*input == MenuInput::Back ? MenuInput::Confirm : *input);
            if (r.event == gamelib::ui::DialogueEvent::Finished || *input == MenuInput::Back) {
                m_dialogue.close();
                m_overlay = Overlay::None;
                app.input().releaseAll();
            }
        }
        return;
    }
    if (m_overlay == Overlay::Map) {
        const bool mapKey = prefs().actions.actionOn(event.key) == actionId(Action::Map);
        if (event.pressed && (mapKey || (input && (*input == MenuInput::Back || *input == MenuInput::Confirm)))) {
            m_overlay = Overlay::None;
            m_sfx.queue(app.sounds(), Sfx::MenuBack);
            app.input().releaseAll();
        }
        return;
    }
    if (m_mode == Mode::Slots) {
        if (input && event.pressed) {
            slotsCommand(app, *input);
        } else if (event.pressed && event.key == Key::Delete) {
            slotsCommand(app, MenuInput::Left);
        }
        return;
    }
    const gamelib::app::ArcadeResult result = m_menus.onKey(app, event, m_time);
    handleResult(app, result);
    if (event.pressed && result.from == ArcadeScreen::Playing && m_menus.is(ArcadeScreen::Playing) && m_mode == Mode::Play
        && prefs().actions.actionOn(event.key) == actionId(Action::Map)) {
        openMap(app);
    }
}

void CavernGame::onGamepad(gamelib::app::App& app, const gamelib::platform::GamepadEvent& event)
{
    if (m_demoMode) {
        if (event.pressed) {
            m_demoMode = false;
            app.input().releaseAll();
        }
        return;
    }
    const auto input = gamelib::ui::menuInput(prefs().actions, event, menuBindings());
    if (m_mode == Mode::Ending) {
        if (event.pressed && m_endingTime > kEndingInputDelay && input) {
            toTitle(app);
        }
        return;
    }
    if (m_overlay == Overlay::Reading) {
        if (input && event.pressed) {
            const gamelib::ui::DialogueResult r = m_dialogue.handle(*input == MenuInput::Back ? MenuInput::Confirm : *input);
            if (r.event == gamelib::ui::DialogueEvent::Finished || *input == MenuInput::Back) {
                m_dialogue.close();
                m_overlay = Overlay::None;
                app.input().releaseAll();
            }
        }
        return;
    }
    if (m_overlay == Overlay::Map) {
        const bool mapButton = prefs().actions.actionOn(event.button) == actionId(Action::Map);
        if (event.pressed && (mapButton || (input && (*input == MenuInput::Back || *input == MenuInput::Confirm)))) {
            m_overlay = Overlay::None;
            m_sfx.queue(app.sounds(), Sfx::MenuBack);
            app.input().releaseAll();
        }
        return;
    }
    if (m_mode == Mode::Slots) {
        if (input && event.pressed) {
            slotsCommand(app, *input);
        }
        return;
    }
    const gamelib::app::ArcadeResult result = m_menus.onGamepad(app, event, m_time);
    handleResult(app, result);
    if (event.pressed && result.from == ArcadeScreen::Playing && m_menus.is(ArcadeScreen::Playing) && m_mode == Mode::Play
        && prefs().actions.actionOn(event.button) == actionId(Action::Map)) {
        openMap(app);
    }
}

void CavernGame::handleResult(gamelib::app::App& app, const gamelib::app::ArcadeResult& result)
{
    if (result.changed) {
        save();
    }
    switch (result.action) {
    case ArcadeAction::Start:
        if (result.from == ArcadeScreen::Paused) {
            // RESTART: back to the last shrine, as saved.
            startSlot(app, m_slot);
            break;
        }
        m_mode = Mode::Slots;
        m_slotCursor = prefs().lastSlot;
        m_confirmErase = -1;
        app.input().releaseAll();
        break;
    case ArcadeAction::ToTitle: toTitle(app); break;
    case ArcadeAction::Quit:
        save();
        app.requestQuit();
        break;
    case ArcadeAction::Game:
        if (result.command == kMapCommand && m_mode == Mode::Play && m_session) {
            openMap(app);
        }
        break;
    case ArcadeAction::Resume:
    case ArcadeAction::Controls:
    case ArcadeAction::None: break;
    }
}

void CavernGame::slotsCommand(gamelib::app::App& app, const MenuInput input)
{
    switch (input) {
    case MenuInput::Up:
        m_slotCursor = (m_slotCursor + kSlotCount) % (kSlotCount + 1);
        m_confirmErase = -1;
        m_sfx.queue(app.sounds(), Sfx::MenuMove);
        break;
    case MenuInput::Down:
        m_slotCursor = (m_slotCursor + 1) % (kSlotCount + 1);
        m_confirmErase = -1;
        m_sfx.queue(app.sounds(), Sfx::MenuMove);
        break;
    case MenuInput::Left:
    case MenuInput::Right:
        // Erase a slot: asked once, done on the second press.
        if (m_slotCursor < kSlotCount && m_saves.view(m_slotCursor).exists) {
            if (m_confirmErase == m_slotCursor) {
                m_saves.erase(m_slotCursor);
                m_confirmErase = -1;
                m_sfx.queue(app.sounds(), Sfx::Break);
            } else {
                m_confirmErase = m_slotCursor;
                m_sfx.queue(app.sounds(), Sfx::DoorLocked);
            }
        }
        break;
    case MenuInput::Confirm:
        if (m_slotCursor == kSlotCount) {
            m_sfx.queue(app.sounds(), Sfx::MenuBack);
            m_mode = Mode::Title;
            m_menus.go(ArcadeScreen::Title, m_time);
        } else {
            m_sfx.queue(app.sounds(), Sfx::MenuSelect);
            startSlot(app, m_slotCursor);
        }
        break;
    case MenuInput::Back:
        m_sfx.queue(app.sounds(), Sfx::MenuBack);
        m_mode = Mode::Title;
        m_menus.go(ArcadeScreen::Title, m_time);
        break;
    }
}

void CavernGame::startSlot(gamelib::app::App& app, const int slot)
{
    m_slot = slot;
    prefs().lastSlot = slot;
    save();
    std::optional<game::Progress> progress = m_saves.load(slot);
    m_session.emplace(progress ? std::move(*progress) : game::Progress::fresh());
    if (m_dev.level) {
        m_session->grantAll();
        m_session->debugStartIn(*m_dev.level);
    }
    (void)m_session->events().take();
    m_view.snap(*m_session);
    m_effects.clear();
    m_banners.clear();
    m_transition.cancel();
    m_overlay = Overlay::None;
    m_mode = Mode::Play;
    m_menus.go(ArcadeScreen::Playing, m_time);
    app.input().releaseAll();
    m_banners.add(std::string{game::areaName(m_session->area())}, std::string{m_session->room().def->name},
                  areaMapColor(m_session->area()).scaled(1.4f), 3.0f);
    m_music.play(app.audio(), areaTune(m_session->area()), 1.0f);
    // Developer aids: CAVERN_SCENE=ability0..3 shows an "ability get" scene, =ending the ending.
    if (const char* scene = std::getenv("CAVERN_SCENE")) {
        const std::string_view name{scene};
        if (name.starts_with("ability") && name.size() == 8) {
            m_session->debugAbilityScene(static_cast<game::Ability>(std::clamp(name[7] - '0', 0, game::kAbilityCount - 1)));
            m_banners.clear();
        } else if (name == "map") {
            m_session->debugRevealAll();
        } else if (name == "ending") {
            startEnding(app);
        }
    }
}

void CavernGame::toTitle(gamelib::app::App& app)
{
    m_session.reset();
    m_transition.cancel();
    m_overlay = Overlay::None;
    m_mode = Mode::Title;
    m_menus.go(ArcadeScreen::Title, m_time);
    save();
    m_music.play(app.audio(), Tune::Title, 0.8f);
    app.input().releaseAll();
}

void CavernGame::openMap(gamelib::app::App& app)
{
    m_overlay = Overlay::Map;
    m_menus.go(ArcadeScreen::Playing, m_time);
    m_sfx.queue(app.sounds(), Sfx::Map);
    app.input().releaseAll();
}

void CavernGame::openSign(gamelib::app::App& app, const int station)
{
    const game::Station& s = game::world().stations()[static_cast<std::size_t>(station)];
    m_dialogue.open(s.kind == game::StationKind::Tablet ? "ANCIENT TABLET" : "SIGN", keyText(s.text));
    m_overlay = Overlay::Reading;
    m_sfx.queue(app.sounds(), Sfx::Read);
    app.input().releaseAll();
}

void CavernGame::startEnding(gamelib::app::App& app)
{
    const game::Progress& p = m_session->progress();
    m_endTicks = p.ticks;
    m_endCompletion = p.completion();
    m_endDeaths = p.deaths;
    m_saves.save(m_slot, p);
    gamelib::app::Bests& bests = m_profile.value().bests;
    m_newBest = bests.offer(kBestTime, m_endTicks, gamelib::app::Bests::Better::Lower);
    (void)bests.offer(kBestCompletion, m_endCompletion);
    save();
    m_transition.start({.kind = gamelib::ui::TransitionKind::Fade, .color = srgb(0xFFF4E0), .coverSeconds = 1.2f,
                        .holdSeconds = 0.4f, .uncoverSeconds = 1.5f},
                       [this, &app] {
                           m_mode = Mode::Ending;
                           m_endingTime = 0.0f;
                           m_session.reset();
                           m_music.play(app.audio(), Tune::Ending, 1.0f);
                       });
}

void CavernGame::tick(gamelib::app::App& app)
{
    if (m_demoMode || m_mode == Mode::Title || m_mode == Mode::Slots) {
        m_attract.tick(app.tickSeconds());
        return;
    }
    if (m_mode != Mode::Play || !m_session || !m_menus.is(ArcadeScreen::Playing) || m_overlay != Overlay::None
        || m_transition.active()) {
        return;
    }
    playTick(app);
}

void CavernGame::playTick(gamelib::app::App& app)
{
    game::Session& s = *m_session;
    game::Input input = readInput(prefs().actions, app.input());
    if (!m_replay.done()) {
        input = m_replay.next().value_or(game::Input{});
    }
    if (input.interact && s.phase() == game::Phase::Playing && s.hero().body.grounded) {
        if (const int st = s.stationInReach(); st >= 0) {
            const game::StationKind kind = game::world().stations()[static_cast<std::size_t>(st)].kind;
            if (kind == game::StationKind::Sign || kind == game::StationKind::Tablet) {
                openSign(app, st);
                return;
            }
        }
    }
    s.tick(input);
    for (const game::Event& e : s.events().take()) {
        react(app, e);
    }
    if (s.wantsSave()) {
        m_saves.save(m_slot, s.progress());
    }
    if (s.phase() == game::Phase::Dead && !m_transition.active()) {
        m_transition.start({.kind = gamelib::ui::TransitionKind::Fade, .color = kInk, .coverSeconds = 0.6f, .holdSeconds = 0.3f,
                            .uncoverSeconds = 0.6f},
                           [this] {
                               m_session->respawn();
                               (void)m_session->events().take();
                               m_view.snap(*m_session);
                               m_effects.clear();
                           });
    } else if (s.phase() == game::Phase::Escaped && !m_transition.active()) {
        startEnding(app);
    }
}

void CavernGame::react(gamelib::app::App& app, const game::Event& e)
{
    using game::EventKind;
    m_effects.react(e, *m_session);
    gamelib::audio::SoundQueue& q = app.sounds();
    switch (e.kind) {
    case EventKind::Jump: m_sfx.queue(q, Sfx::Jump); break;
    case EventKind::AirJump: m_sfx.queue(q, Sfx::AirJump); break;
    case EventKind::WallJump: m_sfx.queue(q, Sfx::WallJump); break;
    case EventKind::Dash: m_sfx.queue(q, Sfx::Dash); break;
    case EventKind::Land: m_sfx.queue(q, Sfx::Land); break;
    case EventKind::Splash: m_sfx.queue(q, Sfx::Splash, e.value == 2 ? 0.5f : 1.0f); break;
    case EventKind::Slash: m_sfx.queue(q, Sfx::Slash, 1.0f, e.value < 0 ? 1.15f : (e.value > 0 ? 0.9f : 1.0f)); break;
    case EventKind::SlashHit: m_sfx.queue(q, Sfx::SlashHit); break;
    case EventKind::Pogo: m_sfx.queue(q, Sfx::Pogo); break;
    case EventKind::Shoot: m_sfx.queue(q, Sfx::Shoot); break;
    case EventKind::NoAmmo: m_sfx.queue(q, Sfx::NoAmmo); break;
    case EventKind::Pound: m_sfx.queue(q, Sfx::PoundDive); break;
    case EventKind::PoundLand:
        m_sfx.queue(q, Sfx::PoundLand);
        app.hitStop(0.05f);
        break;
    case EventKind::Hurt:
    case EventKind::HazardHit:
        m_sfx.queue(q, Sfx::Hurt);
        app.hitStop(0.08f);
        break;
    case EventKind::Death: m_sfx.queue(q, Sfx::Death); break;
    case EventKind::EnemyHit: m_sfx.queue(q, Sfx::EnemyHit); break;
    case EventKind::EnemyDown:
        m_sfx.queue(q, Sfx::EnemyDown, e.value > 0 ? 0.8f : 1.0f);
        if (e.value == 0) {
            app.hitStop(0.03f);
        }
        break;
    case EventKind::EnemyShot: m_sfx.queue(q, Sfx::EnemyShot, 0.7f); break;
    case EventKind::Bomb: m_sfx.queue(q, Sfx::Bomb); break;
    case EventKind::Deflect: m_sfx.queue(q, Sfx::Deflect); break;
    case EventKind::DoorOpen: m_sfx.queue(q, Sfx::DoorOpen); break;
    case EventKind::DoorLocked: m_sfx.queue(q, Sfx::DoorLocked); break;
    case EventKind::Break: m_sfx.queue(q, Sfx::Break); break;
    case EventKind::Shake: m_sfx.queue(q, Sfx::Rumble, 0.7f); break;
    case EventKind::Item: {
        const auto kind = static_cast<game::ItemKind>(e.value);
        if (kind == game::ItemKind::Ability) {
            m_sfx.queue(q, Sfx::AbilityGet);
            m_banners.clear();
            m_music.stop(app.audio(), 0.3f);
        } else {
            m_sfx.queue(q, Sfx::Item);
            std::string title = _itemTitle(e);
            std::string line;
            if (kind == game::ItemKind::Health) {
                line = "YOUR LIFE GROWS BY ONE MASK";
            } else if (kind == game::ItemKind::Ammo) {
                line = "YOU CARRY TWO MORE BOLTS";
            } else {
                for (const game::Item& it : game::world().items()) {
                    if (it.kind == game::ItemKind::Key && m_session->map().cellCenter(it.cell) == e.at) {
                        title = std::string{game::keyName(it.key)};
                    }
                }
                line = "IT OPENS THE DOORS OF ITS COLOR";
            }
            m_banners.add(title, line, kGold, 3.2f);
        }
        break;
    }
    case EventKind::Drop: m_sfx.queue(q, Sfx::Drop); break;
    case EventKind::Saved:
        m_sfx.queue(q, Sfx::Save);
        m_banners.add("REST", "YOUR JOURNEY IS WRITTEN DOWN", areaMapColor(m_session->area()).scaled(1.4f), 2.4f);
        break;
    case EventKind::Mapped:
        m_sfx.queue(q, Sfx::Map);
        m_banners.add("MAP", std::string{game::areaName(static_cast<game::Area>(e.value))} + " CHARTED",
                      areaMapColor(m_session->area()).scaled(1.4f), 2.4f);
        break;
    case EventKind::AreaEnter:
        m_banners.add(std::string{game::areaName(static_cast<game::Area>(e.value))}, std::string{m_session->room().def->name},
                      areaMapColor(static_cast<game::Area>(e.value)).scaled(1.4f), 3.0f);
        break;
    case EventKind::BossWake:
        m_sfx.queue(q, Sfx::BossWake);
        m_banners.add(std::string{game::bossName(e.boss)}, "", kDanger, 2.6f);
        break;
    case EventKind::BossHit: m_sfx.queue(q, Sfx::BossHit); break;
    case EventKind::BossPhase: m_sfx.queue(q, Sfx::BossWake, 0.6f, 1.3f); break;
    case EventKind::BossDown: m_sfx.queue(q, Sfx::BossDown); break;
    case EventKind::BossLand:
    case EventKind::Strike: m_sfx.queue(q, Sfx::Strike); break;
    case EventKind::EscapeStart:
        m_sfx.queue(q, Sfx::Alarm);
        m_banners.add("THE CORE COLLAPSES", "ESCAPE TO THE SURFACE!", kDanger, 3.5f);
        break;
    case EventKind::Escaped:
    case EventKind::Scroll: break;
    }
}

void CavernGame::updateMusic(gamelib::app::App& app)
{
    if (m_mode == Mode::Title || m_mode == Mode::Slots) {
        m_music.play(app.audio(), Tune::Title, 0.8f);
        return;
    }
    if (m_mode != Mode::Play || !m_session) {
        return;
    }
    const game::Session& s = *m_session;
    if (s.phase() == game::Phase::AbilityGet || s.phase() == game::Phase::BossDown || s.phase() == game::Phase::Dying
        || s.phase() == game::Phase::Dead) {
        return;
    }
    if (s.escapeTicksLeft()) {
        m_music.play(app.audio(), Tune::Escape, 0.3f);
    } else if (const game::Boss* boss = s.boss()) {
        m_music.play(app.audio(), boss->kind == game::BossKind::Heart ? Tune::Heart : Tune::Boss, 0.4f);
    } else {
        m_music.play(app.audio(), areaTune(s.area()), 2.0f);
    }
}

void CavernGame::frame(gamelib::app::App& app, const gamelib::app::FrameInfo& info)
{
    m_time = info.time;
    m_transition.update(info.seconds);
    m_banners.update(info.seconds);
    if (m_overlay == Overlay::Reading) {
        m_dialogue.update(info.seconds);
        if (m_dialogue.revealedLastUpdate() > 0) {
            m_sfx.queue(app.sounds(), Sfx::Type, 0.5f);
        }
    }
    if (m_mode == Mode::Ending) {
        m_endingTime += info.seconds;
    }
    if (m_mode == Mode::Play && m_session) {
        const float alpha = m_session->phase() == game::Phase::Playing ? info.alpha : 1.0f;
        m_view.follow(*m_session, alpha, info.seconds);
        m_effects.update(info.seconds, *m_session, m_view.visible());
    } else {
        const game::Session& demo = m_attract.sim().session;
        if (demo.ticks() <= 1) {
            m_demoView.snap(demo);
            m_demoEffects.clear();
        }
        m_demoView.follow(demo, info.alpha, info.seconds);
        m_demoEffects.update(info.seconds, demo, m_demoView.visible());
    }
    updateMusic(app);
}

} // namespace cavern::app
