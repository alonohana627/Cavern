#include "view.hpp"

#include "palette.hpp"

#include "backdrop_frag.spv.hpp"
#include "boss_frag.spv.hpp"
#include "enemy_frag.spv.hpp"
#include "hero_frag.spv.hpp"
#include "item_frag.spv.hpp"
#include "shot_frag.spv.hpp"
#include "tile_frag.spv.hpp"

#include <gamelib/core/collision.hpp>
#include <gamelib/draw/parallax.hpp>
#include <gamelib/draw/tile_map.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <cstdlib>
#include <span>

namespace cavern::app {

namespace {

// helpers

using gamelib::core::Cell;
using gamelib::core::Rect;
using gamelib::core::Vec2;
namespace layers = gamelib::draw::layers;

constexpr float kPi = std::numbers::pi_v<float>;

struct AreaTiles {
    gamelib::draw::TileSet solid;
    gamelib::draw::TileSet water;
};

const AreaTiles& _tiles(const Shapes& shapes, const game::Area area)
{
    static std::array<std::optional<AreaTiles>, game::kAreaCount> cache;
    std::optional<AreaTiles>& slot = cache[static_cast<std::size_t>(area)];
    if (slot) {
        return *slot;
    }
    const AreaPalette& pal = areaPalette(area);
    const float a = static_cast<float>(area);
    AreaTiles t;
    const gamelib::draw::TileStyle rock{.shape = shapes.tile, .color = pal[Role::Rock], .color2 = pal[Role::RockEdge],
                                        .params = {0.0f, a, 0.6f, 0.0f}, .group = 0};
    t.solid.set('#', rock);
    gamelib::draw::TileStyle cracked = rock;
    cracked.params[0] = 5.0f;
    t.solid.set('%', cracked);
    cracked.params[0] = 6.0f;
    t.solid.set('X', cracked);
    cracked.params[0] = 7.0f;
    t.solid.set('Q', cracked);
    t.solid.set('*', {.shape = shapes.tile, .color = srgb(0x5A3A9A), .color2 = srgb(0xC9A6FF),
                      .params = {1.0f, a, 0.6f, 0.0f}, .group = 2});
    t.solid.set('=', {.shape = shapes.tile, .color = pal[Role::Rock].scaled(0.8f), .color2 = pal[Role::RockEdge],
                      .params = {2.0f, a, 0.0f, 0.0f}, .group = 3});
    t.solid.set('^', {.shape = shapes.tile, .color = pal[Role::Rock], .color2 = srgb(0xE8E0D8),
                      .params = {3.0f, a, 0.0f, 0.0f}, .group = 4});
    t.solid.set('L', {.shape = shapes.tile, .color = srgb(0xC0280A), .color2 = srgb(0xFFB030),
                      .params = {8.0f, a, 0.0f, 0.0f}, .group = 5});
    t.solid.set('Z', {.shape = shapes.tile, .color = pal[Role::Rock], .color2 = pal[Role::Accent],
                      .params = {9.0f, a, 0.0f, 0.0f}, .group = 6});
    t.water.set('~', {.shape = shapes.tile, .color = pal[Role::Water], .color2 = srgb(0xBFEFFF),
                      .params = {4.0f, a, 0.0f, 0.0f}, .group = 1});
    slot = t;
    return *slot;
}

struct EnemyLook {
    Color main;
    Color glow;
};

EnemyLook _enemyLook(const game::EnemyKind kind)
{
    using K = game::EnemyKind;
    switch (kind) {
    case K::Crawler: return {srgb(0x6A5A8A), srgb(0xFFD27A)};
    case K::Bat: return {srgb(0x4A4058), srgb(0xFF6A5A)};
    case K::Sentry: return {srgb(0x8A7E6A), srgb(0xFF9A3A)};
    case K::SporePuff: return {srgb(0x7A5A9A), srgb(0x9AFFB0)};
    case K::Hopper: return {srgb(0xC0504A), srgb(0xFFE6A0)};
    case K::Creeper: return {srgb(0x4E7A4A), srgb(0xD8FF7A)};
    case K::Eel: return {srgb(0x2A5A7A), srgb(0x7AF0FF)};
    case K::Shellback: return {srgb(0x3A6A8A), srgb(0x8AB8D0)};
    case K::Jelly: return {srgb(0x5A8AD8), srgb(0x9AE8FF)};
    case K::Shardling: return {srgb(0x3A2A58), srgb(0xD28AFF)};
    case K::PrismEye: return {srgb(0x6A4AA8), srgb(0x7AF2FF)};
    case K::Moth: return {srgb(0x2A2238), srgb(0xE6A8FF)};
    case K::Wisp: return {srgb(0x8A2A10), srgb(0xFF8A2A)};
    case K::Drone: return {srgb(0x3A3030), srgb(0xFFB04A)};
    case K::Lobber: return {srgb(0x4A2A22), srgb(0xFF6A1A)};
    case K::Count: break;
    }
    return {srgb(0x808080), srgb(0xFFFFFF)};
}

EnemyLook _bossLook(const game::BossKind kind)
{
    using B = game::BossKind;
    switch (kind) {
    case B::Sporemother: return {srgb(0x8A3A6A), srgb(0x9AFFB8)};
    case B::Tidewarden: return {srgb(0x2E5C8A), srgb(0x9AE8FF)};
    case B::Colossus: return {srgb(0x8A7A62), srgb(0xFFC060)};
    case B::PrismWyrm: return {srgb(0x5A3AA0), srgb(0xE29BFF)};
    case B::Heart: return {srgb(0x3A2420), srgb(0xFF6A1A)};
    case B::Count: break;
    }
    return {srgb(0x808080), srgb(0xFFFFFF)};
}

int _heroPose(const game::Hero& h)
{
    const gamelib::core::PlatformerBody& b = h.body;
    if (h.pound.active()) {
        return 6;
    }
    if (h.hurtTicks > 0) {
        return 7;
    }
    if (h.swimming) {
        return 8;
    }
    if (b.dashLeft > 0) {
        return 5;
    }
    if (b.wallSliding) {
        return 4;
    }
    if (!b.grounded) {
        return b.velocity.y < 0.0f ? 2 : 3;
    }
    return std::abs(b.velocity.x) > 0.2f ? 1 : 0;
}

void _item(gamelib::draw::DrawList& list, const Shapes& shapes, const Vec2 center, const Vec2 size, const int kind,
           const Color main, const Color glow, const float seed, const float variant = 0.0f, const float highlight = 0.0f)
{
    list.add({.rect = Rect::fromCenter(center, size), .color = main, .color2 = glow, .shape = shapes.item,
              .params = {static_cast<float>(kind), seed, variant, highlight}});
}

} // namespace

Shapes Shapes::registerWith(gamelib::gfx::Renderer& renderer)
{
    return {
        .tile = renderer.addShape(shaders::tile_frag),
        .backdrop = renderer.addShape(shaders::backdrop_frag),
        .hero = renderer.addShape(shaders::hero_frag),
        .enemy = renderer.addShape(shaders::enemy_frag),
        .boss = renderer.addShape(shaders::boss_frag),
        .item = renderer.addShape(shaders::item_frag),
        .shot = renderer.addShape(shaders::shot_frag),
    };
}

WorldView::WorldView(const gamelib::core::Size canvas)
    : m_canvas{canvas}
    , m_camera{canvas, kZoom}
{
}

Rect WorldView::visible() const
{
    return m_camera.view().visibleWorld(m_canvas);
}

void WorldView::snap(const game::Session& session)
{
    m_room = session.scrollTarget();
    m_camera.setBounds(game::world().room(m_room).bounds);
    m_camera.centerOn(session.hero().body.box.center());
}

void WorldView::follow(const game::Session& session, const float alpha, const float seconds)
{
    const int target = session.scrollTarget();
    if (target != m_room) {
        m_room = target;
        m_camera.setBounds(game::world().room(target).bounds,
                           static_cast<float>(game::kScrollTicks) / static_cast<float>(game::kTickRate));
    }
    const game::Hero& hero = session.hero();
    const Rect box = gamelib::core::lerp(hero.previous, hero.body.box, alpha);
    m_camera.follow({.position = box.center(), .facing = static_cast<float>(hero.body.facing), .grounded = hero.body.grounded},
                    seconds, {.deadZone = {40.0f, 30.0f}, .lookahead = 36.0f, .lookaheadRate = 2.0f, .rate = 7.0f});
}

void WorldView::draw(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, const Effects& effects,
                     const WorldDrawOptions& options) const
{
    list.setView({});
    drawBackdrop(list, shapes, session, options.time);
    list.setView(m_camera.view(options.shake));
    drawTiles(list, shapes, session, options.time);
    drawProps(list, shapes, session, options.time);
    drawEnemies(list, shapes, session, options.alpha, options.time);
    drawBoss(list, shapes, session, options.alpha, options.time);
    list.setLayer(layers::Entities + 50);
    const game::Hero& hero = session.hero();
    if (session.phase() != game::Phase::Dying && session.phase() != game::Phase::Dead
        && session.phase() != game::Phase::Hazard && hero.vulnerable.visible(3)) {
        const Rect box = gamelib::core::lerp(hero.previous, hero.body.box, options.alpha);
        const float glow = session.phase() == game::Phase::AbilityGet ? 1.0f : 0.0f;
        drawHero(list, shapes, hero, box, options.time, glow);
    }
    drawShots(list, shapes, session, options.time);
    list.setLayer(layers::Effects);
    effects.drawWorld(list);
    effects.drawAmbient(list, session.area(), options.time);
    drawLights(list, session, options.time);
    list.setView({});
}

void WorldView::drawHero(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Hero& hero, const Rect box,
                         const float time, const float glow)
{
    (void)time;
    constexpr float kMargin = 10.0f;
    int pose = _heroPose(hero);
    float phase = std::fmod(hero.walked / 26.0f, 1.0f);
    if (hero.slash.busy() && pose <= 1) {
        pose = 9;
        phase = hero.slash.progress();
    }
    list.add({.rect = box.expanded(kMargin),
              .color = kHeroCloak,
              .color2 = kHeroTrim,
              .shape = shapes.hero,
              .params = {static_cast<float>(hero.body.facing), static_cast<float>(pose), phase, kMargin},
              .bits = static_cast<std::uint64_t>(std::clamp(glow, 0.0f, 1.0f) * 255.0f)});
    // The slash: a crescent where the hitbox is.
    if (hero.slash.busy() && hero.slash.phase() != gamelib::core::AttackPhase::Startup) {
        const Rect hit = hero.slash.hitbox(box.center());
        const float rotation = hero.slashAim < 0 ? -kPi / 2.0f : (hero.slashAim > 0 ? kPi / 2.0f : (hero.slash.facing() < 0 ? kPi : 0.0f));
        const float size = std::max(hit.width, hit.height) + 18.0f;
        list.add({.rect = Rect::fromCenter(hit.center(), {size, size}),
                  .color = kHeroTrim,
                  .color2 = srgb(0xBFE4FF),
                  .shape = shapes.shot,
                  .rotation = rotation,
                  .params = {9.0f, 0.0f, hero.slash.progress(), 0.0f}});
    }
}

void WorldView::drawBackdrop(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session,
                             const float time) const
{
    (void)time;
    const game::Area area = session.area();
    const AreaPalette& pal = areaPalette(area);
    const float a = static_cast<float>(area);
    list.setLayer(layers::Background);
    const bool sky = area == game::Area::Ruins;
    const std::array backdrop{
        gamelib::draw::ParallaxLayer{.shape = shapes.backdrop, .factor = sky ? Vec2{0.03f, 0.02f} : Vec2{0.12f, 0.08f},
                                     .color = pal[Role::SkyTop], .color2 = pal[sky ? Role::SkyLow : Role::Accent],
                                     .params = {a, 0.0f}},
        gamelib::draw::ParallaxLayer{.shape = shapes.backdrop, .factor = {0.25f, sky ? 0.04f : 0.12f}, .color = pal[Role::Far],
                                     .color2 = pal[Role::Accent], .params = {a, 1.0f}},
        gamelib::draw::ParallaxLayer{.shape = shapes.backdrop, .factor = {0.5f, sky ? 0.08f : 0.3f}, .color = pal[Role::Mid],
                                     .color2 = pal[Role::RockEdge], .params = {a, 2.0f}},
    };
    gamelib::draw::drawParallax(list, backdrop, m_camera.view(), m_canvas);
}

void WorldView::drawTiles(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session,
                          const float time) const
{
    const Rect visible = m_camera.view().visibleWorld(m_canvas).expanded(game::kTile);
    // Collapsing flagstones shake before they go.
    std::vector<gamelib::draw::TileOffset> offsets;
    for (const auto& tile : session.collapse().tiles()) {
        if (tile.state == gamelib::core::CrumbleTiles::State::Shaking) {
            const float k = session.collapse().shakeProgress(tile);
            offsets.push_back({.cell = tile.cell,
                               .offset = {std::sin(time * 60.0f + static_cast<float>(tile.cell.x)) * 1.2f * k, k * 1.5f}});
        }
    }
    // The tiles of the rooms on screen in their own area's look (two rooms during a scroll).
    const int here = session.roomIndex();
    const int there = session.scrollTarget();
    const std::array<int, 2> rooms{here, there};
    for (int k = 0; k < (here == there ? 1 : 2); ++k) {
        const int room = rooms[static_cast<std::size_t>(k)];
        const game::RoomInfo& info = game::world().room(room);
        const Rect area = gamelib::core::Rect{std::max(visible.left(), info.bounds.left()), std::max(visible.top(), info.bounds.top()),
                                              0.0f, 0.0f};
        const float right = std::min(visible.right(), info.bounds.right());
        const float bottom = std::min(visible.bottom(), info.bounds.bottom());
        if (right <= area.x || bottom <= area.y) {
            continue;
        }
        const Rect clip{area.x, area.y, right - area.x, bottom - area.y};
        const AreaTiles& tiles = _tiles(shapes, info.def->area);
        list.setLayer(layers::TilesBehind);
        (void)gamelib::draw::drawTiles(list, session.map(), 0, tiles.solid, clip.expanded(-0.01f), {.offsets = offsets});
        list.setLayer(layers::TilesFront);
        (void)gamelib::draw::drawTiles(list, session.map(), 0, tiles.water, clip.expanded(-0.01f));
    }
    // The seal of a guardian's room.
    (void)time;
}

void WorldView::drawProps(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session,
                          const float time) const
{
    (void)time;
    const game::World& w = game::world();
    const Rect visible = m_camera.view().visibleWorld(m_canvas).expanded(32.0f);
    const AreaPalette& pal = areaPalette(session.area());
    list.setLayer(layers::TilesBehind + 10);
    // Doors.
    const auto doors = w.doors();
    for (std::size_t i = 0; i < doors.size(); ++i) {
        const game::Door& d = doors[i];
        if (!gamelib::core::overlaps(d.box, visible)) {
            continue;
        }
        const bool open = session.doorOpen(static_cast<int>(i));
        Color main = srgb(0x6A6670);
        Color glow = pal[Role::Accent];
        float kind = 0.0f;
        switch (d.kind) {
        case game::DoorKind::Plain: break;
        case game::DoorKind::Key:
            kind = 1.0f;
            glow = keyColor(d.key);
            break;
        case game::DoorKind::OpensFromLeft: kind = 2.0f; break;
        case game::DoorKind::OpensFromRight: kind = 3.0f; break;
        case game::DoorKind::Escape:
            kind = 4.0f;
            main = srgb(0x3A2420);
            glow = srgb(0xFF6A1A);
            break;
        }
        list.add({.rect = d.box, .color = main, .color2 = glow, .shape = shapes.item,
                  .params = {open ? 11.0f : 10.0f, static_cast<float>(i), kind, 0.0f}});
    }
    // Stations.
    const int reach = session.stationInReach();
    const auto stations = w.stations();
    for (std::size_t i = 0; i < stations.size(); ++i) {
        const game::Station& s = stations[i];
        const Vec2 feet = session.map().cellBottom(s.cell);
        if (!visible.contains(feet)) {
            continue;
        }
        const float high = static_cast<int>(i) == reach ? 1.0f : 0.0f;
        switch (s.kind) {
        case game::StationKind::Save:
            _item(list, shapes, feet - Vec2{0.0f, 14.0f}, {28.0f, 30.0f}, 4, pal[Role::RockEdge], pal[Role::Accent],
                  static_cast<float>(i), 0.0f, high);
            break;
        case game::StationKind::Map:
            _item(list, shapes, feet - Vec2{0.0f, 13.0f}, {26.0f, 28.0f}, 5, pal[Role::RockEdge], pal[Role::Accent],
                  static_cast<float>(i), 0.0f, high);
            break;
        case game::StationKind::Sign:
            _item(list, shapes, feet - Vec2{0.0f, 12.0f}, {22.0f, 26.0f}, 6, pal[Role::RockEdge], pal[Role::Accent],
                  static_cast<float>(i), 0.0f, high);
            break;
        case game::StationKind::Tablet:
            _item(list, shapes, feet - Vec2{0.0f, 13.0f}, {22.0f, 28.0f}, 7, pal[Role::Rock].scaled(1.6f), pal[Role::Accent],
                  static_cast<float>(i), 0.0f, high);
            break;
        }
    }
    // Items.
    list.setLayer(layers::Entities - 10);
    const auto items = w.items();
    for (std::size_t i = 0; i < items.size(); ++i) {
        const game::Item& it = items[i];
        const Vec2 center = session.map().cellCenter(it.cell);
        if (!visible.contains(center)) {
            continue;
        }
        if (it.kind == game::ItemKind::Ability) {
            _item(list, shapes, center + Vec2{0.0f, 6.0f}, {24.0f, 16.0f}, 13, pal[Role::RockEdge], pal[Role::Accent], 0.0f);
        }
        if (!session.itemVisible(static_cast<int>(i))) {
            continue;
        }
        switch (it.kind) {
        case game::ItemKind::Health:
            _item(list, shapes, center, {30.0f, 30.0f}, 0, srgb(0xE84A5A), srgb(0xFF8A9A), static_cast<float>(i));
            break;
        case game::ItemKind::Ammo:
            _item(list, shapes, center, {30.0f, 30.0f}, 1, kAmmo, srgb(0x9AF0FF), static_cast<float>(i));
            break;
        case game::ItemKind::Key:
            _item(list, shapes, center, {30.0f, 30.0f}, 2, keyColor(it.key), keyColor(it.key), static_cast<float>(i));
            break;
        case game::ItemKind::Ability:
            _item(list, shapes, center - Vec2{0.0f, 6.0f}, {50.0f, 50.0f}, 3, srgb(0xFFF0C0), kGold, static_cast<float>(i),
                  static_cast<float>(it.ability));
            break;
        }
    }
    for (const game::Drop& d : session.drops()) {
        const bool health = d.kind == game::DropKind::Health;
        _item(list, shapes, d.body.box.center(), {14.0f, 14.0f}, health ? 8 : 9, health ? srgb(0xE84A5A) : kAmmo,
              health ? srgb(0xFF8A9A) : srgb(0x9AF0FF), 0.0f);
    }
    // Daylight at the end of the escape.
    const Vec2 exit = session.map().cellCenter(w.exit());
    if (visible.contains(exit)) {
        _item(list, shapes, exit, {64.0f, 96.0f}, 12, kGold, kGold, 0.0f);
    }
}

void WorldView::drawEnemies(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session,
                            const float alpha, const float time) const
{
    (void)time;
    list.setLayer(layers::Entities);
    for (const game::Enemy& e : session.enemies()) {
        const Rect box = gamelib::core::lerp(e.previous, e.body.box, alpha);
        const EnemyLook look = _enemyLook(e.kind);
        const float margin = 6.0f;
        list.add({.rect = box.expanded(margin),
                  .color = look.main,
                  .color2 = look.glow,
                  .shape = shapes.enemy,
                  .params = {static_cast<float>(e.kind), static_cast<float>(e.body.direction),
                             static_cast<float>((e.age + e.seed) % 60) / 60.0f, static_cast<float>(e.flash) / 8.0f},
                  .bits = static_cast<std::uint64_t>(e.state)});
    }
}

void WorldView::drawBoss(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, const float alpha,
                         const float time) const
{
    (void)time;
    const game::Boss* boss = session.boss();
    if (boss == nullptr) {
        return;
    }
    list.setLayer(layers::Entities + 10);
    const Rect box = gamelib::core::lerp(boss->previous, boss->body.box, alpha);
    const EnemyLook look = _bossLook(boss->kind);
    const float facing = session.hero().body.box.center().x < box.center().x ? -1.0f : 1.0f;
    std::uint64_t state = 0;
    switch (boss->sm.current()) {
    case game::BossState::Move: state = 1; break;
    case game::BossState::Attack: state = 2; break;
    case game::BossState::Attack2: state = 3; break;
    case game::BossState::Attack3: state = 4; break;
    case game::BossState::Recover: state = 5; break;
    case game::BossState::Dead: state = 6; break;
    default: break;
    }
    state |= static_cast<std::uint64_t>(boss->phases.phase()) << 8;
    state |= static_cast<std::uint64_t>(std::clamp(boss->tell, 0, 255)) << 16;
    list.add({.rect = box.expanded(12.0f),
              .color = look.main,
              .color2 = look.glow,
              .shape = shapes.boss,
              .params = {static_cast<float>(boss->kind), facing, 0.0f, static_cast<float>(boss->flash) / 8.0f},
              .bits = state});
}

void WorldView::drawShots(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session,
                          const float time) const
{
    (void)time;
    list.setLayer(layers::Effects - 10);
    for (const game::Shot& s : session.shots().items()) {
        const int look = s.extra.enemy ? s.extra.look : 0;
        Color main = kAmmo;
        Color glow = srgb(0x7FE3FF);
        switch (look) {
        case 1: glow = srgb(0xFF9A3A); break;
        case 2: glow = srgb(0x9AFFB0); break;
        case 3: glow = srgb(0x9AE8FF); break;
        case 4: glow = srgb(0xE29BFF); break;
        case 5:
            main = srgb(0x2A2A30);
            glow = srgb(0xFFB04A);
            break;
        case 6: glow = srgb(0xFF6A1A); break;
        case 7:
            main = srgb(0xC9A6FF);
            glow = srgb(0x7AF2FF);
            break;
        case 8: glow = areaPalette(session.area())[Role::Accent]; break;
        default: break;
        }
        const float rotation = std::atan2(s.velocity.y, s.velocity.x);
        Vec2 size = s.size + Vec2{8.0f, 8.0f};
        if (look == 0) {
            size = {s.size.x + s.size.y + 14.0f, 10.0f};
        }
        list.add({.rect = Rect::fromCenter(s.position, size),
                  .color = main,
                  .color2 = glow,
                  .shape = shapes.shot,
                  .rotation = look == 0 || look == 1 || look == 4 || look == 7 ? rotation : 0.0f,
                  .params = {static_cast<float>(look), static_cast<float>(s.age), 0.0f, 0.0f}});
    }
    for (const auto& w : session.warnings()) {
        list.add({.rect = Rect::fromCenter(w.value, {28.0f, 28.0f}),
                  .color = srgb(0xFFFFFF),
                  .color2 = areaPalette(session.area())[Role::Accent],
                  .shape = shapes.shot,
                  .params = {10.0f, 0.0f, w.progress(), 0.0f}});
    }
}

void WorldView::drawLights(gamelib::draw::DrawList& list, const game::Session& session, const float time) const
{
    (void)time;
    const AreaPalette& pal = areaPalette(session.area());
    const game::Hero& hero = session.hero();
    if (session.phase() != game::Phase::Dead) {
        list.light({.position = hero.body.box.center(), .radius = 120.0f, .color = srgb(0xFFD8A0), .intensity = 1.0f,
                    .flicker = 0.08f, .seed = 1});
    }
    const Rect visible = m_camera.view().visibleWorld(m_canvas).expanded(64.0f);
    for (const game::Station& s : game::world().stations()) {
        const Vec2 at = session.map().cellBottom(s.cell) - Vec2{0.0f, 20.0f};
        if (visible.contains(at) && (s.kind == game::StationKind::Save || s.kind == game::StationKind::Map)) {
            list.light({.position = at, .radius = 110.0f, .color = pal[Role::Accent], .intensity = 1.1f, .flicker = 0.15f,
                        .seed = static_cast<std::uint32_t>(s.cell.x)});
        }
    }
    const auto items = game::world().items();
    for (std::size_t i = 0; i < items.size(); ++i) {
        const Vec2 at = session.map().cellCenter(items[i].cell);
        if (visible.contains(at) && session.itemVisible(static_cast<int>(i))) {
            list.light({.position = at, .radius = 70.0f, .color = kGold, .intensity = 0.9f, .flicker = 0.1f,
                        .seed = static_cast<std::uint32_t>(i)});
        }
    }
    for (const game::Enemy& e : session.enemies()) {
        if (e.kind == game::EnemyKind::Wisp || e.kind == game::EnemyKind::Jelly || e.kind == game::EnemyKind::SporePuff
            || e.kind == game::EnemyKind::Lobber || e.kind == game::EnemyKind::PrismEye) {
            list.light({.position = e.body.box.center(), .radius = 56.0f, .color = _enemyLook(e.kind).glow, .intensity = 0.8f});
        }
    }
    for (const game::Shot& s : session.shots().items()) {
        list.light({.position = s.position, .radius = 34.0f, .color = s.extra.enemy ? srgb(0xFFB070) : kAmmo, .intensity = 0.7f});
    }
    if (const game::Boss* boss = session.boss()) {
        list.light({.position = boss->body.box.center(), .radius = 150.0f, .color = _bossLook(boss->kind).glow, .intensity = 0.8f,
                    .flicker = 0.1f});
    }
    list.setPostProcess({.ambient = pal[Role::Ambient],
                         .bloom = {.intensity = 0.55f, .threshold = 0.85f},
                         .grade = {.saturation = 1.05f}});
}

} // namespace cavern::app
