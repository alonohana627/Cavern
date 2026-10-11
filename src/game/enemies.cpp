// The fifteen enemy kinds of the caves. Each is a small rule on a
// core::WalkerBody (walkers fall and turn at walls and ledges with
// core::stepWalker; fliers move through the tiles with core::moveBox) and a
// core::Vulnerable on team 1. They come back whenever their room is
// entered again (Session::enterRoom spawns the room's markers).
#include "session.hpp"

#include <gamelib/ai/opponent.hpp>
#include <gamelib/core/collision.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace cavern::game {

namespace {

// helpers

using gamelib::core::Rect;
using gamelib::core::Vec2;

constexpr float kPi = std::numbers::pi_v<float>;

struct KindInfo {
    Vec2 size;
    int hp = 1;
};

KindInfo _info(const EnemyKind kind)
{
    switch (kind) {
    case EnemyKind::Crawler: return {{14.0f, 10.0f}, 2};
    case EnemyKind::Bat: return {{12.0f, 10.0f}, 2};
    case EnemyKind::Sentry: return {{14.0f, 14.0f}, 3};
    case EnemyKind::SporePuff: return {{14.0f, 14.0f}, 1};
    case EnemyKind::Hopper: return {{14.0f, 12.0f}, 3};
    case EnemyKind::Creeper: return {{14.0f, 9.0f}, 2};
    case EnemyKind::Eel: return {{18.0f, 8.0f}, 2};
    case EnemyKind::Shellback: return {{18.0f, 14.0f}, 4};
    case EnemyKind::Jelly: return {{14.0f, 14.0f}, 2};
    case EnemyKind::Shardling: return {{14.0f, 12.0f}, 3};
    case EnemyKind::PrismEye: return {{16.0f, 16.0f}, 4};
    case EnemyKind::Moth: return {{12.0f, 10.0f}, 2};
    case EnemyKind::Wisp: return {{10.0f, 10.0f}, 2};
    case EnemyKind::Drone: return {{16.0f, 12.0f}, 3};
    case EnemyKind::Lobber: return {{16.0f, 16.0f}, 4};
    case EnemyKind::Count: break;
    }
    return {{12.0f, 12.0f}, 1};
}

// Moves a flier through the tiles (walls stop it); true if something blocked it.
bool _fly(gamelib::core::WalkerBody& body, const Vec2 delta, const gamelib::core::TileWorld& world)
{
    const gamelib::core::BoxMove move = gamelib::core::moveBox(body.box, delta, world, {.passOneWay = true});
    body.box = move.box;
    return move.contacts.blockedX || move.contacts.blockedY;
}

int _sign(const float v)
{
    return v > 0.0f ? 1 : (v < 0.0f ? -1 : 0);
}

} // namespace

void Session::spawnEnemy(const SpawnDef& spawn)
{
    const KindInfo info = _info(spawn.kind);
    Enemy e;
    e.kind = spawn.kind;
    const Vec2 feet = m_map.cellBottom(spawn.cell);
    switch (spawn.kind) {
    case EnemyKind::Creeper:
        // Hangs under the ceiling above its marker.
        e.body.box = Rect{feet.x - info.size.x / 2.0f, feet.y - kTile, info.size.x, info.size.y};
        break;
    case EnemyKind::Bat:
    case EnemyKind::SporePuff:
    case EnemyKind::Jelly:
    case EnemyKind::PrismEye:
    case EnemyKind::Moth:
    case EnemyKind::Wisp:
    case EnemyKind::Drone:
    case EnemyKind::Eel: e.body.box = Rect::fromCenter(m_map.cellCenter(spawn.cell), info.size); break;
    default: e.body.box = Rect::fromBottomCenter(feet, info.size); break;
    }
    e.previous = e.body.box;
    e.origin = e.body.box.center();
    e.hp = info.hp;
    e.maxHp = info.hp;
    e.seed = static_cast<int>(m_rng.below(1000));
    e.timer = static_cast<int>(m_rng.below(60));
    e.body.direction = m_hero.body.box.center().x < e.origin.x ? -1 : 1;
    e.vulnerable = gamelib::core::Vulnerable{e.body.box, 1, 6, 0};
    m_enemies.push_back(e);
}

