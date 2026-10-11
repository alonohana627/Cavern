#pragma once

// Cavern's colors: a moody palette per area (GameLib's core::Palette by
// role), dark backgrounds behind brighter rock edges and glowing accents,
// plus the hero's and the UI's colors.

#include "world.hpp"

#include <gamelib/core/color.hpp>

#include <cstdint>

namespace cavern::app {

using gamelib::core::Color;

[[nodiscard]] inline Color srgb(const std::uint32_t hex, const float alpha = 1.0f)
{
    return gamelib::core::srgb(hex, alpha);
}

enum class Role : std::uint8_t {
    SkyTop,    // the farthest backdrop, top
    SkyLow,    // and bottom
    Far,       // far parallax silhouettes
    Mid,       // middle parallax layer
    Near,      // near parallax layer
    Rock,      // rock fill
    RockEdge,  // its open edges
    Accent,    // glowing fungus, crystals, embers
    Accent2,   // a second glow
    Water,
    Ambient,   // the light buffer's ambient color
    Count
};
inline constexpr int kRoleCount = static_cast<int>(Role::Count);

using AreaPalette = gamelib::core::Palette<Role, kRoleCount>;

[[nodiscard]] const AreaPalette& areaPalette(game::Area area);
// The area's map tint.
[[nodiscard]] Color areaMapColor(game::Area area);
// A key's color (0 green, 1 blue, 2 amber, 3 violet).
[[nodiscard]] Color keyColor(int key);

inline const Color kInk = srgb(0x05060C);
inline const Color kText = srgb(0xEDE6D6);
inline const Color kTextDim = srgb(0x8A8FA8);
inline const Color kGold = srgb(0xFFD27A);
inline const Color kHealth = srgb(0xF0EEE6);
inline const Color kAmmo = srgb(0x7FE3FF);
inline const Color kDanger = srgb(0xFF5A5A);
inline const Color kHeroCloak = srgb(0x2B3A67);
inline const Color kHeroTrim = srgb(0xE8E2CF);

} // namespace cavern::app
