#include "sounds.hpp"

#include <array>
#include <string>
#include <vector>

namespace cavern::app {

namespace {

// helpers

using gamelib::audio::chord;
using gamelib::audio::Chord;
using gamelib::audio::chordPattern;
using gamelib::audio::note;
using gamelib::audio::noise;
using gamelib::audio::Note;
using gamelib::audio::Song;
using gamelib::audio::SoundRecipe;
using gamelib::audio::tone;
using gamelib::audio::Track;
using gamelib::audio::Wave;

Note _held(Note n)
{
    n.decay = 0.0f;
    return n;
}

std::string _repeat(const std::string& pattern, const int times)
{
    std::string out;
    for (int i = 0; i < times; ++i) {
        out += pattern;
        out += " | ";
    }
    return out;
}

// One bar per chord: a slow arpeggio, a held bass, a pad of the third.
constexpr const char* kSlowArp = "1 - 5 - 8 - 5 - 3 - 5 - 8 - 5 -";
constexpr const char* kRunArp = "1 5 8 5 1 5 8 5 1 5 8 5 1 5 8 5";
constexpr const char* kHeldBass = "1 - - - - - - - 5 - - - - - - -";
constexpr const char* kPumpBass = "1 - 1 - 8 - 1 - 1 - 1 - 8 - 5 -";
constexpr const char* kPad = "3 - - - - - - - - - - - - - - -";

Song _ambient(const float bpm, const std::string& lead, const std::vector<Chord>& high, const std::vector<Chord>& low,
              const std::string& drums, const Wave leadWave = Wave::Pulse25)
{
    Song s{.bpm = bpm, .stepsPerBeat = 4, .tracks = {}, .peakDb = -17.0f};
    s.tracks.push_back(Track{.wave = leadWave, .volume = 0.7f, .decay = 2.0f, .intro = {}, .loop = lead});
    s.tracks.push_back(Track{.wave = Wave::Pulse12, .volume = 0.22f, .decay = 4.0f, .intro = {}, .loop = chordPattern(high, kSlowArp)});
    s.tracks.push_back(Track{.wave = Wave::Triangle, .volume = 0.9f, .decay = 0.0f, .intro = {}, .loop = chordPattern(low, kHeldBass)});
    s.tracks.push_back(Track{.wave = Wave::Pulse50, .volume = 0.12f, .decay = 0.0f, .intro = {}, .loop = chordPattern(high, kPad)});
    if (!drums.empty()) {
        s.tracks.push_back(Track{.wave = Wave::Noise, .volume = 0.18f, .decay = 6.0f, .intro = {}, .loop = drums});
    }
    return s;
}

Song _driving(const float bpm, const std::string& lead, const std::vector<Chord>& high, const std::vector<Chord>& low,
              const std::string& drums)
{
    Song s{.bpm = bpm, .stepsPerBeat = 4, .tracks = {}, .peakDb = -15.0f};
    s.tracks.push_back(Track{.wave = Wave::Pulse25, .volume = 0.8f, .decay = 2.5f, .intro = {}, .loop = lead});
    s.tracks.push_back(Track{.wave = Wave::Pulse12, .volume = 0.25f, .decay = 5.0f, .intro = {}, .loop = chordPattern(high, kRunArp)});
    s.tracks.push_back(Track{.wave = Wave::Triangle, .volume = 1.0f, .decay = 3.0f, .intro = {}, .loop = chordPattern(low, kPumpBass)});
    s.tracks.push_back(Track{.wave = Wave::Noise, .volume = 0.28f, .decay = 7.0f, .intro = {}, .loop = drums});
    return s;
}

constexpr const char* kSoftBeat = "C3 . . . . . . . C7 . . . . . . .";
constexpr const char* kWalkBeat = "C3 . . . C7 . . . C3 . . . C7 . C7 .";
constexpr const char* kDrive = "C3 . C8 . C6 . C8 C3 C3 . C8 . C6 . C8 C8";
constexpr const char* kHurry = "C3 C8 C6 C8 C3 C8 C6 C8 C3 C8 C6 C8 C3 C6 C6 C8";

} // namespace

SoundRecipe sfxRecipe(const Sfx sfx)
{
    switch (sfx) {
    case Sfx::Jump: return {{{tone(Wave::Pulse25, note("C5"), 0.09f, note("G5"))}}, -22.0f};
    case Sfx::AirJump:
        return {{{tone(Wave::Pulse12, note("G5"), 0.06f, note("D6")), tone(Wave::Pulse12, note("D6"), 0.08f, note("A6"))}}, -21.0f};
    case Sfx::WallJump: return {{{tone(Wave::Pulse25, note("A4"), 0.07f, note("E5"))}, {noise(5000.0f, 0.05f, 2000.0f, 0.5f)}}, -21.0f};
    case Sfx::Dash: return {{{noise(9000.0f, 0.2f, 1200.0f, 0.9f)}, {tone(Wave::Pulse12, note("E5"), 0.12f, note("E4"), 0.4f)}}, -18.0f};
    case Sfx::Land: return {{{tone(Wave::Triangle, note("D3"), 0.05f, note("D2"))}, {noise(1800.0f, 0.04f, 600.0f, 0.4f)}}, -24.0f};
    case Sfx::Splash: return {{{noise(6000.0f, 0.25f, 900.0f, 0.8f)}, {tone(Wave::Triangle, note("G4"), 0.1f, note("C4"), 0.5f)}}, -20.0f};
    case Sfx::Slash: return {{{noise(11000.0f, 0.1f, 3000.0f, 0.9f)}, {tone(Wave::Pulse12, note("B5"), 0.06f, note("F5"), 0.35f)}}, -19.0f};
    case Sfx::SlashHit: return {{{tone(Wave::Pulse12, note("E6"), 0.05f, note("B5"))}, {noise(7000.0f, 0.04f, 3000.0f, 0.6f)}}, -20.0f};
    case Sfx::Pogo: return {{{tone(Wave::Pulse25, note("E5"), 0.08f, note("B5"))}}, -21.0f};
    case Sfx::Shoot:
        return {{{tone(Wave::Pulse25, note("C6"), 0.12f, note("G4"))}, {noise(9000.0f, 0.08f, 4000.0f, 0.5f)}}, -18.0f};
    case Sfx::NoAmmo: return {{{tone(Wave::Pulse12, note("C4"), 0.05f), tone(Wave::Pulse12, note("C4"), 0.05f, 0.0f, 0.0f), tone(Wave::Pulse12, note("A3"), 0.06f)}}, -22.0f};
    case Sfx::PoundDive: return {{{tone(Wave::Pulse25, note("G5"), 0.2f, note("C4"))}, {noise(4000.0f, 0.2f, 9000.0f, 0.4f)}}, -19.0f};
    case Sfx::PoundLand:
        return {{{noise(1500.0f, 0.35f, 120.0f, 1.0f)}, {tone(Wave::Triangle, note("E2"), 0.3f, note("A1"))}}, -12.0f};
    case Sfx::Hurt:
        return {{{tone(Wave::Pulse25, note("A5"), 0.05f, note("E5")), tone(Wave::Pulse25, note("D5"), 0.1f, note("A4"))},
                 {noise(3000.0f, 0.12f, 600.0f, 0.6f)}},
                -15.0f};
    case Sfx::Death:
        return {{{tone(Wave::Pulse25, note("E5"), 0.12f, note("B4")), tone(Wave::Pulse25, note("C5"), 0.12f, note("G4")),
                  tone(Wave::Pulse25, note("A4"), 0.12f, note("E4")), tone(Wave::Pulse25, note("F4"), 0.4f, note("C3"))},
                 {noise(2000.0f, 0.6f, 100.0f, 0.7f)}},
                -14.0f};
    case Sfx::EnemyHit: return {{{tone(Wave::Pulse12, note("D6"), 0.04f, note("A5"))}, {noise(5000.0f, 0.05f, 1500.0f)}}, -20.0f};
    case Sfx::EnemyDown:
        return {{{noise(2600.0f, 0.3f, 200.0f)}, {tone(Wave::Triangle, note("A3"), 0.2f, note("D2"))}}, -15.0f};
    case Sfx::EnemyShot: return {{{tone(Wave::Pulse12, note("A5"), 0.08f, note("D5"), 0.6f)}}, -23.0f};
    case Sfx::Bomb: return {{{noise(1800.0f, 0.4f, 150.0f)}, {tone(Wave::Triangle, note("F3"), 0.25f, note("C2"))}}, -14.0f};
    case Sfx::Deflect: return {{{tone(Wave::Pulse12, note("F7"), 0.06f, note("C7"))}}, -21.0f};
    case Sfx::DoorOpen:
        return {{{noise(900.0f, 0.4f, 300.0f, 0.8f)}, {tone(Wave::Triangle, note("C3"), 0.35f, note("G2"))},
                 {tone(Wave::Pulse25, note("G4"), 0.08f), tone(Wave::Pulse25, note("C5"), 0.12f)}},
                -16.0f};
    case Sfx::DoorLocked:
        return {{{tone(Wave::Pulse12, note("C4"), 0.06f), tone(Wave::Pulse12, note("C4"), 0.04f, 0.0f, 0.0f), tone(Wave::Pulse12, note("C4"), 0.06f)},
                 {noise(3000.0f, 0.06f, 1000.0f, 0.4f)}},
                -19.0f};
    case Sfx::Break:
        return {{{noise(2200.0f, 0.55f, 120.0f, 1.0f)}, {tone(Wave::Triangle, note("D3"), 0.4f, note("C2"))}}, -12.0f};
    case Sfx::Rumble: {
        Note n = noise(400.0f, 0.5f, 250.0f, 0.8f);
        return {{{n}}, -20.0f};
    }
    case Sfx::Item:
        return {{{tone(Wave::Pulse25, note("C5"), 0.08f), tone(Wave::Pulse25, note("E5"), 0.08f), tone(Wave::Pulse25, note("G5"), 0.08f),
                  tone(Wave::Pulse25, note("C6"), 0.3f)},
                 {tone(Wave::Triangle, note("C4"), 0.24f), tone(Wave::Triangle, note("G3"), 0.3f)}},
                -14.0f};
    case Sfx::AbilityGet:
        return {{{tone(Wave::Pulse25, note("G4"), 0.12f), tone(Wave::Pulse25, note("C5"), 0.12f), tone(Wave::Pulse25, note("E5"), 0.12f),
                  tone(Wave::Pulse25, note("G5"), 0.12f), tone(Wave::Pulse25, note("C6"), 0.2f), tone(Wave::Pulse25, note("B5"), 0.1f),
                  tone(Wave::Pulse25, note("C6"), 0.7f)},
                 {tone(Wave::Pulse12, note("E5"), 0.24f), tone(Wave::Pulse12, note("G5"), 0.24f), tone(Wave::Pulse12, note("E6"), 0.32f),
                  tone(Wave::Pulse12, note("G6"), 0.8f)},
                 {tone(Wave::Triangle, note("C3"), 0.48f), tone(Wave::Triangle, note("G2"), 0.3f), tone(Wave::Triangle, note("C3"), 0.8f)}},
                -12.0f};
    case Sfx::Drop: return {{{tone(Wave::Pulse25, note("A5"), 0.05f), tone(Wave::Pulse25, note("E6"), 0.08f)}}, -21.0f};
    case Sfx::Save:
        return {{{tone(Wave::Pulse50, note("E5"), 0.15f), tone(Wave::Pulse50, note("B5"), 0.15f), tone(Wave::Pulse50, note("E6"), 0.5f)},
                 {tone(Wave::Triangle, note("E4"), 0.3f), tone(Wave::Triangle, note("B3"), 0.5f)}},
                -15.0f};
    case Sfx::Map:
        return {{{tone(Wave::Pulse12, note("A5"), 0.06f), tone(Wave::Pulse12, note("C6"), 0.06f), tone(Wave::Pulse12, note("E6"), 0.06f),
                  tone(Wave::Pulse12, note("A6"), 0.25f)}},
                -17.0f};
    case Sfx::BossWake:
        return {{{tone(Wave::Pulse25, note("C3"), 0.8f, note("G2"))}, {noise(600.0f, 0.9f, 200.0f, 0.9f)}, {tone(Wave::Triangle, note("C2"), 0.9f, note("A1"))}},
                -12.0f};
    case Sfx::BossHit: return {{{tone(Wave::Pulse25, note("G4"), 0.05f, note("D4"))}, {noise(3000.0f, 0.07f, 800.0f, 0.6f)}}, -17.0f};
    case Sfx::BossDown:
        return {{{noise(2000.0f, 1.2f, 80.0f, 1.0f)}, {tone(Wave::Triangle, note("C3"), 1.0f, note("C1"))},
                 {tone(Wave::Pulse25, note("G5"), 0.2f, note("C5")), tone(Wave::Pulse25, note("E5"), 0.2f, note("A4")),
                  tone(Wave::Pulse25, note("C5"), 0.6f, note("C4"))}},
                -11.0f};
    case Sfx::Strike: return {{{noise(3500.0f, 0.3f, 300.0f, 0.9f)}, {tone(Wave::Triangle, note("A2"), 0.2f, note("E2"))}}, -15.0f};
    case Sfx::Alarm:
        return {{{tone(Wave::Pulse25, note("E5"), 0.25f, note("A5")), tone(Wave::Pulse25, note("E5"), 0.25f, note("A5"))}}, -17.0f};
    case Sfx::Read: return {{{tone(Wave::Pulse50, note("A4"), 0.05f), tone(Wave::Pulse50, note("E5"), 0.08f)}}, -22.0f};
    case Sfx::Type: return {{{tone(Wave::Pulse12, note("C6"), 0.02f)}}, -28.0f};
    case Sfx::MenuMove: return {{{tone(Wave::Pulse12, note("E5"), 0.04f)}}, -22.0f};
    case Sfx::MenuSelect: return {{{tone(Wave::Pulse25, note("C5"), 0.05f), tone(Wave::Pulse25, note("G5"), 0.09f)}}, -19.0f};
    case Sfx::MenuBack: return {{{tone(Wave::Pulse25, note("G4"), 0.05f), tone(Wave::Pulse25, note("C4"), 0.08f)}}, -20.0f};
    case Sfx::Count: break;
    }
    return {{{_held(tone(Wave::Pulse50, note("A4"), 0.01f))}}, -30.0f};
}

Tune areaTune(const game::Area area)
{
    switch (area) {
    case game::Area::Ruins: return Tune::Ruins;
    case game::Area::Fungal: return Tune::Fungal;
    case game::Area::Flooded: return Tune::Flooded;
    case game::Area::Crystal: return Tune::Crystal;
    case game::Area::Core:
    case game::Area::Count: break;
    }
    return Tune::Core;
}

Song tune(const Tune t)
{
    switch (t) {
    case Tune::Title: {
        const std::vector<Chord> hi{chord("Am", 4), chord("F", 4), chord("Dm", 4), chord("E", 4)};
        const std::vector<Chord> lo{chord("Am", 2), chord("F", 2), chord("Dm", 2), chord("E", 2)};
        return _ambient(70.0f,
                        "E5 - - - - - - - D5 - C5 - B4 - - - | C5 - - - - - - - A4 - - - - - - - | "
                        "F4 - - - A4 - - - D5 - - - C5 - B4 - | B4 - - - - - - - G#4 - - - - - . . |",
                        hi, lo, "");
    }
    case Tune::Ruins: {
        const std::vector<Chord> hi{chord("Am", 4), chord("F", 4), chord("C", 4), chord("G", 4)};
        const std::vector<Chord> lo{chord("Am", 2), chord("F", 2), chord("C", 2), chord("G", 2)};
        return _ambient(76.0f,
                        "E5 - - - - - D5 - C5 - - - B4 - A4 - | C5 - - - - - - - A4 - - - - - . . | "
                        "G4 - - - C5 - - - E5 - - - D5 - C5 - | D5 - - - - - - - - - - - . . . . |",
                        hi, lo, _repeat(kSoftBeat, 4));
    }
    case Tune::Fungal: {
        const std::vector<Chord> hi{chord("Dm", 4), chord("C", 4), chord("Bb", 3), chord("C", 4)};
        const std::vector<Chord> lo{chord("Dm", 2), chord("C", 2), chord("Bb", 1), chord("C", 2)};
        return _ambient(84.0f,
                        ". . A4 - . . D5 - F5 - - - E5 - D5 - | E5 - - - . . G4 - C5 - - - . . . . | "
                        ". . D5 - . . F4 - Bb4 - - - A4 - G4 - | A4 - - - - - - - . . . . . . . . |",
                        hi, lo, _repeat(kWalkBeat, 4), Wave::Pulse50);
    }
    case Tune::Flooded: {
        const std::vector<Chord> hi{chord("Em", 4), chord("C", 4), chord("Am", 4), chord("B", 4)};
        const std::vector<Chord> lo{chord("Em", 2), chord("C", 2), chord("Am", 2), chord("B", 2)};
        return _ambient(64.0f,
                        "B4 - - - - - - - E5 - - - - - D5 - | E5 - - - - - - - . . . . . . . . | "
                        "C5 - - - - - B4 - A4 - - - - - - - | D#5 - - - - - - - - - - - . . . . |",
                        hi, lo, "", Wave::Triangle);
    }
    case Tune::Crystal: {
        const std::vector<Chord> hi{chord("F#m", 5), chord("D", 5), chord("A", 4), chord("E", 4)};
        const std::vector<Chord> lo{chord("F#m", 2), chord("D", 2), chord("A", 1), chord("E", 2)};
        return _ambient(72.0f,
                        "C#6 - - - - - - - A5 - - - F#5 - - - | F#5 - - - - - - - E5 - D5 - . . . . | "
                        "E5 - - - - - - - C#5 - - - A4 - - - | B4 - - - - - - - G#4 - - - - - . . |",
                        hi, lo, _repeat(kSoftBeat, 4), Wave::Pulse12);
    }
    case Tune::Core: {
        const std::vector<Chord> hi{chord("Cm", 4), chord("Ab", 3), chord("Bb", 3), chord("G", 3)};
        const std::vector<Chord> lo{chord("Cm", 2), chord("Ab", 1), chord("Bb", 1), chord("G", 1)};
        return _ambient(92.0f,
                        "C5 - - - . . Eb5 - D5 - - - . . C5 - | Ab4 - - - - - - - . . . . . . . . | "
                        "Bb4 - - - D5 - - - F5 - - - Eb5 - D5 - | B4 - - - - - - - - - - - . . . . |",
                        hi, lo, _repeat(kWalkBeat, 4));
    }
    case Tune::Boss: {
        const std::vector<Chord> hi{chord("Em", 4), chord("C", 4), chord("D", 4), chord("B", 3)};
        const std::vector<Chord> lo{chord("Em", 2), chord("C", 2), chord("D", 2), chord("B", 1)};
        return _driving(144.0f,
                        "E5 - E5 - G5 - E5 - B5 - - - A5 - G5 - | E5 - - - C5 - - - D5 - E5 - - - . . | "
                        "F#5 - - - D5 - - - A5 - - - F#5 - D5 - | D#5 - - - F#5 - - - B5 - - - A5 - F#5 - |",
                        hi, lo, _repeat(kDrive, 4));
    }
    case Tune::Heart: {
        const std::vector<Chord> hi{chord("Cm", 4), chord("Ab", 4), chord("Fm", 4), chord("G", 4)};
        const std::vector<Chord> lo{chord("Cm", 2), chord("Ab", 1), chord("Fm", 2), chord("G", 1)};
        return _driving(152.0f,
                        "C5 - C6 - B5 - G5 - Eb5 - - - D5 - C5 - | Eb5 - - - C5 - - - Ab4 - - - C5 - Eb5 - | "
                        "F5 - - - Ab5 - - - C6 - - - Ab5 - F5 - | G5 - - - B4 - - - D5 - - - G5 - - - |",
                        hi, lo, _repeat(kDrive, 4));
    }
    case Tune::Escape: {
        const std::vector<Chord> hi{chord("Am", 4), chord("Am", 4), chord("F", 4), chord("E", 4)};
        const std::vector<Chord> lo{chord("Am", 2), chord("Am", 2), chord("F", 2), chord("E", 2)};
        return _driving(172.0f,
                        "A5 - A5 - E5 - A5 - C6 - B5 - A5 - G#5 - | A5 - E5 - C5 - E5 - A5 - - - . . . . | "
                        "F5 - A5 - C6 - A5 - F5 - A5 - C6 - D6 - | E6 - - - B5 - - - G#5 - - - E5 - - - |",
                        hi, lo, _repeat(kHurry, 4));
    }
    case Tune::Ending: {
        const std::vector<Chord> hi{chord("C", 4), chord("G", 4), chord("Am", 4), chord("F", 4)};
        const std::vector<Chord> lo{chord("C", 2), chord("G", 2), chord("Am", 2), chord("F", 2)};
        return _ambient(88.0f,
                        "E5 - - - G5 - - - C6 - - - B5 - G5 - | D5 - - - G5 - - - B5 - - - A5 - G5 - | "
                        "C5 - - - E5 - - - A5 - - - G5 - E5 - | F5 - - - A5 - - - G5 - - - - - . . |",
                        hi, lo, _repeat(kWalkBeat, 4));
    }
    case Tune::Count: break;
    }
    return Song{};
}

} // namespace cavern::app
