#include "effects.hpp"

#include "palette.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace cavern::app {

namespace {

// helpers

using gamelib::core::Rect;
using gamelib::core::Vec2;
using game::EventKind;

gamelib::fx::AmbientStyle _moteStyle(const game::Area area)
{
    switch (area) {
    case game::Area::Ruins: return {.rate = 3.0f, .speedX = {8.0f, 20.0f}, .speedY = {-2.0f, 2.0f}, .size = {0.6f, 1.2f}};
    case game::Area::Fungal: return {.rate = 9.0f};
    case game::Area::Flooded:
        return {.rate = 9.0f, .speedX = {0.0f, 0.0f}, .speedY = {20.0f, 20.0f}, .life = {1.6f, 1.6f}, .size = {0.8f, 1.2f},
                .gravity = 260.0f, .band = {0.0f, 0.3f}};
    case game::Area::Crystal:
        return {.rate = 9.0f, .speedX = {-2.0f, 2.0f}, .speedY = {-3.0f, 3.0f}, .life = {1.5f, 3.5f}, .size = {0.7f, 1.4f}};
    case game::Area::Core:
    case game::Area::Count: break;
    }
    return {.rate = 9.0f, .speedX = {-6.0f, 6.0f}, .speedY = {-28.0f, -10.0f}, .life = {1.5f, 3.0f}, .size = {0.6f, 1.3f}};
}

} // namespace

Effects::Effects()
    : m_shards{600, {0.0f, 420.0f}, 0.4f, 0x51ED}
    , m_sparks{{.start = 3.0f, .reach = 12.0f, .length = 3.0f, .thickness = 1.0f, .jitter = 0.25f}, 0x5A4C}
    , m_dust{{.size = {1.6f, 3.0f}, .life = {0.3f, 0.5f}, .rise = {4.0f, 12.0f}, .jitter = 6.0f, .fanOut = 1.5f}, 0xD057}
{
}

void Effects::clear()
{
    m_shards.clear();
    m_sparks.clear();
    m_dust.clear();
    m_rings.clear();
    m_shake.clear();
    m_flash.clear();
    m_motes.clear();
}

void Effects::burst(const Vec2 at, const int count, const Color color, const float speed, const float size)
{
    (void)m_shards.burst(at, count,
                         {.speed = {speed * 0.4f, speed}, .size = {size * 0.6f, size}, .life = {0.35f, 0.7f},
                          .spin = {-12.0f, 12.0f}, .color = color});
}

