// The hero: GameLib's platformer controller with the abilities found so
// far, swimming, glassy crystal that cannot be clung to, the slash (frame
// data from attack.hpp: forward, up, and down in the air with a pogo off
// whatever it strikes), the limited-ammo shot, the ground pound and thorns.
#include "session.hpp"

#include <gamelib/core/collision.hpp>

#include <algorithm>
#include <cmath>

namespace cavern::game {

namespace {

// helpers

using gamelib::core::Cell;
using gamelib::core::Rect;
using gamelib::core::TileId;
using gamelib::core::Vec2;

const gamelib::core::AttackClip kSlashForward{.startup = 2, .active = 5, .recovery = 9, .hitbox = {3.0f, -12.0f, 25.0f, 20.0f},
                               .damage = 1, .knockback = {1.6f, -1.0f}, .chainWindow = 4};
const gamelib::core::AttackClip kSlashUp{.startup = 2, .active = 5, .recovery = 9, .hitbox = {-12.0f, -36.0f, 24.0f, 26.0f},
                          .damage = 1, .knockback = {0.0f, -1.5f}, .chainWindow = 4};
const gamelib::core::AttackClip kSlashDown{.startup = 2, .active = 6, .recovery = 8, .hitbox = {-11.0f, 6.0f, 22.0f, 24.0f},
                            .damage = 1, .knockback = {0.0f, 1.5f}, .chainWindow = 4};

const gamelib::core::PoundTuning kPound{.hangTicks = kPoundHangTicks, .speed = kPoundSpeed, .recoveryTicks = kPoundRecoveryTicks};

TileId _tileAt(const gamelib::core::TileMap& map, const Vec2 point)
{
    const Cell c = gamelib::core::cellAt(point, kTile);
    return map.layer(0).inside(c) ? map.layer(0).at(c) : gamelib::core::charTile('#');
}

// Glassy crystal beside the box (within the wall-jump reach): no clinging.
bool _glassBeside(const gamelib::core::TileMap& map, const Rect& box)
{
    for (const float dx : {-4.0f, box.width + 4.0f}) {
        for (const float dy : {2.0f, box.height / 2.0f, box.height - 2.0f}) {
            if (_tileAt(map, {box.x + dx, box.y + dy}) == '*') {
                return true;
            }
        }
    }
    return false;
}

// Thorns under or beside the feet: not a safe place to come back to.
bool _nearThorns(const gamelib::core::TileMap& map, const Rect& box)
{
    for (const float dx : {-kTile, 0.0f, box.width / 2.0f, box.width, box.width + kTile}) {
        const TileId id = _tileAt(map, {box.x + dx, box.bottom() + 2.0f});
        if (id == '^' || id == 'L' || id == 'Q' || id == '=') {
            return true;
        }
    }
    return false;
}

} // namespace

void Session::stepHero(const Input& input)
{
    Hero& h = m_hero;
    gamelib::core::PlatformerBody& body = h.body;
    const gamelib::core::TileWorld world = tileWorld();
    const bool stunned = h.vulnerable.stunned();
    const Input in = stunned ? Input{.jumpHeld = input.jumpHeld} : input;
    h.hurtTicks = std::max(0, h.hurtTicks - 1);
    h.shootCooldown = std::max(0, h.shootCooldown - 1);
    h.shootPose = std::max(0, h.shootPose - 1);

    const bool wasSwimming = h.swimming;
    h.swimming = _tileAt(m_map, body.box.center()) == '~';
    if (h.swimming != wasSwimming) {
        m_events.push({.kind = EventKind::Splash, .at = {body.box.center().x, body.box.center().y}, .value = h.swimming ? 1 : 0});
    }

    bool handled = false;
    stepPound(in, handled);
    if (m_phase != Phase::Playing) {
        return;
    }
    if (!handled) {
        const bool cling = !_glassBeside(m_map, body.box);
        const gamelib::core::PlatformerTuning tuning = heroTuning(m_progress.abilities, h.swimming, cling);
        if (h.swimming) {
            body.airJumpsLeft = 99; // every press a stroke
        }
        const gamelib::core::PlatformerIntent intent{.move = static_cast<float>(in.move),
                                                     .climb = static_cast<float>(in.aim),
                                                     .jumpPressed = in.jumpPressed,
                                                     .jumpHeld = in.jumpHeld,
                                                     .dashPressed = in.dashPressed};
        const float xBefore = body.box.x;
        const gamelib::core::PlatformerStep step = gamelib::core::stepPlatformer(body, intent, tuning, world);
        // Breaking the surface with jump held: a leap out of the water onto the bank.
        if (h.swimming && _tileAt(m_map, body.box.center()) != '~' && body.velocity.y < 0.0f && in.jumpHeld) {
            body.velocity.y = std::min(body.velocity.y, -kWaterLeap);
            body.rising = false;
        }
        const Vec2 feet = body.box.bottomCenter();
        if (step.events.jumped) {
            m_events.push({.kind = EventKind::Jump, .at = feet});
        }
        if (step.events.airJumped) {
            m_events.push({.kind = h.swimming ? EventKind::Splash : EventKind::AirJump, .at = feet, .value = 2});
        }
        if (step.events.wallJumped) {
            m_events.push({.kind = EventKind::WallJump, .at = feet, .value = body.facing});
        }
        if (step.events.dashed) {
            m_events.push({.kind = EventKind::Dash, .at = body.box.center(), .value = body.facing});
        }
        if (step.events.landed) {
            m_events.push({.kind = EventKind::Land, .at = feet});
        }
        if (body.grounded) {
            h.walked += std::abs(body.box.x - xBefore);
            m_collapse.touch(gamelib::core::cellsBelow(body.box, kTile));
        }
        if (step.contacts.hazard) {
            if (m_map.layer(0).at(step.contacts.hazardCell) == 'L') {
                die();
            } else {
                hazardHit();
            }
            return;
        }
    }
    if (body.grounded && !_nearThorns(m_map, body.box) && !h.pound.active()) {
        h.safeFeet = body.box.bottomCenter();
        h.safeRoom = m_room;
    }
    h.vulnerable.setHurtbox(body.box.expanded(-1.5f));
    stepSlash(in);
    fire(in);
}

void Session::stepPound(const Input& in, bool& handled)
{
    Hero& h = m_hero;
    gamelib::core::PlatformerBody& body = h.body;
    if (!h.pound.active() && m_progress.abilities.has(Ability::GroundPound) && in.jumpPressed && in.aim > 0
        && !body.grounded && !h.slash.busy()) {
        if (h.pound.start(body)) {
            m_events.push({.kind = EventKind::Pound, .at = body.box.center()});
        }
    }
    if (!h.pound.active()) {
        return;
    }
    handled = true;
    const gamelib::core::PoundStep step = h.pound.step(body, kPound, tileWorld());
    if (step.contacts.hazard) {
        if (m_map.layer(0).at(step.contacts.hazardCell) == 'L') {
            die();
        } else {
            hazardHit();
        }
        return;
    }
    if (!step.landed) {
        return;
    }
    // Cracked floors under the feet give way, and the dive goes on.
    bool brokeFloor = false;
    for (int x = step.cells.min.x; x <= step.cells.max.x; ++x) {
        const Cell c{x, step.cells.min.y};
        if (m_map.layer(0).inside(c) && m_map.layer(0).at(c) == '%') {
            if (const int p = world().breakableAt(c); p >= 0) {
                breakPatch(p);
                brokeFloor = true;
            }
        }
    }
    const Rect blast = Rect::fromCenter(body.box.bottomCenter() - Vec2{0.0f, 8.0f}, {64.0f, 26.0f});
    m_events.push({.kind = EventKind::PoundLand, .at = body.box.bottomCenter(), .value = brokeFloor ? 1 : 0});
    for (Enemy& e : m_enemies) {
        if (e.hp > 0 && gamelib::core::overlaps(blast, e.body.box)) {
            enemyHurt(e, 3, e.body.box.center());
        }
    }
    if (m_boss && m_phase == Phase::Playing && gamelib::core::overlaps(blast, m_boss->vulnerable.hurtbox())) {
        bossHurt(3, m_boss->body.box.center());
    }
    if (brokeFloor) {
        h.pound.continueDive();
    }
}

void Session::stepSlash(const Input& in)
{
    Hero& h = m_hero;
    gamelib::core::PlatformerBody& body = h.body;
    if (in.slashPressed && !h.pound.active()) {
        const int aim = in.aim < 0 ? -1 : (in.aim > 0 && !body.grounded ? 1 : 0);
        const gamelib::core::AttackClip& clip = aim < 0 ? kSlashUp : (aim > 0 ? kSlashDown : kSlashForward);
        const gamelib::core::AttackPress press = h.slash.start(clip, body.facing);
        if (press == gamelib::core::AttackPress::Started) {
            h.slashAim = aim;
            m_events.push({.kind = EventKind::Slash, .at = body.box.center(), .value = aim});
        } else if (press == gamelib::core::AttackPress::Kept) {
            h.keptAim = aim;
        }
    }
    if (!h.slash.busy()) {
        return;
    }
    const gamelib::core::AttackStep swing = h.slash.tick(body.box.center());
    const std::optional<gamelib::core::Hit>& hit = swing.hit;
    if (swing.chained) {
        // A kept press started the next swing.
        h.slashAim = h.keptAim;
        m_events.push({.kind = EventKind::Slash, .at = body.box.center(), .value = h.slashAim});
    }
    if (!hit) {
        return;
    }
    bool pogo = false;
    for (Enemy& e : m_enemies) {
        if (e.hp <= 0) {
            continue;
        }
        if (const auto landed = e.vulnerable.tryHit(*hit)) {
            enemyHurt(e, landed->damage, e.body.box.center());
            if (e.hp > 0) {
                e.body.velocity.x += landed->knockback.x;
            }
            pogo = true;
        }
    }
    if (m_boss && m_phase == Phase::Playing) {
        if (const auto landed = m_boss->vulnerable.tryHit(*hit)) {
            bossHurt(landed->damage, hit->box.center());
            pogo = true;
        }
    }
    // A slash cuts enemy shots out of the air.
    const int cut = m_shots.removeIf([&](const Shot& s) { return s.extra.enemy && gamelib::core::overlaps(s.box(), hit->box); });
    if (cut > 0) {
        m_events.push({.kind = EventKind::SlashHit, .at = hit->box.center()});
        pogo = true;
    }
    if (hit->attackId != h.struckId) {
        h.struckId = hit->attackId; // doors and cracked walls once per swing
        strikeTiles(hit->box, true);
    }
    // Thorns under a down slash bounce the hero too.
    if (h.slashAim > 0) {
        const gamelib::core::CellRange r = gamelib::core::cellsOverlapping(hit->box, kTile);
        for (int y = r.min.y; y <= r.max.y && !pogo; ++y) {
            for (int x = r.min.x; x <= r.max.x; ++x) {
                const Cell c{x, y};
                if (m_map.layer(0).inside(c) && m_map.layer(0).at(c) == '^') {
                    pogo = true;
                    break;
                }
            }
        }
    }
    if (pogo && h.slashAim > 0 && !body.grounded) {
        gamelib::core::launch(body, {body.velocity.x, -kPogoSpeed}, true);
        body.dashReady = true;
        body.airJumpsLeft = heroTuning(m_progress.abilities, false, true).airJumps;
        m_events.push({.kind = EventKind::Pogo, .at = body.box.bottomCenter()});
    }
}

void Session::fire(const Input& in)
{
    Hero& h = m_hero;
    if (!in.shootPressed || h.shootCooldown > 0 || h.pound.active()) {
        return;
    }
    const gamelib::core::PlatformerBody& body = h.body;
    const auto dir = static_cast<float>(body.facing);
    if (!h.ammo.spend(1)) {
        h.shootCooldown = kShootCooldownTicks;
        m_events.push({.kind = EventKind::NoAmmo, .at = body.box.center()});
        return;
    }
    Shot s;
    const bool up = in.aim < 0;
    s.position = body.box.center() + (up ? Vec2{0.0f, -14.0f} : Vec2{dir * 10.0f, -2.0f});
    s.velocity = up ? Vec2{0.0f, -kShotSpeed} : Vec2{dir * kShotSpeed, 0.0f};
    s.size = up ? Vec2{6.0f, 12.0f} : Vec2{12.0f, 6.0f};
    s.team = 0;
    s.owner = 1;
    s.damage = 2;
    s.lifetimeTicks = 100;
    s.extra = {.enemy = false, .look = 0};
    if (m_shots.spawn(s) != 0) {
        h.shootCooldown = kShootCooldownTicks;
        h.shootPose = 12;
        m_events.push({.kind = EventKind::Shoot, .at = s.position, .value = up ? -1 : 0});
    } else {
        h.ammo.restore(1);
    }
}

} // namespace cavern::game
