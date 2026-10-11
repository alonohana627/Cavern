#pragma once

// Drawing the world: the camera (GameLib's Camera2D held inside the current
// room, gliding to the next one while the rules scroll), the area's
// parallax backdrop, the rock, water, doors and breakables, stations and
// items, enemies, the guardian, shots, the hero and the effects.
// Presentation only.

#include "effects.hpp"
#include "session.hpp"

#include <gamelib/core/camera.hpp>
#include <gamelib/draw/draw_list.hpp>
#include <gamelib/gfx/renderer.hpp>

namespace cavern::app {

// The game's own shader shapes, registered once.
struct Shapes {
    gamelib::draw::ShapeId tile;
    gamelib::draw::ShapeId backdrop;
    gamelib::draw::ShapeId hero;
    gamelib::draw::ShapeId enemy;
    gamelib::draw::ShapeId boss;
    gamelib::draw::ShapeId item;
    gamelib::draw::ShapeId shot;
    gamelib::draw::ShapeId prop;

    [[nodiscard]] static Shapes registerWith(gamelib::gfx::Renderer& renderer);
};

// Canvas units per world unit: one 32 x 18 tile screen fills the 1920 x 1080 canvas.
inline constexpr float kZoom = 1920.0f / game::kScreenWidth;

struct WorldDrawOptions {
    float alpha = 1.0f;        // interpolation between the last two ticks
    gamelib::core::Vec2 shake; // canvas units
    float time = 0.0f;
};

class WorldView {
public:
    explicit WorldView(gamelib::core::Size canvas);

    // Jumps the camera to the hero's room (a new run, a respawn).
    void snap(const game::Session& session);
    // One frame of camera following (call before draw).
    void follow(const game::Session& session, float alpha, float seconds);
    void draw(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, const Effects& effects,
              const WorldDrawOptions& options) const;
    [[nodiscard]] gamelib::core::View2D view(gamelib::core::Vec2 shake = {}) const { return m_camera.view(shake); }
    [[nodiscard]] gamelib::core::Rect visible() const;

    // The hero as drawn in the world and on the ability scene (world units under the list's view).
    static void drawHero(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Hero& hero,
                         gamelib::core::Rect box, float time, float glow);

private:
    void drawBackdrop(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, float time) const;
    void drawTiles(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, float time) const;
    void drawProps(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, float time) const;
    void drawEnemies(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, float alpha,
                     float time) const;
    void drawBoss(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, float alpha,
                  float time) const;
    void drawShots(gamelib::draw::DrawList& list, const Shapes& shapes, const game::Session& session, float time) const;
    void drawLights(gamelib::draw::DrawList& list, const game::Session& session, float time) const;

    gamelib::core::Size m_canvas;
    gamelib::core::Camera2D m_camera;
    int m_room = -1;
};

} // namespace cavern::app
