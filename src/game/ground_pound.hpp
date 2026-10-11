#pragma once

// A ground pound for a platformer body: started in the air, a short hang,
// then a dive straight down at a set speed that ignores the fall cap,
// landing with the row of cells under the feet (so the game can break
// cracked floors at once) and a recovery lock. While it runs it moves the
// body instead of core::stepPlatformer. When the game broke the floor the
// dive landed on, continueDive() keeps falling (through several cracked
// floors in one dive). Generic: no Cavern rules - GameLib's P1 "Ground
// pound" entry, written here first.

#include <gamelib/core/platformer.hpp>
#include <gamelib/core/tile_collision.hpp>
#include <gamelib/core/tile_map.hpp>

namespace cavern::game {

struct PoundTuning {
    int hangTicks = 8;      // still in the air before the dive
    float speed = 7.0f;     // units per tick, straight down
    int recoveryTicks = 12; // stuck on the ground after landing
};

enum class PoundPhase { None, Hang, Dive, Recover };

struct PoundStep {
    bool dived = false;   // the dive started this tick (after the hang)
    bool landed = false;  // hit the ground this tick
    gamelib::core::CellRange cells; // with landed: the cells under the feet (cellsBelow)
    gamelib::core::TileContacts contacts;
};

class GroundPound {
public:
    // Starts a pound if the body is in the air and not climbing; true if it
    // started. The body stops (velocity 0, a dash ends).
    bool start(gamelib::core::PlatformerBody& body);
    // One tick while active(): hangs, dives through the tiles or recovers.
    PoundStep step(gamelib::core::PlatformerBody& body, const PoundTuning& tuning,
                   const gamelib::core::TileWorld& world);
    // After a landing whose floor the game removed: dive on from here.
    void continueDive() { m_phase = PoundPhase::Dive; }
    void cancel() { m_phase = PoundPhase::None; }

    [[nodiscard]] bool active() const { return m_phase != PoundPhase::None; }
    [[nodiscard]] PoundPhase phase() const { return m_phase; }
    [[nodiscard]] int ticks() const { return m_ticks; }

private:
    PoundPhase m_phase = PoundPhase::None;
    int m_ticks = 0;
};

} // namespace cavern::game
