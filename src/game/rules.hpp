#pragma once

// Cavern's numbers: the tile grid and the screen a map cell stands for, the
// tick rate, the hero's box and movement tuning (GameLib's platformer
// controller set up for a nimble cave explorer: quick starts and stops, a
// variable jump with a short apex hang, coyote time and a jump buffer, and
// the dash, wall jump and double jump switched on as they are found),
// health, ammo, timers and the per-tick input. Every rule runs on whole
// ticks at kTickRate, so the same per-tick input always gives the same run.

#include <gamelib/core/math.hpp>
#include <gamelib/core/platformer.hpp>
#include <gamelib/core/replay.hpp>

#include <cstdint>

namespace cavern::game {

inline constexpr int kTickRate = 60;
inline constexpr float kTile = 16.0f;    // world units per tile
inline constexpr int kScreenColumns = 32; // a map cell is one screen: 32 x 18 tiles
inline constexpr int kScreenRows = 18;
inline constexpr float kScreenWidth = kScreenColumns * kTile;  // 512
inline constexpr float kScreenHeight = kScreenRows * kTile;    // 288

// The hero's collision box.
inline constexpr gamelib::core::Vec2 kHeroSize{11.0f, 22.0f};

// The abilities, found in this order (each from an area's guardian).
enum class Ability : std::uint8_t { Dash, WallJump, GroundPound, DoubleJump, Count };
inline constexpr int kAbilityCount = static_cast<int>(Ability::Count);

// Health in masks, ammo in shots.
inline constexpr int kStartHealth = 5;
inline constexpr int kStartAmmo = 5;
inline constexpr int kHealthPerUpgrade = 1;
inline constexpr int kAmmoPerUpgrade = 2;

// Timers, in ticks.
inline constexpr int kHurtStunTicks = 14;         // knocked back, no control
inline constexpr int kHurtInvincibleTicks = 70;   // flickering after a hit (stun included)
inline constexpr int kHazardRespawnTicks = 30;    // dark before the hero is put back at the last safe floor
inline constexpr int kScrollTicks = 36;           // a room-to-room camera scroll
inline constexpr int kDeathTicks = 140;           // the burst before the save is loaded
inline constexpr int kAbilityGetTicks = 210;      // the "ability get" scene
inline constexpr int kBossIntroTicks = 90;        // the boss wakes up and its bar fills
inline constexpr int kBossDownTicks = 150;        // explosions after a boss falls
inline constexpr int kEscapeTicks = 150 * kTickRate; // the escape from the core
inline constexpr int kShootCooldownTicks = 14;

// Movement (units per tick).
inline constexpr float kRunSpeed = 1.75f;
inline constexpr float kSwimSpeed = 1.15f;
inline constexpr float kShotSpeed = 5.2f;
inline constexpr float kKnockbackSpeed = 1.6f;
inline constexpr float kPogoSpeed = 4.2f;       // the bounce off an enemy hit by a down slash
inline constexpr float kPoundSpeed = 7.5f;
inline constexpr int kPoundHangTicks = 9;
inline constexpr int kPoundRecoveryTicks = 12;

// One tick of player input, as recorded for replays: after key mapping,
// never raw keys.
struct Input {
    int move = 0;               // -1 left, 0, 1 right
    int aim = 0;                // -1 up, 0, 1 down (slash up or down, pound, drop through)
    bool jumpHeld = false;
    bool jumpPressed = false;   // pressed this tick (also held)
    bool slashHeld = false;
    bool slashPressed = false;
    bool shootHeld = false;
    bool shootPressed = false;
    bool dashHeld = false;
    bool dashPressed = false;
    bool interact = false;      // up pressed this tick: read a sign, use a save or map station

    friend bool operator==(const Input&, const Input&) = default;
};

// Seven characters per tick (GameLib's InputCodec): move L/R, aim U/D,
// jump, slash, shoot and dash as h (held) / p (pressed), interact i.
[[nodiscard]] const gamelib::core::InputCodec<Input>& inputCodec();

// Bumped whenever a rule or the first rooms change, so stale recordings are refused.
inline constexpr std::uint32_t kRulesVersion = 1;

// The abilities the hero has, as a bit set (bit = Ability).
struct Abilities {
    std::uint8_t bits = 0;
    [[nodiscard]] constexpr bool has(const Ability a) const { return (bits & (1u << static_cast<int>(a))) != 0; }
    constexpr void add(const Ability a) { bits = static_cast<std::uint8_t>(bits | (1u << static_cast<int>(a))); }
    friend constexpr bool operator==(Abilities, Abilities) = default;
};

// The hero's movement for the abilities owned: on land, or swimming in
// water (slower, lighter, a stroke upwards on every jump press); `cling`
// false on glassy crystal walls, which cannot be slid down or jumped off.
[[nodiscard]] gamelib::core::PlatformerTuning heroTuning(Abilities abilities, bool swimming, bool cling);

} // namespace cavern::game
