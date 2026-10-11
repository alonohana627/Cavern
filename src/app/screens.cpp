// What CavernGame draws: the world and its HUD (masks, bolts, keys, the
// minimap, a guardian's bar, the escape clock, prompts at stations and
// banners), the map screen, the save slots, the "ability get" scene, the
// ending and the menus.
#include "cavern_game.hpp"

#include "palette.hpp"

#include <gamelib/core/timers.hpp>
#include <gamelib/draw/vector_text.hpp>
#include <gamelib/fx/juice.hpp>
#include <gamelib/ui/binding_list.hpp>
#include <gamelib/ui/menu.hpp>
#include <gamelib/ui/panel.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace cavern::app {

namespace {

// helpers

using gamelib::app::ArcadeScreen;
using gamelib::core::Rect;
using gamelib::core::Vec2;
using gamelib::draw::Align;
namespace layers = gamelib::draw::layers;

constexpr float kPi = std::numbers::pi_v<float>;

gamelib::ui::MenuStyle _menuStyle()
{
    return {.veil = Color{0.0f, 0.0f, 0.02f, 0.6f},
            .panel = srgb(0x0C1018, 0.96f),
            .border = srgb(0x3E5A6A),
            .title = kGold,
            .text = kText,
            .highlight = kGold,
            .selection = srgb(0x1C2A36),
            .dim = kTextDim,
            .titlePixel = 13.0f,
            .itemPixel = 6.0f,
            .glowWidth = 26.0f};
}

gamelib::ui::PanelStyle _panel()
{
    return {.fill = srgb(0x0A0E16, 0.92f), .border = srgb(0x3E5A6A), .borderWidth = 3.0f, .cornerRadius = 16.0f};
}

gamelib::ui::WorldMapStyle _mapStyle(const bool full)
{
    return {.background = full ? srgb(0x05070C, 0.0f) : srgb(0x05070C, 0.75f),
            .room = srgb(0x3F6FB0),
            .mapped = srgb(0x1E2A3A),
            .border = srgb(0xD8E4F0),
            .link = srgb(0xD8E4F0),
            .borderWidth = full ? 2.5f : 2.0f,
            .gap = 0.1f,
            .cornerRadius = 0.22f,
            .linkWidth = 0.16f,
            .maxCell = 76.0f};
}

std::string _clock(const std::int64_t ticks)
{
    return gamelib::core::formatClock(ticks, game::kTickRate, 0);
}

} // namespace

