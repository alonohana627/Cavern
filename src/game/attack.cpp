#include "attack.hpp"

namespace cavern::game {

void Attacker::begin(const AttackClip& clip, const int facing)
{
    m_clip = clip;
    m_facing = facing < 0 ? -1 : 1;
    m_phase = clip.startup > 0 ? AttackPhase::Startup : AttackPhase::Active;
    m_ticks = 0;
    m_id = ++m_nextId;
}

bool Attacker::start(const AttackClip& clip, const int facing)
{
    if (m_phase == AttackPhase::Idle) {
        begin(clip, facing);
        return true;
    }
    const int total = m_clip.startup + m_clip.active + m_clip.recovery;
    if (m_phase == AttackPhase::Recovery && m_ticks >= total - m_clip.chainWindow) {
        m_queued = clip;
        m_queuedFacing = facing;
        return true;
    }
    return false;
}

void Attacker::cancel()
{
    m_phase = AttackPhase::Idle;
    m_queued.reset();
    m_ticks = 0;
}

std::optional<gamelib::core::Hit> Attacker::tick(const gamelib::core::Vec2 anchor, const int team)
{
    if (m_phase == AttackPhase::Idle) {
        return std::nullopt;
    }
    const int t = m_ticks;
    ++m_ticks;
    const int activeEnd = m_clip.startup + m_clip.active;
    const int total = activeEnd + m_clip.recovery;
    std::optional<gamelib::core::Hit> hit;
    if (t >= m_clip.startup && t < activeEnd) {
        m_phase = AttackPhase::Active;
        hit = gamelib::core::Hit{.box = hitbox(anchor),
                                 .damage = m_clip.damage,
                                 .knockback = {m_clip.knockback.x * static_cast<float>(m_facing), m_clip.knockback.y},
                                 .team = team,
                                 .attackId = m_id,
                                 .hitStopTicks = m_clip.hitStopTicks};
    } else if (t >= activeEnd) {
        m_phase = AttackPhase::Recovery;
    }
    if (m_ticks >= total) {
        m_phase = AttackPhase::Idle;
        if (m_queued) {
            const AttackClip next = *m_queued;
            m_queued.reset();
            begin(next, m_queuedFacing);
        }
    }
    return hit;
}

float Attacker::progress() const
{
    const int total = m_clip.startup + m_clip.active + m_clip.recovery;
    if (m_phase == AttackPhase::Idle || total <= 0) {
        return 0.0f;
    }
    return static_cast<float>(m_ticks) / static_cast<float>(total);
}

gamelib::core::Rect Attacker::hitbox(const gamelib::core::Vec2 anchor) const
{
    const gamelib::core::Rect& h = m_clip.hitbox;
    const float x = m_facing > 0 ? anchor.x + h.x : anchor.x - h.x - h.width;
    return {x, anchor.y + h.y, h.width, h.height};
}

} // namespace cavern::game