void Effects::react(const game::Event& e, const game::Session& session)
{
    const AreaPalette& pal = areaPalette(session.area());
    const Color dust = pal[Role::RockEdge].withAlpha(0.7f);
    switch (e.kind) {
    case EventKind::Land: m_dust.burst(e.at, 4, 6.0f, {}, dust); break;
    case EventKind::Jump: m_dust.burst(e.at, 3, 4.0f, {}, dust); break;
    case EventKind::AirJump:
        m_rings.add(e.at, 10.0f, srgb(0xBFE8FF, 0.8f), 0.3f);
        m_sparks.burst(e.at, 6, srgb(0xBFE8FF), 0.25f);
        break;
    case EventKind::WallJump: m_dust.burst(e.at + Vec2{-6.0f * static_cast<float>(e.value), -8.0f}, 3, 3.0f, {}, dust); break;
    case EventKind::Dash: m_dust.burst(e.at, 5, 5.0f, {-20.0f * static_cast<float>(e.value), 0.0f}, srgb(0x9AA8FF, 0.6f)); break;
    case EventKind::Splash:
        burst(e.at, 10, pal[Role::Water].scaled(1.6f).withAlpha(0.9f), 90.0f, 2.2f);
        break;
    case EventKind::SlashHit: m_sparks.burst(e.at, 5, srgb(0xFFF2C8), 0.2f); break;
    case EventKind::Pogo: m_rings.add(e.at, 9.0f, srgb(0xFFF2C8, 0.8f), 0.25f); break;
    case EventKind::Shoot: m_sparks.burst(e.at, 4, kAmmo, 0.18f); break;
    case EventKind::PoundLand:
        m_shake.trigger(e.value > 0 ? 1.0f : 0.7f);
        m_dust.burst(e.at, 10, 18.0f, {}, dust);
        m_rings.add(e.at, 30.0f, srgb(0xFFF2C8, 0.7f), 0.35f);
        break;
    case EventKind::Hurt:
    case EventKind::HazardHit:
        m_shake.trigger(0.6f);
        m_flash.trigger(0.5f);
        m_flashColor = srgb(0xFF2A2A, 0.35f);
        m_sparks.burst(e.at, 8, kDanger, 0.3f);
        break;
    case EventKind::Death:
        m_shake.trigger(1.0f);
        burst(e.at, 40, kHeroTrim, 160.0f, 3.0f);
        burst(e.at, 30, kHeroCloak.scaled(1.6f), 120.0f, 3.5f);
        m_rings.add(e.at, 40.0f, srgb(0xFFFFFF, 0.8f), 0.6f);
        break;
    case EventKind::EnemyHit: m_sparks.burst(e.at, 6, srgb(0xFFE8B0), 0.22f); break;
    case EventKind::EnemyDown:
        burst(e.at, e.value > 0 ? 24 : 14, pal[Role::Accent], e.value > 0 ? 170.0f : 120.0f, 2.6f);
        m_rings.add(e.at, e.value > 0 ? 34.0f : 16.0f, pal[Role::Accent].withAlpha(0.8f), 0.35f);
        if (e.value > 0) {
            m_shake.trigger(0.5f);
        }
        break;
    case EventKind::Bomb:
        burst(e.at, 16, srgb(0xFFB04A), 140.0f, 2.6f);
        m_rings.add(e.at, 18.0f, srgb(0xFFD08A, 0.8f), 0.3f);
        m_shake.trigger(0.3f);
        break;
    case EventKind::Deflect:
    case EventKind::DoorLocked: m_sparks.burst(e.at, 5, srgb(0xC8D0FF), 0.2f); break;
    case EventKind::DoorOpen:
        m_dust.burst(e.at, 8, 10.0f, {}, dust);
        m_shake.trigger(0.25f);
        break;
    case EventKind::Break:
        burst(e.at, 26, pal[Role::RockEdge], 150.0f, 3.4f);
        m_dust.burst(e.at, 10, 22.0f, {}, dust);
        m_shake.trigger(0.6f);
        break;
    case EventKind::Item:
        m_rings.add(e.at, 26.0f, kGold.withAlpha(0.9f), 0.5f);
        m_sparks.burst(e.at, 12, kGold, 0.5f);
        m_flash.trigger(0.4f);
        m_flashColor = srgb(0xFFF4D0, 0.3f);
        break;
    case EventKind::Drop: m_sparks.burst(e.at, 4, kGold, 0.2f); break;
    case EventKind::Saved:
    case EventKind::Mapped:
        m_rings.add(e.at - Vec2{0.0f, 12.0f}, 30.0f, pal[Role::Accent].withAlpha(0.8f), 0.6f);
        m_flash.trigger(0.3f);
        m_flashColor = pal[Role::Accent].withAlpha(0.25f);
        break;
    case EventKind::BossWake: m_shake.trigger(0.8f); break;
    case EventKind::BossHit: m_sparks.burst(e.at, 7, srgb(0xFFE8B0), 0.25f); break;
    case EventKind::BossLand:
    case EventKind::Strike: m_shake.trigger(0.45f); break;
    case EventKind::BossDown:
        burst(e.at, 60, pal[Role::Accent], 220.0f, 3.5f);
        m_rings.add(e.at, 60.0f, srgb(0xFFFFFF, 0.9f), 0.8f);
        m_flash.trigger(1.0f);
        m_flashColor = srgb(0xFFFFFF, 0.6f);
        m_shake.trigger(1.0f);
        break;
    case EventKind::EscapeStart:
        m_shake.trigger(1.0f);
        m_flash.trigger(1.0f);
        m_flashColor = srgb(0xFF7A2A, 0.5f);
        break;
    default: break;
    }
}

void Effects::update(const float seconds, const game::Session& session, const Rect visible)
{
    m_shards.update(seconds);
    m_sparks.update(seconds);
    m_dust.update(seconds);
    m_rings.update(seconds);
    m_shake.update(seconds);
    m_flash.update(seconds);
    // Ambient motes (GameLib's Ambient): dust in the ruins, spores drifting
    // up in the fungal caves, drips from the flooded ceilings, glints in the
    // crystal depths, embers rising in the core.
    m_motes.setStyle(_moteStyle(session.area()));
    m_motes.update(seconds, visible);
}

void Effects::drawAmbient(gamelib::draw::DrawList& list, const game::Area area, const float time) const
{
    const AreaPalette& pal = areaPalette(area);
    switch (area) {
    case game::Area::Ruins: m_motes.draw(list, {.color = pal[Role::RockEdge].withAlpha(0.8f), .glowAlpha = 0.3f}); break;
    case game::Area::Flooded:
        m_motes.draw(list, {.color = pal[Role::Water].scaled(2.2f).withAlpha(0.7f), .glow = 0.0f, .stretch = 3.0f});
        break;
    case game::Area::Crystal: {
        // Glints twinkle between the area's two accents.
        const Color c = (std::sin(time * 6.0f) > 0.0f ? pal[Role::Accent] : pal[Role::Accent2]).scaled(1.4f);
        m_motes.draw(list, {.color = c.withAlpha(0.8f), .glowAlpha = 0.3f});
        break;
    }
    case game::Area::Fungal:
    case game::Area::Core:
    case game::Area::Count: m_motes.draw(list, {.color = pal[Role::Accent].withAlpha(0.8f), .glowAlpha = 0.3f}); break;
    }
}
void Effects::drawWorld(gamelib::draw::DrawList& list) const
{
    m_dust.draw(list);
    m_shards.draw(list, {}, {.aspect = 0.7f, .cornerRadius = 0.4f, .endScale = 0.3f});
    m_rings.draw(list, {.startScale = 0.3f, .startWidth = 2.0f, .endWidth = 0.6f});
    m_sparks.draw(list);
}

} // namespace cavern::app
