// The guardians: one per area, each guarding an ability, and the molten
// heart of the core. Each is a GameLib StateMachine (idle, a wind-up whose
// "tell" the drawing shows, the attack, a recovery) with PhaseByHealth
// making it faster and adding patterns as it weakens; patterns are chosen
// with nextPattern (never the same twice in a row). Strikes from above are
// announced with core::Telegraphs before they fall.
//
//   the Sporemother   - spore volleys, leaps that send spores along the floor, puffs budding
//   the Tidewarden    - claw charges across the pool, bubble spit, geysers under the hero
//   the Gate Colossus - a slow walk, fist slams with shockwaves, rubble raining from the vault
//   the Prism Wyrm    - flies a figure of eight, shard fans, dives at the hero, falling shards
//   the Molten Heart  - shielded by plates; opens to fire, rains magma, rings of orbs
#include "session.hpp"

#include <gamelib/ai/opponent.hpp>
#include <gamelib/core/collision.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace cavern::game {

namespace {

// helpers

using gamelib::core::Rect;
using gamelib::core::Vec2;

constexpr float kPi = std::numbers::pi_v<float>;

struct BossInfo {
    Vec2 size;
    int hp = 30;
    bool flying = false;
};

BossInfo _info(const BossKind kind)
{
    switch (kind) {
    case BossKind::Sporemother: return {{40.0f, 36.0f}, 20, false};
    case BossKind::Tidewarden: return {{44.0f, 30.0f}, 30, false};
    case BossKind::Colossus: return {{40.0f, 52.0f}, 36, false};
    case BossKind::PrismWyrm: return {{34.0f, 30.0f}, 30, true};
    case BossKind::Heart: return {{56.0f, 56.0f}, 36, true};
    case BossKind::Count: break;
    }
    return {{32.0f, 32.0f}, 20, false};
}

int _sign(const float v)
{
    return v > 0.0f ? 1 : (v < 0.0f ? -1 : 0);
}

// Faster as the phases advance: 100%, 80%, 65% of the base ticks.
int _pace(const Boss& b, const int ticks)
{
    static constexpr std::array<int, 3> kPercent{100, 80, 65};
    return ticks * kPercent[static_cast<std::size_t>(std::clamp(b.phases.phase(), 0, 2))] / 100;
}

void _tell(Boss& b, const int ticksIn, const int windUp)
{
    b.tell = std::clamp(ticksIn * 255 / std::max(windUp, 1), 0, 255);
}

} // namespace

void Session::wakeBoss()
{
    const int index = room().boss;
    const BossDef& def = world().bosses()[static_cast<std::size_t>(index)];
    const BossInfo info = _info(def.kind);
    Boss b;
    b.kind = def.kind;
    b.def = index;
    b.home = m_map.cellBottom(def.cell);
    b.body.box = info.flying ? Rect::fromCenter(m_map.cellCenter(def.cell), info.size) : Rect::fromBottomCenter(b.home, info.size);
    b.body.direction = m_hero.body.box.center().x < b.body.box.center().x ? -1 : 1;
    b.previous = b.body.box;
    b.hp = info.hp;
    b.maxHp = info.hp;
    b.flying = info.flying;
    b.vulnerable = gamelib::core::Vulnerable{b.body.box, 1, 10, 0};
    b.sm.enter(BossState::Intro);
    m_boss = b;
    sealRoom(true);
    m_hero.body.velocity.x = 0.0f;
    m_hero.slash.cancel();
    m_phase = Phase::BossIntro;
    m_phaseTicks = 0;
    m_events.push({.kind = EventKind::BossWake, .at = b.body.box.center(), .boss = b.kind});
}

