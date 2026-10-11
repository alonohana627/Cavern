#pragma once

// The world's short-lived effects, in world units under the camera view:
// shards and sparks (GameLib's Particles and Sparks), dust (Puffs), rings,
// floating ambient motes (spores, drips, embers, glints) per area, and the
// screen shake and flash. Fed by the session's events; presentation only.

#include "session.hpp"

#include <gamelib/draw/draw_list.hpp>
#include <gamelib/fx/ambient.hpp>
#include <gamelib/fx/juice.hpp>
#include <gamelib/fx/particles.hpp>
#include <gamelib/fx/puffs.hpp>

namespace cavern::app {

class Effects {
public:
    Effects();

    void react(const game::Event& event, const game::Session& session);
    void update(float seconds, const game::Session& session, gamelib::core::Rect visible);
    void clear();
    // In world units (the list's view set to the camera).
    void drawWorld(gamelib::draw::DrawList& list) const;
    void drawAmbient(gamelib::draw::DrawList& list, game::Area area, float time) const;

    [[nodiscard]] gamelib::core::Vec2 shake(float time) const { return m_shake.offset(time); }
    void kick(float strength) { m_shake.trigger(strength); }
    [[nodiscard]] float flash() const { return m_flash.value(); }
    [[nodiscard]] gamelib::core::Color flashColor() const { return m_flashColor; }

private:
    void burst(gamelib::core::Vec2 at, int count, gamelib::core::Color color, float speed, float size);

    gamelib::fx::Particles m_shards;
    gamelib::fx::Sparks m_sparks;
    gamelib::fx::Puffs m_dust;
    gamelib::fx::Rings m_rings;
    gamelib::fx::Shake m_shake{0.35f, 18.0f};
    gamelib::fx::Fade m_flash{0.35f};
    gamelib::core::Color m_flashColor{1.0f, 1.0f, 1.0f, 1.0f};
    gamelib::fx::Ambient m_motes{{}, 0x5EED};
};

} // namespace cavern::app