void Session::stepEnemies()
{
    for (Enemy& e : m_enemies) {
        if (e.hp <= 0) {
            continue;
        }
        e.vulnerable.tick();
        ++e.age;
        ++e.timer;
        e.flash = std::max(0, e.flash - 1);
        stepEnemy(e);
        e.vulnerable.setHurtbox(e.body.box);
        // The hero's shots: a shellback's shell turns them aside.
        if (!m_shots.deflectFrom(e.vulnerable).empty()) {
            m_events.push({.kind = EventKind::Deflect, .at = e.body.box.center()});
        }
        while (e.hp > 0) {
            const auto hit = m_shots.hitTest(e.vulnerable);
            if (!hit) {
                break;
            }
            enemyHurt(e, hit->landed.damage, hit->projectile.position);
        }
    }
    std::erase_if(m_enemies, [](const Enemy& e) { return e.hp <= 0; });
}

void Session::stepEnemy(Enemy& e)
{
    const gamelib::core::TileWorld world = tileWorld();
    const Vec2 hero = m_hero.body.box.center();
    const Vec2 me = e.body.box.center();
    const Vec2 toHero = hero - me;
    const float distance = gamelib::core::length(toHero);
    const auto inWater = [&](const Vec2 p) {
        const gamelib::core::Cell c = gamelib::core::cellAt(p, kTile);
        return m_map.layer(0).inside(c) && m_map.layer(0).at(c) == '~';
    };
    switch (e.kind) {
    case EnemyKind::Crawler:
        (void)gamelib::core::stepWalker(e.body, {.gravity = 0.2f, .maxFall = 4.0f, .speed = 0.45f, .turnAtLedges = true}, world);
        break;
    case EnemyKind::Bat:
        if (e.state == 0) {
            if (distance < 96.0f) {
                e.state = 1;
                e.timer = 0;
            }
        } else {
            const Vec2 dir = gamelib::core::normalized(toHero);
            const float wobble = std::sin(static_cast<float>(e.timer) * 0.15f) * 1.2f;
            (void)_fly(e.body, Vec2{dir.x * 1.0f, dir.y * 0.8f + wobble}, world);
            if (distance > 260.0f) {
                e.state = 0;
            }
        }
        break;
    case EnemyKind::Sentry:
        e.body.direction = _sign(toHero.x) != 0 ? _sign(toHero.x) : e.body.direction;
        if (e.timer >= 100 && distance < 220.0f) {
            e.timer = 0;
            enemyFire(me, gamelib::core::normalized(toHero) * 2.1f, 1, 1);
        }
        break;
    case EnemyKind::SporePuff: {
        const float t = static_cast<float>(e.age + e.seed);
        const Vec2 goal = e.origin + Vec2{std::clamp(toHero.x, -40.0f, 40.0f), std::sin(t * 0.04f) * 10.0f};
        (void)_fly(e.body, (goal - me) * 0.03f, world);
        break;
    }
    case EnemyKind::Hopper: {
        const gamelib::core::WalkerStep step =
            gamelib::core::stepWalker(e.body, {.gravity = 0.2f, .maxFall = 4.5f}, world);
        if (e.body.grounded) {
            e.body.velocity.x = 0.0f;
            if (e.timer >= 55 && distance < 200.0f) {
                e.timer = 0;
                e.body.direction = _sign(toHero.x) != 0 ? _sign(toHero.x) : e.body.direction;
                e.body.velocity = {static_cast<float>(e.body.direction) * 1.3f, -3.6f};
                e.body.grounded = false;
            }
        }
        (void)step;
        break;
    }
    case EnemyKind::Creeper:
        if (e.state == 0) {
            // Along the ceiling, turning where it ends or at a wall.
            const Rect above = Rect{e.body.box.x, e.body.box.y - 2.0f, e.body.box.width, 2.0f};
            const bool ceiling = gamelib::core::moveBox(e.body.box, {0.0f, -1.0f}, world).contacts.blockedY;
            (void)above;
            const float ahead = static_cast<float>(e.body.direction) * 0.4f;
            Rect next = e.body.box;
            next.x += ahead + static_cast<float>(e.body.direction) * e.body.box.width / 2.0f;
            const bool ceilingAhead = gamelib::core::moveBox(next, {0.0f, -1.0f}, world).contacts.blockedY;
            if (!ceiling || !ceilingAhead || gamelib::core::wallAhead(e.body.box, e.body.direction, world)) {
                e.body.direction = -e.body.direction;
            } else {
                e.body.box.x += ahead;
            }
            if (std::abs(toHero.x) < 14.0f && toHero.y > 0.0f && toHero.y < 200.0f) {
                e.state = 1;
                e.body.velocity = {};
            }
        } else {
            (void)gamelib::core::stepWalker(e.body, {.gravity = 0.25f, .maxFall = 5.0f, .speed = e.body.grounded ? 0.6f : 0.0f,
                                                     .turnAtLedges = true},
                                            world);
        }
        break;
    case EnemyKind::Eel:
        if (e.state == 0) {
            const float t = static_cast<float>(e.age + e.seed);
            e.body.box = Rect::fromCenter(e.origin + Vec2{std::sin(t * 0.03f) * 18.0f, 0.0f}, e.body.box.size());
            e.body.direction = std::cos(static_cast<float>(e.age + e.seed) * 0.03f) > 0.0f ? 1 : -1;
            if (e.timer >= 110 && distance < 180.0f) {
                e.state = 1;
                e.timer = 0;
                e.body.velocity = {std::clamp(toHero.x / 60.0f, -1.4f, 1.4f), -4.4f};
            }
        } else {
            const bool wet = inWater(me);
            e.body.velocity.y += wet ? 0.06f : 0.16f;
            e.body.box.x += e.body.velocity.x;
            e.body.box.y += e.body.velocity.y;
            if (e.body.velocity.y > 0.0f && e.body.box.center().y >= e.origin.y) {
                e.state = 0;
                e.timer = 0;
                e.origin.x = std::clamp(e.body.box.center().x, e.origin.x - 40.0f, e.origin.x + 40.0f);
            }
        }
        break;
    case EnemyKind::Shellback: {
        if (e.timer % 90 == 0 && std::abs(toHero.x) < 160.0f) {
            e.body.direction = _sign(toHero.x) != 0 ? _sign(toHero.x) : e.body.direction;
        }
        (void)gamelib::core::stepWalker(e.body, {.gravity = 0.2f, .maxFall = 4.0f, .speed = 0.3f, .turnAtLedges = true}, world);
        const bool inFront = _sign(toHero.x) == e.body.direction && toHero.y > -10.0f;
        e.vulnerable.setGuarded(inFront);
        break;
    }
    case EnemyKind::Jelly: {
        const float t = static_cast<float>(e.age + e.seed);
        const Vec2 drift = distance < 200.0f ? gamelib::core::normalized(toHero) * 0.35f : Vec2{};
        (void)_fly(e.body, drift + Vec2{0.0f, std::sin(t * 0.05f) * 0.4f}, world);
        break;
    }
    case EnemyKind::Shardling:
        if (e.state == 0) {
            (void)gamelib::core::stepWalker(e.body, {.gravity = 0.2f, .maxFall = 4.0f, .speed = 0.35f, .turnAtLedges = true},
                                            world);
            if (std::abs(toHero.y) < 20.0f && std::abs(toHero.x) < 150.0f && _sign(toHero.x) == e.body.direction
                && e.timer > 30) {
                e.state = 1;
                e.timer = 0;
            }
        } else if (e.state == 1) {
            // A short wind-up, then the charge.
            if (e.timer > 18) {
                const gamelib::core::WalkerStep step = gamelib::core::stepWalker(
                    e.body, {.gravity = 0.2f, .maxFall = 4.0f, .speed = 2.6f, .turnAtLedges = true}, world);
                if (step.turned || e.timer > 110) {
                    e.state = 2;
                    e.timer = 0;
                }
            }
        } else if (e.timer > 40) {
            e.state = 0;
            e.timer = 0;
        }
        break;
    case EnemyKind::PrismEye: {
        const float t = static_cast<float>(e.age + e.seed) * 0.02f;
        const Vec2 goal = e.origin + Vec2{std::sin(t) * 28.0f, std::sin(2.0f * t) * 12.0f};
        (void)_fly(e.body, (goal - me) * 0.1f, world);
        if (e.timer >= 140 && distance < 240.0f) {
            e.timer = 0;
            const Vec2 aim = gamelib::core::normalized(toHero);
            for (int i = 0; i < 3; ++i) {
                enemyFire(me, gamelib::core::fanDirection(aim, i, 3, 0.28f) * 2.3f, 4, 1);
            }
        }
        break;
    }
    case EnemyKind::Moth: {
        const float t = static_cast<float>(e.age + e.seed);
        if (distance < 220.0f) {
            e.body.velocity = gamelib::core::approach(e.body.velocity, gamelib::core::normalized(toHero) * 0.8f, 0.03f);
        } else {
            e.body.velocity = gamelib::core::approach(e.body.velocity, (e.origin - me) * 0.01f, 0.03f);
        }
        (void)_fly(e.body, e.body.velocity + Vec2{0.0f, std::sin(t * 0.12f) * 0.5f}, world);
        break;
    }
    case EnemyKind::Wisp:
        if (e.state == 0) {
            const float a = static_cast<float>(e.age + e.seed) * 0.05f;
            const Vec2 goal = e.origin + Vec2{std::cos(a), std::sin(a)} * 22.0f;
            (void)_fly(e.body, (goal - me) * 0.2f, world);
            if (e.timer >= 150 && distance < 180.0f) {
                e.state = 1;
                e.timer = 0;
                e.body.velocity = gamelib::core::normalized(toHero) * 2.6f;
            }
        } else if (e.state == 1) {
            if (_fly(e.body, e.body.velocity, world) || e.timer > 45) {
                e.state = 2;
                e.timer = 0;
            }
        } else {
            (void)_fly(e.body, (e.origin - me) * 0.05f, world);
            if (gamelib::core::length(e.origin - me) < 4.0f || e.timer > 120) {
                e.state = 0;
                e.timer = 0;
            }
        }
        break;
    case EnemyKind::Drone: {
        const Vec2 goal{hero.x, std::min(e.origin.y, hero.y - 72.0f)};
        const Vec2 delta = goal - me;
        (void)_fly(e.body, {std::clamp(delta.x * 0.04f, -0.9f, 0.9f), std::clamp(delta.y * 0.04f, -0.6f, 0.6f)}, world);
        if (e.timer >= 110 && std::abs(toHero.x) < 30.0f && toHero.y > 0.0f) {
            e.timer = 0;
            enemyFire(me + Vec2{0.0f, 6.0f}, {0.0f, 0.5f}, 5, 1, 0.12f);
        }
        break;
    }
    case EnemyKind::Lobber:
        (void)gamelib::core::stepWalker(e.body, {.gravity = 0.2f, .maxFall = 4.0f}, world);
        e.body.direction = _sign(toHero.x) != 0 ? _sign(toHero.x) : e.body.direction;
        if (e.timer >= 120 && distance < 240.0f) {
            e.timer = 0;
            const Vec2 from = me - Vec2{0.0f, 8.0f};
            const Vec2 v = gamelib::ai::lobVelocity(from, hero, 0.12f, 60, {3.0f, 5.0f});
            enemyFire(from, v, 6, 1, 0.12f);
        }
        break;
    case EnemyKind::Count: break;
    }
}