void CavernGame::draw(gamelib::app::App& app, gamelib::draw::DrawList& list, const gamelib::app::FrameInfo& info)
{
    const float time = info.time;
    const Rect canvas{0.0f, 0.0f, static_cast<float>(m_canvas.width), static_cast<float>(m_canvas.height)};
    list.setLayer(layers::Background - 100);
    list.box(canvas, kInk);
    list.setLayer(layers::Entities);
    if (m_mode == Mode::Ending) {
        drawEnding(list, time);
        m_transition.draw(list, m_canvas);
        return;
    }
    if (m_mode == Mode::Play && m_session) {
        const game::Session& s = *m_session;
        const float alpha = s.phase() == game::Phase::Playing && m_overlay == Overlay::None && m_menus.is(ArcadeScreen::Playing)
                                ? info.alpha
                                : 1.0f;
        const Vec2 shake = m_profile.value().settings.screenShake ? m_effects.shake(time) : Vec2{};
        m_view.draw(list, m_shapes, s, m_effects, {.alpha = alpha, .shake = shake, .time = time});
        list.setLayer(layers::Hud);
        drawPrompts(list, s);
        if (m_effects.flash() > 0.0f) {
            const Color c = m_effects.flashColor();
            list.box(canvas, c.withAlpha(c.a * m_effects.flash()));
        }
        drawHud(list, s, time);
        if (s.phase() == game::Phase::AbilityGet) {
            drawAbilityScene(list, s, time);
        }
        if (s.phase() == game::Phase::Hazard) {
            const float k = std::min(1.0f, static_cast<float>(s.phaseTicks()) / 10.0f);
            list.box(canvas, kInk.withAlpha(0.7f * k));
        }
        if (m_overlay == Overlay::Map) {
            drawMapScreen(list, s, time);
        }
        if (m_overlay == Overlay::Reading) {
            gamelib::ui::drawDialogueBox(list, m_dialogue, time);
        }
    } else {
        // The demo behind the title and the save slots.
        const game::Session& demo = m_attract.sim().session;
        m_demoView.draw(list, m_shapes, demo, m_demoEffects, {.alpha = info.alpha, .shake = {}, .time = time});
        list.setLayer(layers::Hud);
        if (m_demoMode) {
            gamelib::draw::drawVectorText(list, "DEMO", 1860.0f, 40.0f,
                                          {.size = 44.0f, .color = kGold.withAlpha(0.6f + 0.4f * std::sin(time * 3.0f)),
                                           .weight = 1.4f, .align = Align::Right});
            return;
        }
        list.box(canvas, kInk.withAlpha(0.35f));
        if (m_mode == Mode::Slots) {
            drawSlots(list, time);
        }
    }
    list.setLayer(layers::Hud + 100);
    if (const gamelib::ui::Menu* menu = m_menus.menu()) {
        if (const gamelib::ui::BindingList* controls = m_menus.controls()) {
            gamelib::ui::drawBindingList(list, *controls, m_canvas, _menuStyle());
        } else {
            std::string subtitle;
            std::string footer;
            if (m_menus.is(ArcadeScreen::Title)) {
                subtitle = "A JOURNEY BENEATH THE RUINS";
                const auto best = m_profile.value().bests.get(kBestTime);
                footer = keyText("{left}{right} MOVE   {jump} JUMP   {slash} SLASH   {shoot} SHOOT   {map} MAP   F11 FULLSCREEN");
                if (best) {
                    footer = "BEST ESCAPE " + _clock(*best) + "      " + footer;
                }
            } else if (m_menus.is(ArcadeScreen::Paused) && m_session) {
                subtitle = std::string{m_session->room().def->name} + "  -  " + std::to_string(m_session->progress().completion()) + "%";
                footer = "RESTART RETURNS TO YOUR LAST SHRINE";
            }
            gamelib::ui::drawMenu(list, *menu, m_canvas, _menuStyle(), subtitle, footer);
        }
    }
    m_transition.draw(list, m_canvas);
    (void)app;
}

void CavernGame::drawPrompts(gamelib::draw::DrawList& list, const game::Session& s) const
{
    const int st = s.stationInReach();
    if (st < 0 || s.phase() != game::Phase::Playing || m_overlay != Overlay::None) {
        return;
    }
    const game::Station& station = game::world().stations()[static_cast<std::size_t>(st)];
    std::string text;
    switch (station.kind) {
    case game::StationKind::Save: text = "{up} REST"; break;
    case game::StationKind::Map: text = "{up} STUDY THE MAP"; break;
    case game::StationKind::Sign:
    case game::StationKind::Tablet: text = "{up} READ"; break;
    }
    const Vec2 world = s.map().cellBottom(station.cell) - Vec2{0.0f, 40.0f};
    gamelib::draw::drawWorldText(list, m_view.view(), m_canvas, world, keyText(text),
                                 {.size = 30.0f, .color = kText, .weight = 1.2f, .align = Align::Center,
                                  .shadow = {.color = kInk.withAlpha(0.8f), .offset = {3.0f, 3.0f}}});
}

