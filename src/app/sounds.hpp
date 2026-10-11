#pragma once

// Cavern's sound effects and music, all synthesized with GameLib's 8-bit
// synth at startup: no audio files. Each area has a slow ambient track that
// crossfades at the area borders; the guardians, the heart, the escape,
// the title and the ending have their own.

#include "world.hpp"

#include <gamelib/audio/music.hpp>
#include <gamelib/audio/player.hpp>
#include <gamelib/audio/sound_bank.hpp>
#include <gamelib/audio/synth.hpp>

#include <cstddef>
#include <cstdint>

namespace cavern::app {

enum class Sfx : std::uint8_t {
    Jump,
    AirJump,
    WallJump,
    Dash,
    Land,
    Splash,
    Slash,
    SlashHit,
    Pogo,
    Shoot,
    NoAmmo,
    PoundDive,
    PoundLand,
    Hurt,
    Death,
    EnemyHit,
    EnemyDown,
    EnemyShot,
    Bomb,
    Deflect,
    DoorOpen,
    DoorLocked,
    Break,
    Rumble,
    Item,
    AbilityGet,
    Drop,
    Save,
    Map,
    BossWake,
    BossHit,
    BossDown,
    Strike,
    Alarm,
    Read,
    Type,
    MenuMove,
    MenuSelect,
    MenuBack,
    Count
};
inline constexpr std::size_t kSfxCount = static_cast<std::size_t>(Sfx::Count);

[[nodiscard]] gamelib::audio::SoundRecipe sfxRecipe(Sfx sfx);

enum class Tune : std::uint8_t { Title, Ruins, Fungal, Flooded, Crystal, Core, Boss, Heart, Escape, Ending, Count };
inline constexpr std::size_t kTuneCount = static_cast<std::size_t>(Tune::Count);

[[nodiscard]] gamelib::audio::Song tune(Tune tune);
[[nodiscard]] Tune areaTune(game::Area area);

using Music = gamelib::audio::MusicBank<Tune, kTuneCount>;
using Sounds = gamelib::audio::SoundBank<Sfx, kSfxCount>;

} // namespace cavern::app