void Session::heroMeetsEnemies()
{
    if (m_phase != Phase::Playing) {
        return;
    }
    for (const Enemy& e : m_enemies) {
        if (e.hp > 0 && gamelib::core::overlaps(e.body.box.expanded(-1.5f), m_hero.vulnerable.hurtbox())) {
            hurtHero(1, e.body.box);
            if (m_phase != Phase::Playing) {
                return;
            }
        }
    }
}

void Session::enemyHurt(Enemy& e, const int damage, const Vec2 at)
{
    if (e.hp <= 0) {
        return;
    }
    e.hp -= damage;
    e.flash = 8;
    m_events.push({.kind = EventKind::EnemyHit, .at = at, .enemy = e.kind});
    if (e.hp > 0) {
        return;
    }
    const Vec2 center = e.body.box.center();
    m_events.push({.kind = EventKind::EnemyDown, .at = center, .value = 0, .enemy = e.kind});
    if (e.kind == EnemyKind::SporePuff) {
        // Bursts into a ring of slow spores.
        for (int i = 0; i < 6; ++i) {
            const float a = static_cast<float>(i) * kPi / 3.0f + 0.3f;
            enemyFire(center, Vec2{std::cos(a), std::sin(a)} * 1.0f, 2, 1);
        }
    }
    dropLoot(center, 35);
}

void Session::enemyFire(const Vec2 at, const Vec2 velocity, const int look, const int damage, const float gravity)
{
    Shot s;
    s.position = at;
    s.velocity = velocity;
    s.size = look == 4 ? Vec2{8.0f, 8.0f} : (look == 8 ? Vec2{12.0f, 12.0f} : Vec2{7.0f, 7.0f});
    s.team = 1;
    s.owner = 100;
    s.damage = damage;
    s.gravity = gravity;
    s.lifetimeTicks = look == 2 ? 70 : 240;
    s.tileContact = gamelib::core::TileContact::Remove;
    s.extra = {.enemy = true, .look = look};
    if (m_shots.spawn(s) != 0) {
        m_events.push({.kind = EventKind::EnemyShot, .at = at, .value = look});
    }
}

} // namespace cavern::game