void CavernGame::drawHud(gamelib::draw::DrawList& list, const game::Session& s, const float time) const
{
    const game::Hero& h = s.hero();
    // Masks of health.
    for (int i = 0; i < h.health.max; ++i) {
        const Vec2 at{70.0f + static_cast<float>(i) * 50.0f, 64.0f};
        const bool full = i < h.health.current;
        list.circle(at, 18.0f, kInk.withAlpha(0.7f), {.glowWidth = 6.0f, .glowColor = kInk.withAlpha(0.4f)});
        list.circle(at, 15.0f, full ? kHealth : srgb(0x2A2E3A), {.ringWidth = full ? 0.0f : 3.0f,
                                                                   .glowWidth = full ? 8.0f : 0.0f,
                                                                   .glowColor = srgb(0xFFFFFF, 0.25f)});
        if (full) {
            list.circle(at + Vec2{-4.0f, -2.0f}, 3.0f, kInk);
            list.circle(at + Vec2{4.0f, -2.0f}, 3.0f, kInk);
        }
    }
    // Bolts.
    for (int i = 0; i < h.ammo.max; ++i) {
        const Vec2 at{62.0f + static_cast<float>(i) * 26.0f, 112.0f};
        const bool full = i < h.ammo.current;
        list.box(Rect::fromCenter(at, {12.0f, 22.0f}), full ? kAmmo : srgb(0x1E2A34),
                 {.cornerRadius = 5.0f, .glowWidth = full ? 6.0f : 0.0f, .glowColor = kAmmo.withAlpha(0.35f)}, 0.4f);
    }
    // Keys found.
    for (int k = 0; k < 4; ++k) {
        if (s.progress().hasKey(k)) {
            const Vec2 at{70.0f + static_cast<float>(k) * 40.0f, 158.0f};
            list.circle(at - Vec2{6.0f, 0.0f}, 8.0f, keyColor(k), {.ringWidth = 4.0f, .glowWidth = 6.0f, .glowColor = keyColor(k).withAlpha(0.4f)});
            list.box(Rect{at.x, at.y - 2.5f, 16.0f, 5.0f}, keyColor(k));
        }
    }
    // The minimap.
    const Rect mini{1920.0f - 48.0f - 330.0f, 36.0f, 330.0f, 190.0f};
    drawMinimap(list, s, mini, false, time);
    // The guardian's bar.
    if (const game::Boss* boss = s.boss()) {
        // On a dark plate in the band below the floor line, never on the playfield.
        gamelib::ui::drawPanel(list, {420.0f, 984.0f, 1080.0f, 86.0f}, _panel());
        const Rect bar{460.0f, 1036.0f, 1000.0f, 20.0f};
        gamelib::ui::drawBar(list, bar, static_cast<float>(boss->shown) / static_cast<float>(std::max(boss->maxHp, 1)),
                             {.background = srgb(0x0A0C14, 0.9f), .fill = kDanger, .trail = srgb(0xFFE08A)});
        gamelib::draw::drawVectorText(list, game::bossName(boss->kind), 960.0f, 994.0f,
                                      {.size = 30.0f, .color = kText, .weight = 1.3f, .align = Align::Center,
                                       .shadow = {.color = kInk, .offset = {3.0f, 3.0f}}});
    }
    // The escape clock.
    if (const std::optional<int> left = s.escapeTicksLeft()) {
        const bool urgent = *left < 20 * game::kTickRate;
        const Color c = urgent && std::fmod(time, 0.5f) < 0.25f ? kDanger : kGold;
        gamelib::draw::drawVectorText(list, gamelib::core::formatClock(*left, game::kTickRate, 1), 960.0f, 50.0f,
                                      {.size = 64.0f, .color = c, .weight = 1.6f, .align = Align::Center,
                                       .shadow = {.color = kInk, .offset = {4.0f, 4.0f}}});
    }
    m_banners.draw(list, 960.0f, {.top = 300.0f, .titleSize = 70.0f, .titleWeight = 1.5f, .linePixel = 5.0f, .lineGap = 90.0f,
                                  .lineColor = kText, .shadow = kInk.withAlpha(0.8f)});
}

