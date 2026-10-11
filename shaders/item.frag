#version 450
#include "gamelib/sprite.glsl"
#include "gamelib/sdf.glsl"
#include "gamelib/noise.glsl"

// Things placed in the world: upgrades, keys and abilities with a pulsing
// aura, the shrines that save, the map stations, signs and lore tablets,
// small drops, doors (plain stone, key-colored, one-way, the core's seal)
// and the daylight at the end of the escape. The quad is centered on the
// thing; world units.
//   color   main    color2  glow
//   params  x kind (0 health vessel, 1 bolt shard, 2 key, 3 ability,
//           4 shrine, 5 map station, 6 sign, 7 tablet, 8 health drop,
//           9 bolt drop, 10 closed door, 11 open door frame, 12 daylight,
//           13 empty pedestal), y seed, z variant (ability 0..3, door kind
//           0 plain 1 key 2 from left 3 from right 4 escape), w highlight 0..1

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

const vec3 kInk = vec3(0.02, 0.02, 0.04);
const float kPi = 3.14159265;

float heart(vec2 q, float r)
{
    // Inigo Quilez's heart, y flipped for the screen and centered.
    vec2 p = vec2(abs(q.x), -q.y) / r + vec2(0.0, 0.55);
    vec2 a = p - vec2(0.0, 1.0);
    vec2 b = p - 0.5 * max(p.x + p.y, 0.0);
    float d = p.y + p.x > 1.0 ? length(p - vec2(0.25, 0.75)) - 0.35355 : sqrt(min(dot(a, a), dot(b, b))) * sign(p.x - p.y);
    return d * r;
}