void Session::stepBoss()
{
    if (!m_boss) {
        return;
    }
    Boss& b = *m_boss;
    b.vulnerable.tick();
    b.flash = std::max(0, b.flash - 1);
    if (m_phase == Phase::BossIntro) {
        b.sm.tick();
        return;
    }
    if (b.sm.is(BossState::Intro)) {
        b.sm.enter(BossState::Idle);
    }
    // Strikes announced earlier fall now: a shard, a rock or a gout of magma from the vault.
    for (const Vec2 at : m_warnings.tick()) {
        const int look = b.kind == BossKind::Heart ? 6 : 7;
        Shot s;
        s.position = {at.x, room().bounds.top() + 20.0f};
        s.velocity = {0.0f, 2.0f};
        s.gravity = 0.25f;
        s.size = {12.0f, 14.0f};
        s.team = 1;
        s.owner = 101;
        s.damage = 1;
        s.lifetimeTicks = 200;
        s.extra = {.enemy = true, .look = look};
        (void)m_shots.spawn(s);
        m_events.push({.kind = EventKind::Strike, .at = at});
    }
    switch (b.kind) {
    case BossKind::Sporemother: stepSporemother(b); break;
    case BossKind::Tidewarden: stepTidewarden(b); break;
    case BossKind::Colossus: stepColossus(b); break;
    case BossKind::PrismWyrm: stepPrismWyrm(b); break;
    case BossKind::Heart: stepHeart(b); break;
    case BossKind::Count: break;
    }
    if (!m_boss) {
        return;
    }
    b.sm.tick();
    b.vulnerable.setHurtbox(b.body.box.expanded(-2.0f));
    if (m_phase != Phase::Playing) {
        return;
    }
    if (!m_shots.deflectFrom(b.vulnerable).empty()) {
        m_events.push({.kind = EventKind::Deflect, .at = b.body.box.center()});
    }
    while (const auto hit = m_shots.hitTest(b.vulnerable)) {
        bossHurt(hit->landed.damage, hit->projectile.position);
        if (!m_boss || m_phase != Phase::Playing) {
            return;
        }
    }
    if (gamelib::core::overlaps(b.body.box.expanded(-4.0f), m_hero.vulnerable.hurtbox())) {
        hurtHero(1, b.body.box);
    }
}

void Session::bossHurt(const int damage, const Vec2 at)
{
    if (!m_boss || m_boss->hp <= 0) {
        return;
    }
    Boss& b = *m_boss;
    if (b.vulnerable.guarded()) {
        m_events.push({.kind = EventKind::Deflect, .at = at});
        return;
    }
    b.hp = std::max(0, b.hp - damage);
    b.shown = b.hp;
    b.flash = 8;
    m_events.push({.kind = EventKind::BossHit, .at = at, .value = b.hp, .boss = b.kind});
    if (b.phases.update(b.hp, b.maxHp)) {
        m_events.push({.kind = EventKind::BossPhase, .at = b.body.box.center(), .value = b.phases.phase(), .boss = b.kind});
    }
    if (b.hp <= 0) {
        b.sm.enter(BossState::Dead);
        b.tell = 0;
        b.vulnerable.setGuarded(false);
        m_shots.removeIf([](const Shot& s) { return s.extra.enemy; });
        m_warnings.clear();
        for (Enemy& e : m_enemies) {
            e.hp = 0; // its brood falls with it
        }
        m_phase = Phase::BossDown;
        m_phaseTicks = 0;
    }
}