void CavernGame::drawMinimap(gamelib::draw::DrawList& list, const game::Session& s, const Rect area, const bool full,
                             const float time) const
{
    const game::World& w = game::world();
    const gamelib::core::MapReveal& reveal = s.progress().reveal;
    const Vec2 here = Vec2{s.hero().body.box.center().x / game::kScreenWidth, s.hero().body.box.center().y / game::kScreenHeight};
    const gamelib::ui::WorldMapStyle style = _mapStyle(full);
    const gamelib::core::GridLayout layout =
        full ? gamelib::ui::worldMapLayout(area, reveal, style) : gamelib::ui::minimapLayout(area, here, 60.0f);
    std::vector<gamelib::ui::MapMarker> markers;
    for (const game::Station& st : w.stations()) {
        if (st.kind != game::StationKind::Save && st.kind != game::StationKind::Map) {
            continue;
        }
        const Vec2 at{(static_cast<float>(st.cell.x) + 0.5f) / game::kScreenColumns,
                      (static_cast<float>(st.cell.y) + 0.5f) / game::kScreenRows};
        markers.push_back({.at = at, .color = st.kind == game::StationKind::Save ? srgb(0x7DFFB0) : srgb(0xFFD27A),
                           .shape = gamelib::ui::MarkerShape::Square, .size = 0.3f});
    }
    for (const game::BossDef& b : w.bosses()) {
        if (s.progress().flags.has(b.flag)) {
            continue;
        }
        const Vec2 at{(static_cast<float>(b.cell.x) + 0.5f) / game::kScreenColumns,
                      (static_cast<float>(b.cell.y) + 0.5f) / game::kScreenRows};
        markers.push_back({.at = at, .color = kDanger, .shape = gamelib::ui::MarkerShape::Diamond, .size = 0.45f});
    }
    const auto items = w.items();
    for (std::size_t i = 0; i < items.size(); ++i) {
        const game::Item& it = items[i];
        const game::RoomDef& def = *w.room(it.room).def;
        // Seen: its room was visited and it is still there.
        if (!s.itemVisible(static_cast<int>(i))
            || reveal.strongest(def.corner, def.size) != gamelib::core::MapKnowledge::Visited) {
            continue;
        }
        const Vec2 at{(static_cast<float>(it.cell.x) + 0.5f) / game::kScreenColumns,
                      (static_cast<float>(it.cell.y) + 0.5f) / game::kScreenRows};
        markers.push_back({.at = at, .color = kGold, .shape = gamelib::ui::MarkerShape::Ring, .size = 0.32f});
    }
    const float blink = 0.6f + 0.4f * std::sin(time * 6.0f);
    markers.push_back({.at = here, .color = srgb(0xFFFFFF, blink), .shape = gamelib::ui::MarkerShape::Dot, .size = full ? 0.4f : 0.45f,
                       .showInFog = true});
    if (!full) {
        gamelib::ui::drawPanel(list, area.expanded(6.0f), _panel());
    }
    gamelib::ui::drawWorldMap(list, area, layout, m_worldMap, reveal, markers, style);
}

void CavernGame::drawMapScreen(gamelib::draw::DrawList& list, const game::Session& s, const float time) const
{
    const Rect canvas{0.0f, 0.0f, static_cast<float>(m_canvas.width), static_cast<float>(m_canvas.height)};
    list.box(canvas, kInk.withAlpha(0.88f));
    const Rect panel{80.0f, 90.0f, 1760.0f, 880.0f};
    gamelib::ui::drawPanel(list, panel, _panel());
    gamelib::draw::drawVectorText(list, "MAP", 960.0f, 110.0f, {.size = 54.0f, .color = kGold, .weight = 1.5f, .align = Align::Center});
    drawMinimap(list, s, {panel.x + 50.0f, panel.y + 90.0f, panel.width - 100.0f, panel.height - 120.0f}, true, time);
    const std::string info = std::string{game::areaName(s.area())} + "  -  " + std::string{s.room().def->name}
                             + "      ITEMS " + std::to_string(s.progress().itemsTaken()) + "/"
                             + std::to_string(game::world().itemCount()) + "  (" + std::to_string(s.progress().completion())
                             + "%)      TIME " + _clock(s.progress().ticks);
    list.text(info, 960.0f, 990.0f, {.pixelSize = 4.0f, .color = kText, .align = Align::Center, .shadow = true});
    list.text("SHRINE", 711.0f, 1036.0f, {.pixelSize = 3.0f, .color = srgb(0x7DFFB0)});
    list.text("MAP", 859.0f, 1036.0f, {.pixelSize = 3.0f, .color = srgb(0xFFD27A)});
    list.text("GUARDIAN", 953.0f, 1036.0f, {.pixelSize = 3.0f, .color = kDanger});
    list.text("ITEM", 1137.0f, 1036.0f, {.pixelSize = 3.0f, .color = kGold});
}