void main()
{
    SpriteData s = sprite;
    vec2 p = spriteLocal(inUv);
    int kind = int(s.params.x + 0.5);
    float seed = s.params.y;
    float variant = s.params.z;
    float high = s.params.w;
    vec3 mainC = s.color.rgb;
    vec3 glowC = s.color2.rgb;
    float time = mod(s.time, 3600.0);
    float pulse = 0.5 + 0.5 * sin(time * 3.0 + seed);
    float hover = sin(time * 2.0 + seed) * 1.2;
    vec4 acc = vec4(0.0);

    if (kind <= 3) {
        vec2 q = p - vec2(0.0, hover);
        float auraR = kind == 3 ? 11.0 : 7.0;
        float aura = sdCircle(q, auraR * (0.85 + 0.15 * pulse));
        acc = layerGlow(acc, aura, glowC, kind == 3 ? 12.0 : 6.0, 0.35 + 0.25 * pulse);
        acc = over(vec4(glowC, coverage(abs(aura) - 0.4) * 0.5 * pulse), acc);
        if (kind == 0) {
            float h = heart(q, 5.0);
            acc = layerOutlined(acc, h, mainC, kInk, 0.7);
            acc = layerFill(acc, sdEllipse(q - vec2(-1.5, -1.5), vec2(1.0, 1.6)), vec3(1.8));
        } else if (kind == 1) {
            float shard = max(abs(q.x) * 1.6 + abs(q.y) * 0.7 - 5.0, -1e3);
            acc = layerOutlined(acc, shard, mainC, kInk, 0.6);
            acc = layerFill(acc, max(abs(q.x) * 1.6 + abs(q.y) * 0.7 - 2.5, q.x), mainC * 1.6);
        } else if (kind == 2) {
            float ring = abs(sdCircle(q - vec2(-2.5, 0.0), 2.6)) - 0.9;
            float shaft = sdBox(q - vec2(2.0, 0.0), vec2(3.5, 0.9));
            float teeth = min(sdBox(q - vec2(4.0, 1.8), vec2(0.6, 1.2)), sdBox(q - vec2(2.2, 1.6), vec2(0.6, 0.9)));
            float key = min(ring, min(shaft, teeth));
            acc = layerOutlined(acc, key, mainC, kInk, 0.7);
        } else {
            // An ability: a bright orb with the ability's sign inside.
            float orb = sdCircle(q, 6.0);
            acc = layerOutlined(acc, orb, mainC * (1.2 + 0.4 * pulse), kInk, 0.6);
            int a = int(variant + 0.5);
            float sign;
            if (a == 0) { // dash: a chevron
                sign = min(sdSegment(q, vec2(-3.0, -3.0), vec2(1.0, 0.0), 0.8), sdSegment(q, vec2(-3.0, 3.0), vec2(1.0, 0.0), 0.8));
                sign = min(sign, min(sdSegment(q, vec2(0.0, -3.0), vec2(4.0, 0.0), 0.8), sdSegment(q, vec2(0.0, 3.0), vec2(4.0, 0.0), 0.8)));
            } else if (a == 1) { // cling claws: three hooks
                sign = min(sdSegment(q, vec2(-2.5, -3.0), vec2(-2.5, 2.5), 0.7), min(sdSegment(q, vec2(0.0, -3.5), vec2(0.0, 2.5), 0.7), sdSegment(q, vec2(2.5, -3.0), vec2(2.5, 2.5), 0.7)));
            } else if (a == 2) { // quake stomp: an arrow down onto a line
                sign = min(sdSegment(q, vec2(0.0, -4.0), vec2(0.0, 1.5), 0.8), min(sdSegment(q, vec2(-2.5, -0.5), vec2(0.0, 2.0), 0.8), sdSegment(q, vec2(2.5, -0.5), vec2(0.0, 2.0), 0.8)));
                sign = min(sign, sdSegment(q, vec2(-4.0, 3.8), vec2(4.0, 3.8), 0.6));
            } else { // echo wings: two arcs
                sign = min(abs(sdCircle(q - vec2(-1.5, 1.5), 3.0)) - 0.7, abs(sdCircle(q - vec2(1.5, 1.5), 3.0)) - 0.7);
                sign = max(sign, q.y - 1.0);
            }
            acc = layerFill(acc, sign, vec3(0.05, 0.05, 0.1));
        }
    } else if (kind == 4) {
        // A shrine: a carved stone pedestal crowned by a glowing cap-flame.
        float base = sdRoundBox(p - vec2(0.0, 10.0), vec2(9.0, 3.0), 1.0);
        float pillar = sdRoundBox(p - vec2(0.0, 3.0), vec2(5.0, 6.0), 1.0);
        float bowl = max(sdEllipse(p - vec2(0.0, -3.5), vec2(8.0, 3.2)), p.y + 3.5 - 0.0);
        acc = layerOutlined(acc, min(base, pillar), mainC, kInk, 0.7);
        acc = layerOutlined(acc, bowl, mainC * 1.15, kInk, 0.7);
        float flame = sdEllipse(p - vec2(0.0, -8.0 - pulse), vec2(3.6, 5.0 + pulse));
        acc = layerGlow(acc, flame, glowC, 10.0, 0.4 + 0.3 * high);
        acc = layerFill(acc, flame, glowC * (1.6 + high));
        acc = layerFill(acc, sdEllipse(p - vec2(0.0, -7.0), vec2(1.6, 2.6)), vec3(2.6, 2.6, 2.2));
        acc = layerFill(acc, abs(sdCircle(p - vec2(0.0, 3.0), 2.0)) - 0.5, glowC * 1.2);
    } else if (kind == 5) {
        // A map station: a lectern holding a glowing chart.
        float post = sdRoundBox(p - vec2(0.0, 5.0), vec2(2.0, 7.0), 0.8);
        float foot = sdRoundBox(p - vec2(0.0, 11.0), vec2(7.0, 1.6), 0.8);
        acc = layerOutlined(acc, min(post, foot), mainC * 0.8, kInk, 0.7);
        vec2 r = rotate2d(p - vec2(0.0, -3.0), -0.25);
        float board = sdRoundBox(r, vec2(9.0, 5.0), 1.0);
        acc = layerGlow(acc, board, glowC, 6.0, 0.25 + 0.4 * high);
        acc = layerOutlined(acc, board, vec3(0.85, 0.78, 0.58), kInk, 0.7);
        float lines = max(abs(fract(r.x / 3.0) - 0.5) * 3.0 - 0.35, abs(r.y) - 3.5);
        acc = layerFill(acc, min(lines, abs(sdCircle(r - vec2(2.0, 0.5), 2.0)) - 0.35), glowC * 1.3);
    } else if (kind == 6) {
        // A sign: a wooden board on a post.
        acc = layerOutlined(acc, sdBox(p - vec2(0.0, 5.0), vec2(1.2, 7.0)), vec3(0.42, 0.28, 0.16), kInk, 0.6);
        float board = sdRoundBox(p - vec2(0.0, -3.0), vec2(8.0, 4.5), 0.8);
        acc = layerGlow(acc, board, vec3(1.2, 1.0, 0.6), 4.0, high * 0.5);
        acc = layerOutlined(acc, board, vec3(0.62, 0.44, 0.26), kInk, 0.7);
        acc = layerFill(acc, max(abs(fract((p.y + 3.0) / 2.5) - 0.5) * 2.5 - 0.4, abs(p.x) - 5.5), vec3(0.3, 0.2, 0.12));
    } else if (kind == 7) {
        // A lore tablet: a stone slab with glowing runes.
        float slab = sdRoundBox(p - vec2(0.0, 1.0), vec2(8.0, 11.0), 3.0);
        slab = max(slab, -(p.y + 10.0 - abs(p.x) * 0.4));
        acc = layerOutlined(acc, slab, mainC, kInk, 0.7);
        vec2 cell = floor((p + vec2(6.0, 6.0)) / vec2(4.0, 3.5));
        vec2 f = fract((p + vec2(6.0, 6.0)) / vec2(4.0, 3.5)) - 0.5;
        float h = hashCell(cell + seed);
        float rune = h > 0.5 ? max(abs(f.x) - 0.12, abs(f.y) - 0.3) : min(abs(f.y) - 0.1, abs(length(f) - 0.25) - 0.08);
        rune = max(rune * 3.5, max(abs(p.x) - 5.5, abs(p.y - 1.0) - 8.0));
        acc = layerFill(acc, rune, glowC * (1.0 + high + 0.3 * pulse));
    } else if (kind == 8 || kind == 9) {
        float orb = sdCircle(p, 3.0);
        acc = layerGlow(acc, orb, glowC, 4.0, 0.6);
        acc = layerOutlined(acc, orb, mainC, kInk, 0.5);
        acc = layerFill(acc, sdCircle(p - vec2(-0.8, -0.8), 0.9), vec3(2.0));
    } else if (kind == 10) {
        // A closed door: a heavy slab in a frame; key doors carry a colored
        // band and lock, one-way doors an arrow toward the side that opens it.
        vec2 half_ = s.size * 0.5;
        float slab = sdRoundBox(p, half_ - vec2(1.0, 0.5), 2.0);
        acc = layerOutlined(acc, slab, mainC, kInk, 0.8);
        float bands = abs(fract(p.y / 12.0) - 0.5) * 12.0;
        acc = layerFill(acc, max(1.0 - bands, abs(p.x) - half_.x + 2.0), mainC * 0.6);
        int kindDoor = int(variant + 0.5);
        if (kindDoor == 1) {
            float band = max(abs(p.y) - 4.0, abs(p.x) - half_.x + 1.5);
            acc = layerGlow(acc, band, glowC, 5.0, 0.4 + 0.3 * pulse);
            acc = layerFill(acc, band, glowC * 1.3);
            acc = layerFill(acc, abs(sdCircle(p, 2.2)) - 0.6, kInk);
        } else if (kindDoor == 2 || kindDoor == 3) {
            float dir = kindDoor == 2 ? -1.0 : 1.0;
            vec2 r = vec2(p.x * dir, p.y);
            float arrow = min(sdSegment(r, vec2(-2.0, -3.0), vec2(2.0, 0.0), 0.8), sdSegment(r, vec2(-2.0, 3.0), vec2(2.0, 0.0), 0.8));
            acc = layerFill(acc, arrow, glowC * (0.8 + 0.5 * pulse));
        } else if (kindDoor == 4) {
            float vein = abs(warpedNoise(p / 4.0 + vec2(0.0, time * 0.3), 1.0, 2) - 0.5);
            acc = over(vec4(glowC * 1.8, smoothstep(0.08, 0.0, vein) * coverage(slab)), acc);
        } else {
            acc = layerOutlined(acc, abs(sdCircle(p - vec2(0.0, 2.0), 2.4)) - 0.6, glowC, kInk, 0.4);
        }
        acc.rgb += glowC * high * 0.3;
    } else if (kind == 11) {
        // An open door's frame: posts at both edges.
        vec2 half_ = s.size * 0.5;
        float frame = max(abs(p.x) - half_.x, -(abs(p.x) - half_.x + 2.0));
        frame = max(frame, abs(p.y) - half_.y);
        acc = layerOutlined(acc, frame, mainC * 0.8, kInk, 0.5);
    } else if (kind == 12) {
        // Daylight pouring in at the end of the escape.
        float beam = abs(p.x + p.y * 0.25) - 10.0 + p.y * 0.1;
        acc = vec4(vec3(2.2, 2.0, 1.6), (1.0 - smoothstep(-6.0, 4.0, beam)) * 0.5 * (0.8 + 0.2 * pulse));
    } else {
        // An empty pedestal.
        float ped = min(sdRoundBox(p - vec2(0.0, 6.0), vec2(8.0, 2.0), 1.0), sdRoundBox(p - vec2(0.0, 2.0), vec2(4.0, 3.0), 1.0));
        acc = layerOutlined(acc, ped, mainC, kInk, 0.7);
    }
    outColor = acc;
}
