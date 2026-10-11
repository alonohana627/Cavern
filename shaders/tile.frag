#version 450
#include "gamelib/sprite.glsl"
#include "gamelib/sdf.glsl"
#include "gamelib/noise.glsl"

// Every tile of the caves (draw::drawTiles): rock with a look per area
// (sandstone blocks in the ruins, mossy rock with glowcaps in the fungal
// caves, wet blue stone with hanging weed in the flooded halls, violet rock
// sprouting crystal in the depths, basalt with magma veins in the core),
// glassy crystal, ledges, thorns, water, cracked floors and walls,
// collapsing flagstones, lava and the seal of a guardian's room. One tile
// is one 16x16 quad in world units; its neighbours decide which edges are
// open (a rim, a cap of moss, crystals or glowcaps on top).
//   color   fill          color2  edge / rim
//   params  x kind (0 rock, 1 glass, 2 ledge, 3 thorns, 4 water, 5 cracked
//           floor, 6 cracked wall, 7 collapsing floor, 8 lava, 9 seal),
//           y area (0 ruins, 1 fungal, 2 flooded, 3 crystal, 4 core),
//           z accent glow strength, w -
//   bits    neighbour mask | variant << 8 | cell x << 32 | cell y << 48

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

const uint N = 1u, E = 2u, S = 4u, W = 8u, NE = 16u, SE = 32u, SW = 64u, NW = 128u;
const float kPi = 3.14159265;

float cellHash(ivec2 c, uint salt)
{
    return hash12(uvec2(c + ivec2(1 << 16)) + uvec2(salt * 131u, salt * 977u));
}

// Distance (positive inside) from the open sides of a tile, with rounded
// outer corners where two open sides meet.
float edgeDistance(vec2 p, uint mask, float radius)
{
    bool n = (mask & N) == 0u, e = (mask & E) == 0u, s = (mask & S) == 0u, w = (mask & W) == 0u;
    float d = 1e3;
    if (n) d = min(d, p.y + 8.0);
    if (s) d = min(d, 8.0 - p.y);
    if (w) d = min(d, p.x + 8.0);
    if (e) d = min(d, 8.0 - p.x);
    // Round the corners where both sides are open.
    vec2 c = vec2(e ? 8.0 - radius : (w ? -8.0 + radius : 99.0), s ? 8.0 - radius : (n ? -8.0 + radius : 99.0));
    if (abs(c.x) < 50.0 && abs(c.y) < 50.0) {
        vec2 q = (p - c) * vec2(e ? 1.0 : -1.0, s ? 1.0 : -1.0);
        if (q.x > 0.0 && q.y > 0.0) {
            d = min(d, radius - length(q));
        }
    }
    return d;
}

vec3 rockColor(int area, vec2 world, vec3 fill, ivec2 cell, uint mask)
{
    float n = valueNoise(world / 6.0);
    float big = fbm(world / 40.0, 3);
    vec3 col = fill * (0.82 + 0.3 * n) * (0.85 + 0.3 * big);
    if (area == 0) {
        // Sandstone blocks in courses, weathered.
        float row = floor(world.y / 8.0);
        vec2 f = vec2(fract((world.x + 7.0 * mod(row, 2.0)) / 16.0) * 16.0, fract(world.y / 8.0) * 8.0);
        float seam = min(min(f.x, 16.0 - f.x), min(f.y, 8.0 - f.y));
        float shade = hashCell(vec2(floor((world.x + 7.0 * mod(row, 2.0)) / 16.0), row));
        col = fill * (0.8 + 0.3 * shade) * (0.9 + 0.15 * n);
        col = mix(col, fill * 0.45, smoothstep(1.1, 0.3, seam));
    } else if (area == 2) {
        // Wet stone blocks, larger, with a cold sheen.
        float row = floor(world.y / 16.0);
        vec2 f = vec2(fract((world.x + 12.0 * mod(row, 2.0)) / 24.0) * 24.0, fract(world.y / 16.0) * 16.0);
        float seam = min(min(f.x, 24.0 - f.x), min(f.y, 16.0 - f.y));
        col = fill * (0.75 + 0.35 * n);
        col = mix(col, fill * 0.4, smoothstep(1.2, 0.3, seam));
    } else if (area == 4) {
        // Basalt with faint columnar joints.
        float cols = abs(fract(world.x / 11.0 + 0.3 * n) - 0.5);
        col = mix(col, fill * 0.55, smoothstep(0.47, 0.5, cols));
    }
    // Deep inside the rock is darker.
    if (mask == 0xffu) {
        col *= 0.72;
    }
    return col;
}