// The Sporemother: a bloated mushroom matron on root legs.
void Session::stepSporemother(Boss& b)
{
    const gamelib::core::TileWorld world = tileWorld();
    const Vec2 hero = m_hero.body.box.center();
    const Vec2 me = b.body.box.center();
    const gamelib::core::WalkerStep step = gamelib::core::stepWalker(b.body, {.gravity = 0.25f, .maxFall = 6.0f}, world);
    if (step.landed && b.sm.is(BossState::Attack2)) {
        // The landing sends spores rolling along the floor both ways.
        m_events.push({.kind = EventKind::BossLand, .at = b.body.box.bottomCenter(), .boss = b.kind});
        const float y = b.body.box.bottom() - 6.0f;
        for (const int d : {-1, 1}) {
            enemyFire({me.x + static_cast<float>(d) * 18.0f, y}, {static_cast<float>(d) * 2.0f, 0.0f}, 2, 1);
        }
        b.body.velocity.x = 0.0f;
        b.sm.enter(BossState::Recover);
        return;
    }
    switch (b.sm.current()) {
    case BossState::Idle:
        b.tell = 0;
        if (b.body.grounded) {
            b.body.velocity.x = 0.0f;
            b.body.direction = _sign(hero.x - me.x) != 0 ? _sign(hero.x - me.x) : b.body.direction;
        }
        if (b.sm.after(_pace(b, 50))) {
            const int count = b.phases.phase() >= 1 ? 3 : 2;
            b.pattern = gamelib::core::nextPattern(m_rng, count, b.pattern);
            b.sm.enter(b.pattern == 0 ? BossState::Attack : (b.pattern == 1 ? BossState::Move : BossState::Attack3));
        }
        break;
    case BossState::Attack: // a volley of spores in a fan towards the hero
        _tell(b, b.sm.ticksIn(), 34);
        if (b.sm.ticksIn() == 34) {
            const int shots = b.phases.phase() >= 2 ? 7 : 5;
            const Vec2 aim = gamelib::core::normalized(Vec2{hero.x - me.x, -std::abs(hero.x - me.x) * 0.6f - 40.0f});
            for (int i = 0; i < shots; ++i) {
                const Vec2 dir = gamelib::core::fanDirection(aim, i, shots, 0.22f);
                enemyFire(me - Vec2{0.0f, 12.0f}, dir * 2.6f, 2, 1, 0.05f);
            }
            b.tell = 0;
        }
        if (b.sm.after(50)) {
            b.sm.enter(BossState::Recover);
        }
        break;
    case BossState::Move: // the wind-up of a leap
        _tell(b, b.sm.ticksIn(), 26);
        if (b.sm.ticksIn() == 26) {
            const Rect arena = room().bounds.expanded(-2.0f * kTile);
            const Vec2 target{std::clamp(hero.x, arena.left() + 24.0f, arena.right() - 24.0f), b.body.box.bottom()};
            b.body.velocity = gamelib::ai::leapVelocity(b.body.box.bottomCenter(), target, 5.2f, 0.25f, 3.0f);
            b.body.grounded = false;
            b.tell = 0;
            b.sm.enter(BossState::Attack2);
        }
        break;
    case BossState::Attack2: // in the air; the landing above ends it
        if (b.sm.after(200)) {
            b.sm.enter(BossState::Recover);
        }
        break;
    case BossState::Attack3: // two spore puffs bud from her cap
        _tell(b, b.sm.ticksIn(), 30);
        if (b.sm.ticksIn() == 30) {
            int puffs = 0;
            for (const Enemy& e : m_enemies) {
                puffs += e.kind == EnemyKind::SporePuff && e.hp > 0 ? 1 : 0;
            }
            for (int i = 0; i < 2 && puffs < 3; ++i, ++puffs) {
                const gamelib::core::Cell c = gamelib::core::cellAt(me + Vec2{(i == 0 ? -1.0f : 1.0f) * 30.0f, -34.0f}, kTile);
                spawnEnemy({.kind = EnemyKind::SporePuff, .cell = c});
            }
            b.tell = 0;
        }
        if (b.sm.after(45)) {
            b.sm.enter(BossState::Recover);
        }
        break;
    case BossState::Recover:
        b.tell = 0;
        if (b.body.grounded) {
            b.body.velocity.x = 0.0f;
        }
        if (b.sm.after(_pace(b, 34))) {
            b.sm.enter(BossState::Idle);
        }
        break;
    default: break;
    }
}

