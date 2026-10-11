#pragma once

// Melee attacks as frame data: a clip of startup, active and recovery ticks
// with a hitbox relative to the attacker (written facing right, mirrored
// for the left), and an Attacker that steps one clip at a time and hands
// out a core::Hit only on the active ticks, with a fresh attack id per
// swing so a slash that overlaps a target for several ticks hurts it once.
// A press during the last chainWindow ticks of recovery is kept and starts
// the next swing when the clip ends (a combo). Generic: no Cavern rules -
// GameLib's P1 "Attack timelines" entry, written here first.

#include <gamelib/core/combat.hpp>
#include <gamelib/core/math.hpp>

#include <optional>

namespace cavern::game {

struct AttackClip {
    int startup = 3;   // ticks before the hitbox appears
    int active = 4;    // ticks it is out
    int recovery = 8;  // ticks after, before another attack
    // The hitbox relative to the anchor (usually the attacker's center),
    // facing right; mirrored around the anchor's x when facing left.
    gamelib::core::Rect hitbox{6.0f, -8.0f, 20.0f, 16.0f};
    int damage = 1;
    gamelib::core::Vec2 knockback; // facing right; x mirrored
    int chainWindow = 0;           // the last ticks of recovery in which a press is kept
    int hitStopTicks = 0;
};

enum class AttackPhase { Idle, Startup, Active, Recovery };

class Attacker {
public:
    // Attack ids are idBase + 1, + 2, ... (distinct per attacker).
    explicit Attacker(int idBase = 1'000'000)
        : m_nextId{idBase}
    {
    }

    // Starts `clip` facing `facing` (-1 or 1). True if it started; while
    // another clip runs, a press inside its chain window is kept for when it
    // ends (true), any other press is ignored (false).
    bool start(const AttackClip& clip, int facing);
    // Stops the current clip and a kept press (the attacker was hit).
    void cancel();

    // One tick at `anchor`: the Hit while the clip is active, else nullopt.
    // Call once per tick; a kept press starts its clip here.
    std::optional<gamelib::core::Hit> tick(gamelib::core::Vec2 anchor, int team);

    [[nodiscard]] AttackPhase phase() const { return m_phase; }
    [[nodiscard]] bool busy() const { return m_phase != AttackPhase::Idle; }
    [[nodiscard]] int facing() const { return m_facing; }
    [[nodiscard]] const AttackClip& clip() const { return m_clip; }
    // 0 at the start of the clip .. 1 at its end (drawing the swing).
    [[nodiscard]] float progress() const;
    [[nodiscard]] int attackId() const { return m_id; }
    // The hitbox in world units for `anchor` (drawing, debug).
    [[nodiscard]] gamelib::core::Rect hitbox(gamelib::core::Vec2 anchor) const;

private:
    void begin(const AttackClip& clip, int facing);

    AttackClip m_clip;
    std::optional<AttackClip> m_queued;
    int m_queuedFacing = 1;
    AttackPhase m_phase = AttackPhase::Idle;
    int m_ticks = 0;  // ticks into the clip
    int m_facing = 1;
    int m_id = 0;
    int m_nextId = 0;
};

} // namespace cavern::game
