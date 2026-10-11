#include "rules.hpp"

namespace cavern::game {

const gamelib::core::InputCodec<Input>& inputCodec()
{
    static const gamelib::core::InputCodec<Input> codec = gamelib::core::InputCodec<Input>{}
                                                              .axis(&Input::move, 'L', 'R')
                                                              .axis(&Input::aim, 'U', 'D')
                                                              .button(&Input::jumpHeld, &Input::jumpPressed, 'j', 'J')
                                                              .button(&Input::slashHeld, &Input::slashPressed, 's', 'S')
                                                              .button(&Input::shootHeld, &Input::shootPressed, 'f', 'F')
                                                              .button(&Input::dashHeld, &Input::dashPressed, 'd', 'D')
                                                              .flag(&Input::interact, 'i');
    return codec;
}

gamelib::core::PlatformerTuning heroTuning(const Abilities abilities, const bool swimming, const bool cling)
{
    gamelib::core::PlatformerTuning t;
    // Quick starts and stops on the ground, a little less grip in the air.
    t.maxSpeed = swimming ? kSwimSpeed : kRunSpeed;
    t.groundAccel = 0.32f;
    t.groundDecel = 0.45f;
    t.airAccel = 0.24f;
    t.airDecel = 0.18f;
    // A held jump rises 3.4 tiles and peaks after 23 ticks; falling is a
    // little heavier than rising, with a short hang at the apex while held.
    t.setJumpArc(3.4f * kTile, 23);
    t.fallGravity = 1.45f;
    t.apexThreshold = 0.55f;
    t.apexGravity = 0.6f;
    t.maxFallSpeed = 5.6f;
    t.jumpCut = 0.4f;
    t.coyoteTicks = 6;
    t.jumpBufferTicks = 7;
    t.cornerCorrection = 4.0f;
    t.hazardInset = 3.0f;
    t.dropThroughTicks = 10;
    if (abilities.has(Ability::Dash)) {
        // 11 ticks at 4.4 units: 3 tiles in a fifth of a second, once in the air.
        t.dashSpeed = 4.4f;
        t.dashTicks = 11;
        t.dashCooldown = 22;
    }
    if (abilities.has(Ability::WallJump) && cling) {
        t.wallSlideSpeed = 1.1f;
        t.wallJumpSpeed = {2.5f, 4.2f};
        t.wallJumpLockTicks = 9;
        t.wallJumpDistance = 3.0f;
    }
    if (abilities.has(Ability::DoubleJump)) {
        t.airJumps = 1;
        t.airJumpSpeed = 4.4f; // about 2.8 tiles more
    }
    if (swimming) {
        // Water: light and slow, every press a stroke upwards.
        t.gravity *= 0.35f;
        t.fallGravity = 1.0f;
        t.maxFallSpeed = 1.5f;
        t.airJumps = 99;
        t.airJumpSpeed = 2.6f;
        t.jumpSpeed = 3.2f;
        t.coyoteTicks = 0;
        t.airAccel = 0.18f;
        t.airDecel = 0.12f;
    }
    return t;
}

} // namespace cavern::game
