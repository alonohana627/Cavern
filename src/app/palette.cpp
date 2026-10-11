#include "palette.hpp"

#include <array>

namespace cavern::app {

const AreaPalette& areaPalette(const game::Area area)
{
    // SkyTop, SkyLow, Far, Mid, Near, Rock, RockEdge, Accent, Accent2, Water, Ambient
    static const std::array<AreaPalette, game::kAreaCount> palettes{
        // Ruins: a cold dusk over weathered sandstone, lantern gold.
        AreaPalette::fromHex({0x0D1430, 0x4A3E6A, 0x1A1F3E, 0x2A2C4E, 0x3A3654, 0x4E4658, 0xB8A27E, 0xFFC86A, 0x8AB8FF,
                              0x2A5A8A, 0x8C8AB0}),
        // Fungal caves: deep teal dark, mossy rock, glowing caps.
        AreaPalette::fromHex({0x040B0E, 0x0C2226, 0x0A1A1E, 0x10282A, 0x163234, 0x1E2E2C, 0x4E8A5A, 0x7DFFB0, 0xD27BFF,
                              0x1A4A5A, 0x3A5058}),
        // Flooded halls: drowned blue stone, pale foam.
        AreaPalette::fromHex({0x030914, 0x0A2040, 0x081830, 0x0C2440, 0x123052, 0x1C2A40, 0x5A86B0, 0x9FE8FF, 0x5AFFD8,
                              0x1E6AA8, 0x34486A}),
        // Crystal depths: violet dark, amethyst glass, cyan gleams.
        AreaPalette::fromHex({0x07040F, 0x1C0E34, 0x140A28, 0x1E1038, 0x2A1648, 0x261C38, 0x9A72D8, 0xE29BFF, 0x7AF2FF,
                              0x3A2A7A, 0x3E3458}),
        // The core: black basalt, magma light.
        AreaPalette::fromHex({0x0A0304, 0x2E0A08, 0x1E0806, 0x2A0C08, 0x3A120C, 0x2A1E1E, 0xA8503A, 0xFF7A2A, 0xFFD04A,
                              0xFF5A1A, 0x58342E}),
    };
    return palettes[static_cast<std::size_t>(area)];
}

Color areaMapColor(const game::Area area)
{
    switch (area) {
    case game::Area::Ruins: return srgb(0x9A8A6A);
    case game::Area::Fungal: return srgb(0x3E9A6A);
    case game::Area::Flooded: return srgb(0x3A72B8);
    case game::Area::Crystal: return srgb(0x8A5AC8);
    case game::Area::Core: return srgb(0xC0502A);
    case game::Area::Count: break;
    }
    return srgb(0x808080);
}

Color keyColor(const int key)
{
    switch (key) {
    case 0: return srgb(0x6AF08A);
    case 1: return srgb(0x5AB8FF);
    case 2: return srgb(0xFFB84A);
    case 3: return srgb(0xC77DFF);
    default: return srgb(0xC8C8C8);
    }
}

} // namespace cavern::app
