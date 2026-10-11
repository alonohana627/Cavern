// The guardians: one per area, each guarding an ability, and the molten
// heart of the core. Scripted with GameLib's StateMachine and PhaseByHealth.
#include "session.hpp"

#include <gamelib/core/collision.hpp>

#include <algorithm>

namespace cavern::game {

namespace {

// helpers

using gamelib::core::Rect;
using gamelib::core::Vec2;

struct BossInfo {
    Vec2 size;
    int hp = 30;
    bool flying = false;
};

BossInfo _info(const BossKind kind)
{
    switch (kind) {
    case BossKind::Sporemother: return {{40.0f, 36.0f}, 26, false};
    case BossKind::Tidewarden: return {{44.0f, 30.0f}, 32, false};
    case BossKind::Colossus: return {{40.0f, 52.0f}, 38, false};
    case BossKind::PrismWyrm: return {{34.0f, 30.0f}, 40, true};
    case BossKind::Heart: return {{56.0f, 56.0f}, 50, true};
    case BossKind::Count: break;
    }
    return {{32.0f, 32.0f}, 20, false};
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
    b.previous = b.body.box;
    b.hp = info.hp;
    b.maxHp = info.hp;
    b.flying = info.flying;
    b.vulnerable = gamelib::core::Vulnerable{b.body.box, 1, 10, 0};
    b.sm.enter(BossState::Intro);
    m_boss = b;
    sealRoom(true);
    m_hero.body.velocity.x = 0.0f;
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
    switch (b.kind) {
    case BossKind::Sporemother: stepSporemother(b); break;
    case BossKind::Tidewarden: stepTidewarden(b); break;
    case BossKind::Colossus: stepColossus(b); break;
    case BossKind::PrismWyrm: stepPrismWyrm(b); break;
    case BossKind::Heart: stepHeart(b); break;
    case BossKind::Count: break;
    }
    b.sm.tick();
    b.vulnerable.setHurtbox(b.body.box);
    if (m_phase != Phase::Playing || !m_boss) {
        return;
    }
    while (const auto hit = m_shots.hitTest(b.vulnerable)) {
        bossHurt(hit->landed.damage, hit->projectile.position);
        if (!m_boss || m_phase != Phase::Playing) {
            return;
        }
    }
    if (gamelib::core::overlaps(b.body.box.expanded(-3.0f), m_hero.vulnerable.hurtbox())) {
        hurtHero(1, b.body.box);
    }
}

void Session::bossHurt(const int damage, const Vec2 at)
{
    if (!m_boss || m_boss->hp <= 0) {
        return;
    }
    Boss& b = *m_boss;
    b.hp = std::max(0, b.hp - damage);
    b.shown = b.hp;
    b.flash = 8;
    m_events.push({.kind = EventKind::BossHit, .at = at, .value = b.hp, .boss = b.kind});
    if (b.phases.update(b.hp, b.maxHp)) {
        m_events.push({.kind = EventKind::BossPhase, .at = b.body.box.center(), .value = b.phases.phase(), .boss = b.kind});
    }
    if (b.hp <= 0) {
        b.sm.enter(BossState::Dead);
        m_shots.removeIf([](const Shot& s) { return s.extra.enemy; });
        m_warnings.clear();
        m_phase = Phase::BossDown;
        m_phaseTicks = 0;
    }
}

void Session::stepSporemother(Boss& b)
{
    (void)b;
}

void Session::stepTidewarden(Boss& b)
{
    (void)b;
}

void Session::stepColossus(Boss& b)
{
    (void)b;
}

void Session::stepPrismWyrm(Boss& b)
{
    (void)b;
}

void Session::stepHeart(Boss& b)
{
    (void)b;
}

} // namespace cavern::game