// The Tidewarden: an armoured crab-knight with one great claw.
void Session::stepTidewarden(Boss& b)
{
    const gamelib::core::TileWorld world = tileWorld();
    const Vec2 hero = m_hero.body.box.center();
    const Vec2 me = b.body.box.center();
    const gamelib::core::WalkerStep step = gamelib::core::stepWalker(b.body, {.gravity = 0.25f, .maxFall = 6.0f}, world);
    switch (b.sm.current()) {
    case BossState::Idle:
        b.tell = 0;
        b.body.velocity.x = 0.0f;
        b.body.direction = _sign(hero.x - me.x) != 0 ? _sign(hero.x - me.x) : b.body.direction;
        if (b.sm.after(_pace(b, 46))) {
            b.pattern = gamelib::core::nextPattern(m_rng, 3, b.pattern);
            b.sm.enter(b.pattern == 0 ? BossState::Move : (b.pattern == 1 ? BossState::Attack : BossState::Attack2));
            b.count = 0;
        }
        break;
    case BossState::Move: // raises the claw, then charges across the floor
        if (b.sm.ticksIn() < 34) {
            _tell(b, b.sm.ticksIn(), 34);
            b.body.velocity.x = 0.0f;
            break;
        }
        b.tell = 255;
        b.body.velocity.x = static_cast<float>(b.body.direction) * (b.phases.phase() >= 2 ? 3.6f : 3.0f);
        if (step.contacts.blockedX || b.sm.after(150)) {
            m_events.push({.kind = EventKind::BossLand, .at = me, .boss = b.kind});
            b.body.velocity.x = 0.0f;
            ++b.count;
            if (b.phases.phase() >= 1 && b.count < 2) {
                b.body.direction = -b.body.direction;
                b.sm.enter(BossState::Move); // a second charge back
            } else {
                b.sm.enter(BossState::Recover);
            }
        }
        break;
    case BossState::Attack: // bubbles drifting towards the hero on a wave
        _tell(b, b.sm.ticksIn(), 24);
        if (b.sm.ticksIn() >= 24 && (b.sm.ticksIn() - 24) % 14 == 0 && b.count < (b.phases.phase() >= 1 ? 5 : 3)) {
            const Vec2 dir = gamelib::core::normalized(hero - me);
            Shot s;
            s.position = me + Vec2{static_cast<float>(b.body.direction) * 16.0f, -8.0f};
            s.velocity = dir * 1.6f;
            s.size = {10.0f, 10.0f};
            s.team = 1;
            s.owner = 101;
            s.motion = gamelib::core::ProjectileMotion::Sine;
            s.sineAmplitude = 10.0f;
            s.sinePeriodTicks = 50;
            s.lifetimeTicks = 220;
            s.extra = {.enemy = true, .look = 3};
            if (m_shots.spawn(s) != 0) {
                m_events.push({.kind = EventKind::EnemyShot, .at = s.position, .value = 3});
            }
            ++b.count;
        }
        if (b.sm.after(24 + 14 * 5 + 10)) {
            b.sm.enter(BossState::Recover);
        }
        break;
    case BossState::Attack2: // geysers burst from the floor under the hero
        _tell(b, b.sm.ticksIn(), 20);
        if (b.sm.ticksIn() == 20) {
            const float floor = b.home.y - 14.0f;
            m_warnings.add({hero.x, floor}, 40);
            if (b.phases.phase() >= 1) {
                m_warnings.add({hero.x - 48.0f, floor}, 52);
                m_warnings.add({hero.x + 48.0f, floor}, 52);
            }
            b.tell = 0;
        }
        if (b.sm.after(70)) {
            b.sm.enter(BossState::Recover);
        }
        break;
    case BossState::Recover:
        b.tell = 0;
        b.body.velocity.x = 0.0f;
        if (b.sm.after(_pace(b, 40))) {
            b.sm.enter(BossState::Idle);
        }
        break;
    default: break;
    }
}

