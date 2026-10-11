#include "session.hpp"

#include <gamelib/core/collision.hpp>
#include <gamelib/core/replay.hpp>

#include <algorithm>
#include <cmath>

namespace cavern::game {

namespace {

// helpers

using gamelib::core::Cell;
using gamelib::core::Rect;
using gamelib::core::TileId;
using gamelib::core::Vec2;

constexpr float kStationReach = 26.0f;

Vec2 _feetOf(const Cell cell)
{
    return {(static_cast<float>(cell.x) + 0.5f) * kTile, static_cast<float>(cell.y + 1) * kTile};
}

Rect _itemBox(const Cell cell)
{
    return gamelib::core::cellRect(cell, kTile).expanded(-2.0f);
}

} // namespace

Progress Progress::fresh()
{
    Progress p;
    const Cell size = world().mapSize();
    p.reveal = gamelib::core::MapReveal{size.x, size.y};
    return p;
}

int Progress::itemsTaken() const
{
    int taken = 0;
    for (const Item& item : world().items()) {
        taken += flags.has(item.flag) ? 1 : 0;
    }
    return taken;
}

int Progress::completion() const
{
    const int total = world().itemCount();
    return total > 0 ? itemsTaken() * 100 / total : 100;
}

Session::Session(Progress progress)
    : m_progress{std::move(progress)}
    , m_map{world().map()}
{
    const Cell size = world().mapSize();
    if (m_progress.reveal.width() != size.x || m_progress.reveal.height() != size.y) {
        m_progress.reveal = gamelib::core::MapReveal{size.x, size.y};
    }
    applyFlags();
    m_collapse = gamelib::core::CrumbleTiles{m_map.layer(0), gamelib::core::charTile('Q'), gamelib::core::charTile(' '),
                                             {.shakeTicks = 34, .goneTicks = -1, .connected = true}};
    respawn();
}

void Session::applyFlags()
{
    gamelib::core::TileLayer& layer = m_map.layer(0);
    for (const Door& door : world().doors()) {
        if (m_progress.flags.has(door.flag)) {
            for (const Cell c : door.cells) {
                layer.at(c) = gamelib::core::charTile(' ');
            }
        }
    }
    for (const Breakable& patch : world().breakables()) {
        if (m_progress.flags.has(patch.flag)) {
            for (const Cell c : patch.cells) {
                layer.at(c) = gamelib::core::charTile(' ');
            }
        }
    }
}

void Session::respawn()
{
    m_hero.health.max = m_progress.maxHealth();
    m_hero.ammo.max = m_progress.maxAmmo();
    m_hero.health.refill();
    m_hero.ammo.refill();
    m_hero.vulnerable = gamelib::core::Vulnerable{{}, 0, kHurtInvincibleTicks, kHurtStunTicks};
    m_hero.slash.cancel();
    m_hero.pound.cancel();
    m_hero.body = {};
    m_hero.shootCooldown = 0;
    m_hero.hurtTicks = 0;
    m_scroll.reset();
    if (m_boss) {
        sealRoom(false);
        m_boss.reset();
    }
    Vec2 feet = _feetOf(world().start());
    if (m_escapeStart >= 0) {
        // The escape starts again from the heart's chamber.
        for (const BossDef& def : world().bosses()) {
            if (def.kind == BossKind::Heart) {
                feet = _feetOf(def.cell);
            }
        }
        m_escapeStart = m_ticks;
    } else if (m_progress.saveStation >= 0 && m_progress.saveStation < static_cast<int>(world().stations().size())) {
        feet = _feetOf(world().stations()[static_cast<std::size_t>(m_progress.saveStation)].cell);
    }
    placeHero(feet);
    m_phase = Phase::Playing;
    m_phaseTicks = 0;
    enterRoom(world().roomAt(m_hero.body.box.center()), false);
}

void Session::placeHero(const Vec2 feet)
{
    m_hero.body.box = Rect::fromBottomCenter(feet, kHeroSize);
    m_hero.body.velocity = {};
    m_hero.previous = m_hero.body.box;
    m_hero.safeFeet = feet;
    m_hero.safeRoom = world().roomAt(m_hero.body.box.center());
    m_hero.vulnerable.setHurtbox(m_hero.body.box);
}

void Session::enterRoom(const int room, const bool fromScroll)
{
    const Area before = area();
    m_room = std::max(room, 0);
    m_enemies.clear();
    m_drops.clear();
    m_shots.clear();
    m_warnings.clear();
    const RoomInfo& info = world().room(m_room);
    m_progress.reveal.visitArea(info.def->corner, info.def->size);
    spawnRoom();
    if (fromScroll) {
        m_events.push({.kind = EventKind::Scroll, .at = m_hero.body.box.center(), .value = m_room});
    }
    if (!fromScroll || before != area()) {
        m_events.push({.kind = EventKind::AreaEnter, .at = m_hero.body.box.center(), .value = static_cast<int>(area())});
    }
}

void Session::spawnRoom()
{
    for (const SpawnDef& spawn : room().spawns) {
        spawnEnemy(spawn);
    }
}

bool Session::doorOpen(const int door) const
{
    return m_progress.flags.has(world().doors()[static_cast<std::size_t>(door)].flag);
}

bool Session::broken(const int breakable) const
{
    return m_progress.flags.has(world().breakables()[static_cast<std::size_t>(breakable)].flag);
}

bool Session::itemTaken(const int item) const
{
    return m_progress.flags.has(world().items()[static_cast<std::size_t>(item)].flag);
}

bool Session::itemVisible(const int item) const
{
    if (itemTaken(item)) {
        return false;
    }
    const Item& it = world().items()[static_cast<std::size_t>(item)];
    if (it.kind == ItemKind::Ability) {
        const int boss = world().room(it.room).boss;
        if (boss >= 0) {
            const BossDef& def = world().bosses()[static_cast<std::size_t>(boss)];
            return m_progress.flags.has(def.flag) && !(m_boss && m_phase == Phase::BossDown);
        }
    }
    return true;
}

int Session::stationInReach() const
{
    const Rect& box = m_hero.body.box;
    const auto& stations = world().stations();
    for (std::size_t i = 0; i < stations.size(); ++i) {
        const Station& s = stations[i];
        if (s.room != m_room) {
            continue;
        }
        const Vec2 feet = _feetOf(s.cell);
        if (std::abs(feet.x - box.center().x) <= kStationReach && std::abs(feet.y - box.bottom()) <= kTile) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

std::optional<int> Session::escapeTicksLeft() const
{
    if (m_escapeStart < 0) {
        return std::nullopt;
    }
    return std::max(0, kEscapeTicks - (m_ticks - m_escapeStart));
}

void Session::tick(const Input& input)
{
    m_wantsSave = false;
    ++m_ticks;
    ++m_phaseTicks;
    if (m_phase != Phase::Dead && m_phase != Phase::Escaped) {
        ++m_progress.ticks;
    }
    m_hero.previous = m_hero.body.box;
    for (Enemy& e : m_enemies) {
        e.previous = e.body.box;
    }
    for (Drop& d : m_drops) {
        d.previous = d.body.box;
    }
    if (m_boss) {
        m_boss->previous = m_boss->body.box;
    }
    switch (m_phase) {
    case Phase::Playing: playTick(input); break;
    case Phase::Scroll: scrollTick(); break;
    case Phase::Hazard:
        if (m_phaseTicks >= kHazardRespawnTicks) {
            m_room = m_hero.safeRoom;
            placeHero(m_hero.safeFeet);
            m_hero.vulnerable.makeInvincible(kHurtInvincibleTicks);
            m_phase = Phase::Playing;
            m_phaseTicks = 0;
        }
        break;
    case Phase::AbilityGet:
        if (m_phaseTicks >= kAbilityGetTicks) {
            m_phase = Phase::Playing;
            m_phaseTicks = 0;
        }
        break;
    case Phase::BossIntro:
        if (m_boss) {
            if (m_phaseTicks % 2 == 0 && m_boss->shown < m_boss->maxHp) {
                ++m_boss->shown;
            }
            stepBoss();
        }
        if (m_phaseTicks >= kBossIntroTicks) {
            if (m_boss) {
                m_boss->shown = m_boss->maxHp;
            }
            m_phase = Phase::Playing;
            m_phaseTicks = 0;
        }
        break;
    case Phase::BossDown:
        if (m_boss && m_phaseTicks % 12 == 0) {
            const Rect& b = m_boss->body.box;
            const Vec2 at{b.x + m_rng.uniform(0.0f, b.width), b.y + m_rng.uniform(0.0f, b.height)};
            m_events.push({.kind = EventKind::EnemyDown, .at = at, .value = 1});
        }
        if (m_phaseTicks >= kBossDownTicks && m_boss) {
            const BossDef& def = world().bosses()[static_cast<std::size_t>(m_boss->def)];
            m_progress.flags.set(def.flag);
            const bool heart = m_boss->kind == BossKind::Heart;
            m_events.push({.kind = EventKind::BossDown, .at = m_boss->body.box.center(), .boss = m_boss->kind});
            sealRoom(false);
            m_boss.reset();
            m_phase = Phase::Playing;
            m_phaseTicks = 0;
            if (heart) {
                startEscape();
            }
        }
        break;
    case Phase::Dying:
        if (m_phaseTicks >= kDeathTicks) {
            m_phase = Phase::Dead;
            m_phaseTicks = 0;
        }
        break;
    case Phase::Dead:
    case Phase::Escaped: break;
    }
}

void Session::playTick(const Input& input)
{
    m_hero.vulnerable.tick();
    stepHero(input);
    if (m_phase != Phase::Playing) {
        return;
    }
    checkRoomChange();
    if (m_phase != Phase::Playing) {
        return;
    }
    stepEnemies();
    heroMeetsEnemies();
    stepBoss();
    stepShots();
    stepDrops();
    if (m_phase != Phase::Playing) {
        return;
    }
    touchItems();
    for (const auto& e : m_collapse.tick(m_map.layer(0), kTile)) {
        if (e.kind == gamelib::core::CrumbleTiles::EventKind::Shake) {
            m_events.push({.kind = EventKind::Shake, .at = m_map.cellCenter(e.cell)});
        } else if (e.kind == gamelib::core::CrumbleTiles::EventKind::Break) {
            if (const int patch = world().breakableAt(e.cell); patch >= 0 && !broken(patch)) {
                breakPatch(patch);
            }
        }
    }
    if (input.interact) {
        useStation();
    }
    // The guardian wakes once the hero is well inside its room.
    if (room().boss >= 0 && !m_boss) {
        const BossDef& def = world().bosses()[static_cast<std::size_t>(room().boss)];
        const gamelib::core::Vec2 lair = m_map.cellCenter(def.cell);
        if (!m_progress.flags.has(def.flag) && std::abs(m_hero.body.box.center().x - room().bounds.center().x) < room().bounds.width / 2.0f - 3.0f * kTile
            && std::abs(m_hero.body.box.center().y - lair.y) < kScreenHeight / 2.0f
            && m_hero.body.grounded) {
            wakeBoss();
            return;
        }
    }
    if (const std::optional<int> left = escapeTicksLeft(); left && *left <= 0) {
        die();
        return;
    }
    if (m_escapeStart >= 0
        && gamelib::core::overlaps(m_hero.body.box, gamelib::core::cellRect(world().exit(), kTile).expanded(kTile))) {
        m_phase = Phase::Escaped;
        m_phaseTicks = 0;
        m_progress.finished = true;
        m_escapeStart = -1;
        m_events.push({.kind = EventKind::Escaped, .at = m_hero.body.box.center()});
    }
}

void Session::checkRoomChange()
{
    const Rect& bounds = room().bounds;
    const Vec2 center = m_hero.body.box.center();
    if (bounds.contains(center)) {
        return;
    }
    if (m_boss) {
        return; // sealed in: the seal tiles hold the hero
    }
    if (auto scroll = gamelib::core::RoomScroll::start(world().roomBounds(), m_room, m_hero.body.box, kScrollTicks, 10.0f)) {
        m_scroll = scroll;
        m_phase = Phase::Scroll;
        m_phaseTicks = 0;
        m_hero.slash.cancel();
        return;
    }
    // Out of the world (the open sky above the ruins): held at the edge.
    if (center.y < bounds.top()) {
        return;
    }
    die();
}

void Session::scrollTick()
{
    if (!m_scroll) {
        m_phase = Phase::Playing;
        return;
    }
    if (m_scroll->tick(m_hero.body.box)) {
        const int to = m_scroll->to;
        const gamelib::core::Direction side = m_scroll->side;
        m_scroll.reset();
        m_phase = Phase::Playing;
        m_phaseTicks = 0;
        enterRoom(to, true);
        // Rising into the room above: a lift onto its floor beside the way up.
        if (side == gamelib::core::Direction::Up) {
            m_hero.body.velocity.y = std::min(m_hero.body.velocity.y, -kRoomLift);
            m_hero.body.rising = false;
        }
    }
}

void Session::touchItems()
{
    const auto& items = world().items();
    for (std::size_t i = 0; i < items.size(); ++i) {
        const Item& item = items[i];
        if (item.room != m_room || !itemVisible(static_cast<int>(i))
            || !gamelib::core::overlaps(m_hero.body.box, _itemBox(item.cell))) {
            continue;
        }
        m_progress.flags.set(item.flag);
        Event e{.kind = EventKind::Item, .at = m_map.cellCenter(item.cell), .value = static_cast<int>(item.kind)};
        switch (item.kind) {
        case ItemKind::Health:
            ++m_progress.healthUpgrades;
            m_hero.health.max = m_progress.maxHealth();
            m_hero.health.refill();
            break;
        case ItemKind::Ammo:
            ++m_progress.ammoUpgrades;
            m_hero.ammo.max = m_progress.maxAmmo();
            m_hero.ammo.refill();
            break;
        case ItemKind::Key: m_progress.keys = static_cast<std::uint8_t>(m_progress.keys | (1u << item.key)); break;
        case ItemKind::Ability:
            m_progress.abilities.add(item.ability);
            m_lastAbility = item.ability;
            e.ability = item.ability;
            m_hero.health.refill();
            m_hero.ammo.refill();
            m_hero.body.velocity = {};
            m_phase = Phase::AbilityGet;
            m_phaseTicks = 0;
            break;
        }
        e.value = static_cast<int>(item.kind);
        m_events.push(e);
    }
}

void Session::useStation()
{
    const int index = stationInReach();
    if (index < 0 || !m_hero.body.grounded) {
        return;
    }
    const Station& s = world().stations()[static_cast<std::size_t>(index)];
    switch (s.kind) {
    case StationKind::Save:
        if (m_escapeStart >= 0) {
            return; // no saving while the core comes down
        }
        m_hero.health.refill();
        m_hero.ammo.refill();
        m_progress.saveStation = index;
        m_wantsSave = true;
        m_events.push({.kind = EventKind::Saved, .at = _feetOf(s.cell)});
        break;
    case StationKind::Map: {
        const Area a = world().room(s.room).def->area;
        for (const RoomInfo& r : world().rooms()) {
            if (r.def->area == a) {
                (void)m_progress.reveal.mapArea(r.def->corner, r.def->size);
            }
        }
        m_progress.flags.set("map." + std::to_string(static_cast<int>(a)));
        m_events.push({.kind = EventKind::Mapped, .at = _feetOf(s.cell), .value = static_cast<int>(a)});
        break;
    }
    case StationKind::Sign:
    case StationKind::Tablet: break; // read by the caller
    }
}

void Session::hurtHero(const int damage, const Rect& source)
{
    Hero& h = m_hero;
    if (m_phase != Phase::Playing || h.vulnerable.invincible()) {
        return;
    }
    const gamelib::core::Hit hit{.box = h.body.box, .damage = damage, .team = 1};
    if (!h.vulnerable.tryHit(hit)) {
        return;
    }
    h.health.restore(-damage);
    h.slash.cancel();
    h.pound.cancel();
    h.hurtTicks = 20;
    const Vec2 push = gamelib::core::knockbackAway(source, h.body.box, kKnockbackSpeed, 2.6f, -h.body.facing);
    gamelib::core::launch(h.body, push);
    h.body.lockTicks = kHurtStunTicks;
    m_events.push({.kind = EventKind::Hurt, .at = h.body.box.center(), .value = h.health.current});
    if (h.health.current <= 0) {
        die();
    }
}

void Session::hazardHit(const int damage)
{
    Hero& h = m_hero;
    h.health.restore(-damage);
    h.slash.cancel();
    h.pound.cancel();
    m_events.push({.kind = EventKind::HazardHit, .at = h.body.box.center(), .value = h.health.current});
    if (h.health.current <= 0) {
        die();
        return;
    }
    m_phase = Phase::Hazard;
    m_phaseTicks = 0;
    h.body.velocity = {};
}

void Session::die()
{
    if (m_phase == Phase::Dying || m_phase == Phase::Dead) {
        return;
    }
    ++m_progress.deaths;
    m_hero.health.current = 0;
    m_hero.body.velocity = {};
    m_phase = Phase::Dying;
    m_phaseTicks = 0;
    m_events.push({.kind = EventKind::Death, .at = m_hero.body.box.center()});
}

void Session::openDoor(const int door)
{
    const Door& d = world().doors()[static_cast<std::size_t>(door)];
    if (m_progress.flags.has(d.flag)) {
        return;
    }
    m_progress.flags.set(d.flag);
    for (const Cell c : d.cells) {
        m_map.layer(0).at(c) = gamelib::core::charTile(' ');
    }
    m_events.push({.kind = EventKind::DoorOpen, .at = d.box.center(), .value = door});
}

void Session::breakPatch(const int patch)
{
    const Breakable& b = world().breakables()[static_cast<std::size_t>(patch)];
    if (m_progress.flags.has(b.flag)) {
        return;
    }
    m_progress.flags.set(b.flag);
    Vec2 sum;
    for (const Cell c : b.cells) {
        m_map.layer(0).at(c) = gamelib::core::charTile(' ');
        sum += m_map.cellCenter(c);
    }
    m_events.push({.kind = EventKind::Break, .at = sum / static_cast<float>(b.cells.size()), .value = static_cast<int>(b.kind)});
}

void Session::strikeTiles(const Rect& box, const bool slash)
{
    const gamelib::core::CellRange range = gamelib::core::cellsOverlapping(box.expanded(1.0f), kTile);
    const gamelib::core::TileLayer& layer = m_map.layer(0);
    bool struck = false;
    std::vector<int> doors;
    for (int y = range.min.y; y <= range.max.y; ++y) {
        for (int x = range.min.x; x <= range.max.x; ++x) {
            const Cell c{x, y};
            if (!layer.inside(c)) {
                continue;
            }
            const TileId id = layer.at(c);
            if (isDoorTile(id)) {
                if (const int d = world().doorAt(c); d >= 0 && std::ranges::find(doors, d) == doors.end()) {
                    doors.push_back(d);
                }
            } else if (id == 'X') {
                if (const int p = world().breakableAt(c); p >= 0) {
                    breakPatch(p);
                }
            } else if (world().shapes()[id] == gamelib::core::TileShape::Solid) {
                struck = true;
            }
        }
    }
    for (const int d : doors) {
        const Door& door = world().doors()[static_cast<std::size_t>(d)];
        const float heroX = m_hero.body.box.center().x;
        bool opens = false;
        switch (door.kind) {
        case DoorKind::Plain: opens = true; break;
        case DoorKind::Key: opens = m_progress.hasKey(door.key); break;
        case DoorKind::OpensFromLeft: opens = heroX < door.box.center().x; break;
        case DoorKind::OpensFromRight: opens = heroX > door.box.center().x; break;
        case DoorKind::Escape: opens = false; break;
        }
        if (opens) {
            openDoor(d);
        } else {
            m_events.push({.kind = EventKind::DoorLocked, .at = door.box.center(), .value = d});
        }
    }
    if (struck && slash) {
        m_events.push({.kind = EventKind::SlashHit, .at = box.center()});
    }
}

void Session::stepShots()
{
    const gamelib::core::TileWorld tiles = tileWorld();
    m_shots.step(1.0f, gamelib::core::shotSolid(tiles), room().bounds.expanded(8.0f));
    for (const auto& removed : m_shots.removed()) {
        if (!removed.projectile.extra.enemy && removed.reason == gamelib::core::ProjectileEnd::HitSolid) {
            strikeTiles(removed.projectile.box().expanded(2.0f), false);
        }
        if (removed.projectile.extra.look == 5 && removed.reason != gamelib::core::ProjectileEnd::LeftBounds) {
            // A bomb bursts where it lands.
            m_events.push({.kind = EventKind::Bomb, .at = removed.projectile.position});
            if (gamelib::core::overlaps(Rect::fromCenter(removed.projectile.position, {36.0f, 36.0f}), m_hero.body.box)) {
                hurtHero(1, Rect::fromCenter(removed.projectile.position, {4.0f, 4.0f}));
            }
        }
    }
    // Enemy shots against the hero.
    while (const auto hit = m_shots.hitTest(m_hero.vulnerable)) {
        // tryHit started the invincibility: apply the rest by hand.
        m_hero.vulnerable.clearInvincibility();
        hurtHero(hit->landed.damage, hit->projectile.box());
        if (m_phase != Phase::Playing) {
            return;
        }
    }
}

void Session::dropLoot(const Vec2 at, const int chance)
{
    if (!m_rng.chance(static_cast<float>(chance) / 100.0f)) {
        return;
    }
    DropKind kind = DropKind::Health;
    if (m_hero.health.full() || (!m_hero.ammo.full() && m_rng.chance(0.5f))) {
        kind = DropKind::Ammo;
    }
    Drop d;
    d.kind = kind;
    d.body.box = Rect::fromCenter(at, {8.0f, 8.0f});
    d.body.velocity = {m_rng.uniform(-0.6f, 0.6f), -2.4f};
    d.previous = d.body.box;
    d.life = 9 * kTickRate;
    m_drops.push_back(d);
}

void Session::stepDrops()
{
    const gamelib::core::TileWorld tiles = tileWorld();
    for (Drop& d : m_drops) {
        --d.life;
        (void)gamelib::core::stepWalker(d.body, {.gravity = 0.18f, .maxFall = 3.0f}, tiles);
        d.body.velocity.x *= d.body.grounded ? 0.8f : 0.99f;
    }
    std::erase_if(m_drops, [&](const Drop& d) {
        if (d.life <= 0) {
            return true;
        }
        if (!gamelib::core::overlaps(d.body.box.expanded(3.0f), m_hero.body.box)) {
            return false;
        }
        if (d.kind == DropKind::Health) {
            m_hero.health.restore(1);
        } else {
            m_hero.ammo.restore(2);
        }
        m_events.push({.kind = EventKind::Drop, .at = d.body.box.center(), .value = static_cast<int>(d.kind)});
        return true;
    });
}

void Session::sealRoom(const bool sealed)
{
    gamelib::core::TileLayer& layer = m_map.layer(0);
    if (!sealed) {
        for (const Cell c : m_seals) {
            layer.at(c) = gamelib::core::charTile(' ');
        }
        m_seals.clear();
        return;
    }
    const gamelib::core::CellRange r = room().cells;
    const auto seal = [&](const Cell c) {
        if (layer.at(c) == ' ' || layer.at(c) == '~') {
            layer.at(c) = gamelib::core::charTile('Z');
            m_seals.push_back(c);
        }
    };
    for (int x = r.min.x; x <= r.max.x; ++x) {
        seal({x, r.min.y});
        seal({x, r.max.y});
    }
    for (int y = r.min.y + 1; y < r.max.y; ++y) {
        seal({r.min.x, y});
        seal({r.max.x, y});
    }
}

void Session::startEscape()
{
    m_escapeStart = m_ticks;
    for (std::size_t i = 0; i < world().doors().size(); ++i) {
        if (world().doors()[i].kind == DoorKind::Escape) {
            openDoor(static_cast<int>(i));
        }
    }
    m_events.push({.kind = EventKind::EscapeStart, .at = m_hero.body.box.center()});
}

void Session::grantAll()
{
    for (int a = 0; a < kAbilityCount; ++a) {
        m_progress.abilities.add(static_cast<Ability>(a));
    }
    m_progress.keys = 0x0F;
}

void Session::debugRevealAll()
{
    for (const RoomInfo& r : world().rooms()) {
        (void)m_progress.reveal.visitArea(r.def->corner, r.def->size);
    }
}

void Session::debugAbilityScene(const Ability ability)
{
    m_progress.abilities.add(ability);
    m_lastAbility = ability;
    m_phase = Phase::AbilityGet;
    m_phaseTicks = 0;
}

void Session::debugStartIn(const int room)
{
    if (room < 0 || room >= world().roomCount()) {
        return;
    }
    const RoomInfo& info = world().room(room);
    const gamelib::core::TileWorld tiles = tileWorld();
    for (int column = info.cells.min.x + 2; column < info.cells.max.x - 1; ++column) {
        if (const auto feet = gamelib::core::topFloor(tiles, column, info.cells.min.y + 1, info.cells.max.y)) {
            placeHero(*feet);
            enterRoom(room, false);
            return;
        }
    }
}

std::uint64_t Session::hash() const
{
    gamelib::core::StateHash h;
    h.add(m_hero.body.box.position()).add(m_hero.body.velocity).add(m_hero.health.current).add(m_hero.ammo.current);
    h.add(m_room).add(m_ticks).add(static_cast<int>(m_phase)).add(m_enemies.size());
    for (const Enemy& e : m_enemies) {
        h.add(e.body.box.position()).add(e.hp);
    }
    if (m_boss) {
        h.add(m_boss->body.box.position()).add(m_boss->hp);
    }
    return h.value();
}

} // namespace cavern::game