void main()
{
    SpriteData s = sprite;
    vec2 p = spriteLocal(inUv); // -8..8, y down
    int kind = int(s.params.x + 0.5);
    int area = int(s.params.y + 0.5);
    float glow = s.params.z;
    uint mask = s.bits.x & 0xffu;
    float variant = float((s.bits.x >> 8) & 0xffu) / 255.0;
    ivec2 cell = ivec2(int(s.bits.y & 0xffffu), int(s.bits.y >> 16));
    vec2 world = vec2(cell) * 16.0 + p + 8.0;
    float time = mod(s.time, 3600.0);
    vec3 fill = s.color.rgb;
    vec3 rim = s.color2.rgb;
    vec4 c = vec4(0.0);

    if (kind == 0 || kind == 5 || kind == 6 || kind == 7) {
        float d = edgeDistance(p, mask, 4.0);
        float alpha = coverage(-d);
        vec3 col = rockColor(area, world, fill, cell, mask);
        if (kind == 7) {
            // Flagstones over the drop.
            vec2 f = fract(world / vec2(16.0, 8.0)) * vec2(16.0, 8.0);
            float seam = min(min(f.x, 16.0 - f.x), min(f.y, 8.0 - f.y));
            col = rim * (0.55 + 0.2 * valueNoise(world / 3.0));
            col = mix(col, fill * 0.35, smoothstep(1.0, 0.3, seam));
        }
        // Open edges: a rim band, thicker on top.
        bool topOpen = (mask & N) == 0u;
        float band = topOpen && p.y < -2.0 ? 3.2 : 2.0;
        float rimT = smoothstep(band, band - 1.2, d);
        col = mix(col, rim * (0.85 + 0.25 * valueNoise(world / 2.0)), rimT * 0.9);
        if (topOpen) {
            float top = p.y + 8.0; // 0 at the top edge
            if (area == 1) {
                // Moss along the top, with glowcaps.
                float moss = smoothstep(4.0 + 1.5 * valueNoise(world / 3.0), 1.0, top);
                col = mix(col, rim * vec3(0.6, 1.0, 0.7), moss * 0.8);
                float slot = floor(world.x / 6.0);
                float h = hashCell(vec2(slot, float(cell.y)));
                if (h > 0.55) {
                    vec2 cp = vec2(world.x - (slot + 0.5) * 6.0, top - 1.6);
                    float cap = sdEllipse(cp - vec2(0.0, 0.2), vec2(2.2, 1.1));
                    float pulse = 0.7 + 0.3 * sin(time * 1.7 + h * 20.0);
                    vec3 capColor = (h > 0.8 ? vec3(0.85, 0.5, 1.0) : vec3(0.5, 1.0, 0.75)) * (1.4 + glow) * pulse;
                    col = mix(col, capColor, coverage(cap));
                }
            } else if (area == 3) {
                // Crystals growing out of the top edge.
                float slot = floor(world.x / 7.0);
                float h = hashCell(vec2(slot, float(cell.y) + 3.0));
                if (h > 0.5) {
                    vec2 cp = vec2(world.x - (slot + 0.5) * 7.0, top);
                    float shard = max(abs(cp.x + (h - 0.75) * (5.0 - cp.y) * 0.4) - cp.y * 0.45, cp.y - 5.0);
                    float tw = 0.6 + 0.4 * sin(time * 2.3 + h * 31.0);
                    vec3 crystal = mix(vec3(0.85, 0.55, 1.0), vec3(0.5, 0.95, 1.0), step(0.8, h)) * (1.2 + glow * tw);
                    col = mix(col, crystal, coverage(shard) * 0.95);
                }
            } else if (area == 0) {
                // Grass tufts on the old stones.
                float tuft = smoothstep(2.4, 0.6, top) * step(0.4, valueNoise(world / 4.0));
                col = mix(col, vec3(0.42, 0.55, 0.32) * 0.7, tuft * 0.7);
            } else if (area == 2) {
                col = mix(col, vec3(0.6, 0.85, 0.95) * 0.6, smoothstep(1.5, 0.0, top) * 0.5);
            } else if (area == 4) {
                col = mix(col, vec3(1.0, 0.45, 0.15) * 0.6, smoothstep(2.0, 0.0, top) * 0.4);
            }
        }
        // Weed hanging from ceilings in the flooded halls; glowing veins in the core.
        if (area == 2 && (mask & S) == 0u) {
            float slot = floor(world.x / 5.0);
            float h = hashCell(vec2(slot, float(cell.y) + 7.0));
            float len = 3.0 + 6.0 * h;
            float x = world.x - (slot + 0.5) * 5.0 + sin(time * 1.3 + h * 9.0) * (8.0 - p.y) * 0.05;
            float strand = max(abs(x) - 0.6, (8.0 - p.y) - len);
            col = mix(col, vec3(0.25, 0.6, 0.45) * 0.7, coverage(strand) * step(h, 0.7));
        }
        if (area == 4) {
            float vein = abs(warpedNoise(world / 22.0, 1.4, 3) - 0.5);
            float v = smoothstep(0.035, 0.0, vein) * (0.6 + 0.4 * sin(time * 1.5 + world.x * 0.05));
            col = mix(col, vec3(1.6, 0.55, 0.15) * (1.0 + glow), v * 0.9);
        }
        if (kind == 5 || kind == 6) {
            // Visible cracks: jagged bright lines running across the patch.
            vec2 q = world / (kind == 5 ? vec2(14.0, 9.0) : vec2(9.0, 14.0));
            float crack = abs(warpedNoise(q, 1.1, 2) - 0.5);
            float line = smoothstep(0.06, 0.015, crack);
            col = mix(col, fill * 0.15, line);
            col = mix(col, rim * 1.2, smoothstep(0.02, 0.0, crack) * 0.6);
        }
        c = vec4(col, alpha);
    } else if (kind == 1) {
        // Glassy crystal: faceted, translucent, with moving glints.
        float d = edgeDistance(p, mask, 3.0);
        vec2 q = world / 12.0;
        vec3 v = voronoi(q);
        float facet = smoothstep(0.02, 0.12, v.y - v.x);
        float shade = 0.55 + 0.45 * fract(v.z * 7.3);
        vec3 col = mix(rim * 1.3, fill * shade, facet);
        float sweep = smoothstep(0.96, 1.0, sin((world.x - world.y) * 0.05 + time * 1.2));
        col += vec3(0.6, 0.8, 1.0) * sweep * 0.6;
        col = mix(col, rim * 1.6, smoothstep(2.0, 0.5, d));
        c = vec4(col * (1.0 + glow * 0.3), coverage(-d) * 0.94);
    } else if (kind == 2) {
        // A ledge: a plank of stone or wood across the top of the tile.
        float plank = sdRoundBox(p - vec2(0.0, -5.5), vec2(8.6, 2.4), 1.0);
        vec3 col = rim * (0.75 + 0.25 * valueNoise(world / 3.0));
        col = mix(col, fill, smoothstep(-6.5, -3.5, p.y));
        c = vec4(col, coverage(plank));
        // Ropes or roots under each end.
        if ((mask & W) == 0u || (mask & E) == 0u) {
            float side = (mask & W) == 0u ? -6.0 : 6.0;
            float post = sdBox(p - vec2(side, 0.0), vec2(0.7, 4.0));
            c = over(vec4(fill * 0.6, coverage(post)), c);
        }
    } else if (kind == 3) {
        // Thorns: a row of sharp spikes on the floor.
        float x = fract((world.x) / 5.33) * 5.33 - 2.66;
        float h = 7.0 + 3.0 * hashCell(vec2(floor(world.x / 5.33), float(cell.y)));
        float apex = 8.0 - h;
        float spike = max(abs(x) - 2.66 * (p.y - apex) / h, apex - p.y);
        vec3 col = mix(fill * 0.6, rim * 1.2, smoothstep(8.0, -8.0 + (16.0 - h), p.y));
        c = vec4(col, coverage(spike));
        c = over(vec4(fill * 0.5, coverage(sdBox(p - vec2(0.0, 7.0), vec2(8.0, 1.2)))), c);
    } else if (kind == 4) {
        // Water: translucent, darker with depth, a moving surface line.
        float alpha = 0.55;
        vec3 col = fill;
        float depthShade = 0.75 + 0.25 * valueNoise(world / 20.0 + vec2(time * 0.1, 0.0));
        col *= depthShade;
        float caustic = abs(sin(world.x * 0.18 + time * 1.4 + sin(world.y * 0.21 + time)) * sin(world.y * 0.13 - time * 0.9));
        col += rim * smoothstep(0.85, 1.0, caustic) * 0.25;
        if ((mask & N) == 0u) {
            float wave = sin(world.x * 0.25 + time * 2.2) * 0.9 + sin(world.x * 0.11 - time * 1.3) * 0.7;
            float surface = p.y + 6.0 - wave;
            alpha *= coverage(-surface);
            col = mix(col, rim * 1.5, smoothstep(1.4, 0.0, abs(surface)) * 0.9);
        }
        c = vec4(col, alpha);
    } else if (kind == 8) {
        // Lava: churning molten rock, bright at the top.
        float n = fbm(world / 10.0 + vec2(time * 0.25, -time * 0.1), 3);
        vec3 col = mix(fill, rim, smoothstep(0.35, 0.75, n)) * (1.6 + 0.6 * sin(time * 2.0 + world.x * 0.1));
        float alpha = 1.0;
        if ((mask & N) == 0u) {
            float wave = sin(world.x * 0.3 + time * 1.8) * 0.8;
            float surface = p.y + 5.0 - wave;
            alpha = coverage(-surface);
            col = mix(col, vec3(2.4, 1.6, 0.6), smoothstep(1.5, 0.0, abs(surface)));
        }
        c = vec4(col, alpha);
    } else if (kind == 9) {
        // The seal of a guardian's room: humming bars of light.
        float bars = abs(fract(world.y / 5.0 + time * 0.8) - 0.5);
        vec3 col = mix(fill * 0.4, rim * 1.8, smoothstep(0.3, 0.05, bars));
        float edge = sdBox(p, vec2(7.0, 8.2));
        c = vec4(col, coverage(edge) * 0.92);
    }
    outColor = c;
}