// The Gate Colossus: a slow stone golem with a slit of light for a face.
void Session::stepColossus(Boss& b)
{
    const gamelib::core::TileWorld world = tileWorld();
    const Vec2 hero = m_hero.body.box.center();
    const Vec2 me = b.body.box.center();
    const gamelib::core::WalkerStep step = gamelib::core::stepWalker(b.body, {.gravity = 0.3f, .maxFall = 7.0f}, world);
    if (step.landed && b.sm.is(BossState::Attack3)) {
        m_events.push({.kind = EventKind::BossLand, .at = b.body.box.bottomCenter(), .boss = b.kind});
        for (const int d : {-1, 1}) {
            enemyFire({me.x + static_cast<float>(d) * 22.0f, b.body.box.bottom() - 7.0f}, {static_cast<float>(d) * 2.4f, 0.0f}, 7, 1);
        }
        b.body.velocity.x = 0.0f;
        b.sm.enter(BossState::Recover);
        return;
    }
    switch (b.sm.current()) {
    case BossState::Idle:
    case BossState::Move: {
        b.tell = 0;
        b.body.direction = _sign(hero.x - me.x) != 0 ? _sign(hero.x - me.x) : b.body.direction;
        b.body.velocity.x = std::abs(hero.x - me.x) > 40.0f ? static_cast<float>(b.body.direction) * 0.45f : 0.0f;
        if (b.sm.after(_pace(b, 70))) {
            const int count = b.phases.phase() >= 1 ? 3 : 2;
            b.pattern = gamelib::core::nextPattern(m_rng, count, b.pattern);
            b.body.velocity.x = 0.0f;
            if (b.pattern == 2) {
                // A leap at the hero (from phase two).
                b.body.velocity = gamelib::ai::leapVelocity(b.body.box.bottomCenter(), {hero.x, b.body.box.bottom()}, 6.0f, 0.3f, 2.4f);
                b.body.grounded = false;
                b.sm.enter(BossState::Attack3);
            } else {
                b.sm.enter(b.pattern == 0 ? BossState::Attack : BossState::Attack2);
            }
        }
        break;
    }
    case BossState::Attack: // the fist rises, then slams: shockwaves both ways
        b.body.velocity.x = 0.0f;
        _tell(b, b.sm.ticksIn(), 40);
        if (b.sm.ticksIn() == 40) {
            m_events.push({.kind = EventKind::BossLand, .at = b.body.box.bottomCenter(), .boss = b.kind});
            const float y = b.body.box.bottom() - 7.0f;
            for (const int d : {-1, 1}) {
                enemyFire({me.x + static_cast<float>(d) * 26.0f, y}, {static_cast<float>(d) * 2.6f, 0.0f}, 7, 1);
            }
            if (b.phases.phase() >= 2) {
                for (const int d : {-1, 1}) {
                    enemyFire({me.x + static_cast<float>(d) * 26.0f, y - 22.0f}, {static_cast<float>(d) * 1.8f, 0.0f}, 7, 1);
                }
            }
            b.tell = 0;
        }
        if (b.sm.after(64)) {
            b.sm.enter(BossState::Recover);
        }
        break;
    case BossState::Attack2: // rubble shaken loose from the vault
        b.body.velocity.x = 0.0f;
        _tell(b, b.sm.ticksIn(), 24);
        if (b.sm.ticksIn() == 24) {
            const float floor = b.home.y - 14.0f;
            const int rocks = b.phases.phase() >= 1 ? 5 : 3;
            const Rect arena = room().bounds.expanded(-2.0f * kTile);
            for (int i = 0; i < rocks; ++i) {
                const float x = std::clamp(hero.x + gamelib::core::centeredOffset(i, rocks, 44.0f) + m_rng.uniform(-8.0f, 8.0f),
                                           arena.left(), arena.right());
                m_warnings.add({x, floor}, 46 + i * 6);
            }
            b.tell = 0;
        }
        if (b.sm.after(80)) {
            b.sm.enter(BossState::Recover);
        }
        break;
    case BossState::Attack3:
        if (b.sm.after(220)) {
            b.sm.enter(BossState::Recover);
        }
        break;
    case BossState::Recover:
        b.tell = 0;
        b.body.velocity.x = 0.0f;
        if (b.sm.after(_pace(b, 36))) {
            b.sm.enter(BossState::Move);
        }
        break;
    default: break;
    }
}

