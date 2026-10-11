#include "ground_pound.hpp"

namespace cavern::game {

bool GroundPound::start(gamelib::core::PlatformerBody& body)
{
    if (m_phase != PoundPhase::None || body.grounded || body.climbing) {
        return false;
    }
    m_phase = PoundPhase::Hang;
    m_ticks = 0;
    body.velocity = {};
    body.dashLeft = 0;
    body.rising = false;
    body.wallSliding = false;
    body.jumpBuffer = 0;
    return true;
}

PoundStep GroundPound::step(gamelib::core::PlatformerBody& body, const PoundTuning& tuning,
                            const gamelib::core::TileWorld& world)
{
    PoundStep out;
    ++m_ticks;
    switch (m_phase) {
    case PoundPhase::None: break;
    case PoundPhase::Hang:
        body.velocity = {};
        out.contacts = gamelib::core::probeContacts(body.box, world);
        if (m_ticks >= tuning.hangTicks) {
            m_phase = PoundPhase::Dive;
            m_ticks = 0;
            out.dived = true;
        }
        break;
    case PoundPhase::Dive: {
        body.velocity = {0.0f, tuning.speed};
        const gamelib::core::BoxMove move = gamelib::core::moveBox(body.box, body.velocity, world);
        body.box = move.box;
        out.contacts = move.contacts;
        if (move.contacts.ground) {
            out.landed = true;
            out.cells = gamelib::core::cellsBelow(body.box, world.tileSize());
            body.velocity = {};
            body.grounded = true;
            m_phase = PoundPhase::Recover;
            m_ticks = 0;
        }
        break;
    }
    case PoundPhase::Recover:
        body.velocity = {};
        out.contacts = gamelib::core::probeContacts(body.box, world);
        body.grounded = out.contacts.ground;
        if (m_ticks >= tuning.recoveryTicks || !body.grounded) {
            m_phase = body.grounded ? PoundPhase::None : PoundPhase::Dive;
            m_ticks = 0;
        }
        break;
    }
    return out;
}

} // namespace cavern::game