void CavernGame::drawSlots(gamelib::draw::DrawList& list, const float time) const
{
    (void)time;
    gamelib::ui::SaveSlotStyle style{.menu = _menuStyle(), .danger = kDanger};
    style.menu.veil = Color{0.0f, 0.0f, 0.02f, 0.35f}; // the demo stays visible behind
    style.menu.titlePixel = 9.0f;
    style.width = 940.0f; // room for the erase question
    const gamelib::ui::SaveSlotLabels labels{.slot = "JOURNEY",
                                             .empty = "NEW JOURNEY",
                                             .damaged = "DAMAGED - A BACKUP WILL BE TRIED",
                                             .erase = "ERASE THIS JOURNEY? PRESS AGAIN",
                                             .back = "BACK"};
    gamelib::ui::drawSaveSlots(list, m_slotList, m_canvas, style, labels, "CHOOSE A JOURNEY",
                               keyText("{jump} BEGIN    {left} ERASE    {pause} BACK"));
}

void CavernGame::drawAbilityScene(gamelib::draw::DrawList& list, const game::Session& s, const float time) const
{
    const float t = static_cast<float>(s.phaseTicks()) / static_cast<float>(game::kAbilityGetTicks);
    const float in = std::min(1.0f, t * 5.0f);
    const float out = std::min(1.0f, (1.0f - t) * 6.0f);
    const float k = std::min(in, out);
    const Rect canvas{0.0f, 0.0f, static_cast<float>(m_canvas.width), static_cast<float>(m_canvas.height)};
    list.box(canvas, kInk.withAlpha(0.55f * k));
    // Rays behind the name.
    for (int i = 0; i < 12; ++i) {
        const float a = static_cast<float>(i) * kPi / 6.0f + time * 0.4f;
        list.box(Rect::fromCenter({960.0f, 380.0f}, {1100.0f * k, 46.0f}), kGold.withAlpha(0.05f * k), {.cornerRadius = 23.0f}, a);
    }
    const game::Ability a = s.lastAbility();
    gamelib::draw::drawVectorText(list, game::abilityName(a), 960.0f, 330.0f,
                                  {.size = 96.0f * (0.8f + 0.2f * in), .color = kGold.withAlpha(k), .weight = 1.8f, .align = Align::Center,
                                   .shadow = {.color = kInk.withAlpha(0.8f * k), .offset = {5.0f, 6.0f}}});
    const std::vector<std::string> lines = gamelib::ui::wrapText(keyText([&] {
        switch (a) {
        case game::Ability::Dash: return std::string_view{"{dash} DASHES - ON THE GROUND, OR ONCE IN THE AIR. CROSS GAPS NO JUMP CAN SPAN."};
        case game::Ability::WallJump: return std::string_view{"PRESS AGAINST A WALL TO CLING. {jump} LEAPS OFF IT. GLASSY CRYSTAL GIVES NO GRIP."};
        case game::Ability::GroundPound: return std::string_view{"{down} AND {jump} IN THE AIR STRIKE DOWNWARD. CRACKED FLOORS SHATTER BENEATH IT."};
        case game::Ability::DoubleJump: return std::string_view{"{jump} AGAIN IN THE AIR TO LEAP ONCE MORE."};
        case game::Ability::Count: break;
        }
        return std::string_view{};
    }()),
                                                                 1200.0f, 5.0f);
    gamelib::ui::drawLines(list, lines, 960.0f, 480.0f, {.pixelSize = 5.0f, .color = kText.withAlpha(k), .align = Align::Center, .shadow = true}, 5.0f);
}