// The Prism Wyrm: a winged crystal serpent that never lands.
void Session::stepPrismWyrm(Boss& b)
{
    const Vec2 hero = m_hero.body.box.center();
    const Vec2 me = b.body.box.center();
    const Rect arena = room().bounds.expanded(-3.0f * kTile);
    const float floor = room().bounds.bottom() - 2.0f * kTile;
    const Vec2 center{arena.center().x, floor - 7.0f * kTile};
    const auto moveTo = [&](const Vec2 goal, const float speed) {
        const Vec2 d = goal - me;
        const float len = gamelib::core::length(d);
        const Vec2 step = len > speed ? d / len * speed : d;
        b.body.box.x += step.x;
        b.body.box.y += step.y;
    };
    b.body.direction = _sign(hero.x - me.x) != 0 ? _sign(hero.x - me.x) : b.body.direction;
    const float t = static_cast<float>(m_ticks) * 0.012f;
    const Vec2 orbit = center + Vec2{std::sin(t) * arena.width * 0.3f, std::sin(2.0f * t) * 2.0f * kTile};
    switch (b.sm.current()) {
    case BossState::Idle:
        b.tell = 0;
        moveTo(orbit, 1.6f);
        if (b.sm.after(_pace(b, 70))) {
            const int count = b.phases.phase() >= 1 ? 3 : 2;
            b.pattern = gamelib::core::nextPattern(m_rng, count, b.pattern);
            b.sm.enter(b.pattern == 0 ? BossState::Attack : (b.pattern == 1 ? BossState::Move : BossState::Attack2));
        }
        break;
    case BossState::Attack: // a fan of shards at the hero
        moveTo(orbit, 0.6f);
        _tell(b, b.sm.ticksIn(), 30);
        if (b.sm.ticksIn() == 30 || (b.phases.phase() >= 2 && b.sm.ticksIn() == 52)) {
            const Vec2 aim = gamelib::core::normalized(hero - me);
            const int shots = b.phases.phase() >= 1 ? 5 : 3;
            for (int i = 0; i < shots; ++i) {
                enemyFire(me, gamelib::core::fanDirection(aim, i, shots, 0.25f) * 2.8f, 7, 1);
            }
            b.tell = 0;
        }
        if (b.sm.after(70)) {
            b.sm.enter(BossState::Recover);
        }
        break;
    case BossState::Move: // marks the hero, then dives through the mark
        if (b.sm.justEntered()) {
            b.target = hero;
        }
        if (b.sm.ticksIn() < 36) {
            _tell(b, b.sm.ticksIn(), 36);
            moveTo(me + gamelib::core::normalized(me - b.target) * 2.0f, 0.6f); // rears back
            break;
        }
        b.tell = 255;
        moveTo(b.target + gamelib::core::normalized(b.target - me) * 40.0f, b.phases.phase() >= 2 ? 5.0f : 4.2f);
        if (gamelib::core::length(b.target - me) < 8.0f || b.sm.after(110)) {
            b.sm.enter(BossState::Attack3);
        }
        break;
    case BossState::Attack3: // climbs back out of the dive
        b.tell = 0;
        moveTo(orbit, 2.2f);
        if (b.sm.after(50)) {
            b.sm.enter(BossState::Recover);
        }
        break;
    case BossState::Attack2: // shards rain from the ceiling
        moveTo(orbit, 0.8f);
        _tell(b, b.sm.ticksIn(), 26);
        if (b.sm.ticksIn() == 26) {
            const float strikeY = floor - 14.0f;
            for (int i = 0; i < 6; ++i) {
                const float x = std::clamp(hero.x + gamelib::core::centeredOffset(i, 6, 52.0f), arena.left(), arena.right());
                m_warnings.add({x, strikeY}, 50 + (i % 2) * 18);
            }
            b.tell = 0;
        }
        if (b.sm.after(90)) {
            b.sm.enter(BossState::Recover);
        }
        break;
    case BossState::Recover:
        // Tired after an attack, it hangs low for a moment: the time to strike back.
        if (b.sm.justEntered()) {
            b.target = me;
        }
        moveTo({b.target.x, floor - 3.5f * kTile}, 1.8f);
        if (b.sm.after(_pace(b, 80))) {
            b.sm.enter(BossState::Idle);
        }
        break;
    default: break;
    }
    // Keep inside the room.
    const Rect r = room().bounds.expanded(-kTile);
    b.body.box.x = std::clamp(b.body.box.x, r.left(), r.right() - b.body.box.width);
    b.body.box.y = std::clamp(b.body.box.y, r.top(), r.bottom() - 2.0f * kTile - b.body.box.height);
}

// The Molten Heart: shielded by plates of rock, open only while it attacks.
void Session::stepHeart(Boss& b)
{
    const Vec2 hero = m_hero.body.box.center();
    const Vec2 me = b.body.box.center();
    const Rect arena = room().bounds.expanded(-3.0f * kTile);
    // A slow sway over the middle of the chamber.
    const float t = static_cast<float>(m_ticks) * 0.01f;
    const Vec2 goal{b.home.x + std::sin(t) * arena.width * 0.25f, b.home.y + 6.0f + std::sin(t * 2.3f) * 10.0f};
    const Vec2 d = goal - me;
    b.body.box.x += std::clamp(d.x, -1.0f, 1.0f);
    b.body.box.y += std::clamp(d.y, -1.0f, 1.0f);
    switch (b.sm.current()) {
    case BossState::Idle: // closed: rings of orbs
        b.vulnerable.setGuarded(true);
        b.tell = 0;
        if (b.sm.ticksIn() % _pace(b, 75) == 30) {
            const int orbs = b.phases.phase() >= 1 ? 10 : 8;
            const float spin = static_cast<float>(b.sm.entries()) * 0.3f;
            for (int i = 0; i < orbs; ++i) {
                const float a = spin + static_cast<float>(i) * 2.0f * kPi / static_cast<float>(orbs);
                enemyFire(me, Vec2{std::cos(a), std::sin(a)} * 1.5f, 8, 1);
            }
        }
        if (b.sm.after(_pace(b, 150))) {
            b.sm.enter(BossState::Move);
        }
        break;
    case BossState::Move: // the plates part
        _tell(b, b.sm.ticksIn(), 30);
        if (b.sm.after(30)) {
            b.sm.enter(BossState::Attack);
            b.count = 0;
        }
        break;
    case BossState::Attack: // open: aimed orbs and magma from the vault
        b.vulnerable.setGuarded(false);
        b.tell = 255;
        if (b.sm.ticksIn() % 40 == 10) {
            enemyFire(me, gamelib::core::normalized(hero - me) * 2.2f, 8, 1);
        }
        if (b.phases.phase() >= 1 && b.sm.ticksIn() == 20) {
            const float floor = room().bounds.bottom() - 2.0f * kTile - 14.0f;
            const int gouts = b.phases.phase() >= 2 ? 5 : 3;
            for (int i = 0; i < gouts; ++i) {
                const float x = std::clamp(hero.x + gamelib::core::centeredOffset(i, gouts, 60.0f), arena.left(), arena.right());
                m_warnings.add({x, floor}, 55 + i * 8);
            }
        }
        if (b.sm.after(_pace(b, 200))) {
            b.sm.enter(BossState::Recover);
        }
        break;
    case BossState::Recover: // the plates close again
        b.tell = std::max(0, 255 - b.sm.ticksIn() * 10);
        if (b.sm.after(26)) {
            b.vulnerable.setGuarded(true);
            b.sm.enter(BossState::Idle);
        }
        break;
    default:
        b.vulnerable.setGuarded(true);
        b.sm.enter(BossState::Idle);
        break;
    }
}

} // namespace cavern::game