void CavernGame::drawEnding(gamelib::draw::DrawList& list, const float time) const
{
    const Rect canvas{0.0f, 0.0f, static_cast<float>(m_canvas.width), static_cast<float>(m_canvas.height)};
    // Dawn over the ruins: a warm sky and the silhouettes.
    list.add({.rect = canvas, .color = srgb(0x2A3A6A), .color2 = srgb(0xFFB070), .shape = m_shapes.backdrop,
              .params = {time * 6.0f, 0.0f, 5.0f, 0.0f}});
    list.add({.rect = canvas, .color = srgb(0x1A1A2E), .color2 = srgb(0xFFD08A), .shape = m_shapes.backdrop,
              .params = {time * 10.0f, 0.0f, 5.0f, 1.0f}});
    const float k = std::min(1.0f, m_endingTime / 2.0f);
    gamelib::draw::drawVectorText(list, "INTO THE DAYLIGHT", 960.0f, 140.0f,
                                  {.size = 90.0f, .color = kGold.withAlpha(k), .weight = 1.8f, .align = Align::Center,
                                   .shadow = {.color = kInk.withAlpha(0.8f * k), .offset = {5.0f, 6.0f}}});
    const std::vector<std::string> story = gamelib::ui::wrapText(
        "THE MOLTEN HEART IS STILL. THE CAVES SETTLE INTO SILENCE BEHIND YOU, AND THE WANDERER WALKS ON INTO THE MORNING.",
        1300.0f, 5.0f);
    gamelib::ui::drawLines(list, story, 960.0f, 290.0f, {.pixelSize = 5.0f, .color = kText.withAlpha(k), .align = Align::Center, .shadow = true}, 5.0f);
    const float k2 = std::clamp((m_endingTime - 1.5f) / 1.5f, 0.0f, 1.0f);
    const Rect panel{560.0f, 470.0f, 800.0f, 380.0f};
    gamelib::ui::PanelStyle style = _panel();
    style.fill = style.fill.withAlpha(0.98f * k2);
    style.border = style.border.withAlpha(k2);
    gamelib::ui::drawPanel(list, panel, style);
    const auto row = [&](const float y, const std::string& label, const std::string& value, const Color c) {
        list.text(label, panel.x + 60.0f, y, {.pixelSize = 6.0f, .color = kTextDim.withAlpha(k2)});
        list.text(value, panel.right() - 60.0f, y, {.pixelSize = 6.0f, .color = c.withAlpha(k2), .align = Align::Right});
    };
    row(panel.y + 50.0f, "TIME", _clock(m_endTicks) + (m_newBest ? "  BEST!" : ""), kGold);
    row(panel.y + 130.0f, "COMPLETION", std::to_string(m_endCompletion) + "%", kText);
    row(panel.y + 210.0f, "DEATHS", std::to_string(m_endDeaths), kText);
    if (const auto best = m_profile.value().bests.get(kBestTime)) {
        row(panel.y + 290.0f, "BEST ESCAPE", _clock(*best), kTextDim);
    }
    if (m_endingTime > 4.0f) {
        list.text(keyText("{jump} RETURN TO THE TITLE"), 960.0f, 960.0f,
                  {.pixelSize = 5.0f, .color = kText.withAlpha(0.6f + 0.4f * std::sin(time * 3.0f)), .align = Align::Center});
    }
}

} // namespace cavern::app
