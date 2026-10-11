#version 450
#include "gamelib/sprite.glsl"
#include "gamelib/sdf.glsl"
#include "gamelib/noise.glsl"

// The fifteen cave creatures, each from a handful of SDF parts with dark
// outlines: a crawler beetle, a bat, a stone sentry, a spore puff, a cap
// hopper, a ceiling creeper, an eel, a shellback, a jelly, a shardling, a
// prism eye, a crystal moth, an ember wisp, a hovering drone and a magma
// lobber. The quad is the body box plus a margin, centered on the body.
//   color   main color (tinted per area)   color2  glow / accent
//   params  x kind 0..14, y facing (-1, 1), z animation phase 0..1,
//           w flash 0..1 (just hit)
//   bits.x  state (kind's own: charging, hanging, open...)

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

const vec3 kInk = vec3(0.02, 0.02, 0.04);
const float kPi = 3.14159265;

void main()
{
    SpriteData s = sprite;
    vec2 p = spriteLocal(inUv);
    int kind = int(s.params.x + 0.5);
    float facing = s.params.y < 0.0 ? -1.0 : 1.0;
    float t = s.params.z;
    float flash = s.params.w;
    uint state = s.bits.x;
    vec3 mainC = s.color.rgb;
    vec3 glowC = s.color2.rgb;
    float time = mod(s.time, 3600.0);
    vec2 q = vec2(p.x * facing, p.y);
    vec4 acc = vec4(0.0);
    float w = t * 2.0 * kPi;

    if (kind == 0) {
        // Crawler: a domed beetle on scuttling legs.
        for (int i = 0; i < 3; ++i) {
            float x = -4.0 + float(i) * 4.0;
            float k = sin(w * 2.0 + float(i) * 2.1) * 1.2;
            acc = layerOutlined(acc, sdSegment(q, vec2(x, 1.0), vec2(x + k, 4.6), 0.55), kInk * 2.0, kInk, 0.4);
        }
        float shell = max(sdEllipse(q - vec2(-0.5, 1.5), vec2(6.8, 5.6)), q.y - 2.6);
        acc = layerOutlined(acc, shell, mainC, kInk, 0.7);
        acc = layerFill(acc, max(abs(q.x + 0.5) - 0.35, q.y - 2.2) , mainC * 0.6);
        acc = layerFill(acc, max(sdEllipse(q - vec2(-2.0, -1.6), vec2(2.0, 1.1)), -1e3), mainC * 1.4);
        float head = sdCircle(q - vec2(6.0, 1.0), 2.2);
        acc = layerOutlined(acc, head, mainC * 0.7, kInk, 0.6);
        acc = layerFill(acc, sdCircle(q - vec2(6.9, 0.5), 0.7), glowC * 1.6);
        acc = layerOutlined(acc, sdSegment(q, vec2(7.0, -0.5), vec2(9.5, -3.0 + sin(w) * 0.6), 0.35), kInk, kInk, 0.2);
    } else if (kind == 1) {
        // Bat: a furred body between flapping wings.
        float flap = sin(time * 14.0 + t * 6.0);
        for (int side = -1; side <= 1; side += 2) {
            vec2 r = vec2(q.x * float(side), q.y);
            vec2 tip = vec2(8.5, -2.0 - flap * 3.5);
            float wing = sdSegment(r, vec2(1.5, -0.5), tip, 1.0);
            wing = min(wing, max(sdSegment(r, vec2(1.5, 1.0), tip + vec2(-1.0, 3.5), 2.2), -sdCircle(r - vec2(6.0, 2.5 - flap), 2.0)));
            acc = layerOutlined(acc, wing, mainC * 0.75, kInk, 0.6);
        }
        acc = layerOutlined(acc, sdCircle(q - vec2(0.0, 0.5), 3.4), mainC, kInk, 0.7);
        acc = layerOutlined(acc, min(sdSegment(q, vec2(-1.5, -2.0), vec2(-2.2, -4.6), 0.6), sdSegment(q, vec2(1.5, -2.0), vec2(2.2, -4.6), 0.6)), mainC, kInk, 0.5);
        float eyes = min(sdCircle(q - vec2(-1.1, 0.0), 0.65), sdCircle(q - vec2(1.1, 0.0), 0.65));
        acc = layerGlow(acc, eyes, vec3(2.0, 0.3, 0.3), 1.5, 0.6);
        acc = layerFill(acc, eyes, vec3(2.2, 0.4, 0.3));
    } else if (kind == 2) {
        // Sentry: a carved stone turret with a glowing eye that brightens before it fires.
        float base = sdRoundBox(q - vec2(0.0, 3.0), vec2(6.6, 4.0), 1.5);
        acc = layerOutlined(acc, base, mainC * 0.8, kInk, 0.7);
        float head = sdRoundBox(q - vec2(0.0, -2.5), vec2(5.4, 4.6), 2.5);
        acc = layerOutlined(acc, head, mainC, kInk, 0.7);
        acc = layerFill(acc, sdBox(q - vec2(0.0, -2.5), vec2(5.4, 0.4)), mainC * 0.6);
        float charge = clamp(t, 0.0, 1.0);
        float eye = sdCircle(q - vec2(2.2, -2.5), 1.8);
        acc = layerGlow(acc, eye, glowC * 1.8, 3.0 + charge * 4.0, 0.4 + charge * 0.5);
        acc = layerFill(acc, eye, mix(glowC, vec3(2.4, 2.0, 1.4), charge));
    } else if (kind == 3) {
        // Spore puff: a swollen puffball with glowing spots, breathing.
        float r = 6.2 + sin(time * 3.0 + t * 9.0) * 0.5;
        float ball = sdCircle(q, r);
        acc = layerGlow(acc, ball, glowC, 4.0, 0.4);
        acc = layerOutlined(acc, ball, mainC, kInk, 0.7);
        for (int i = 0; i < 5; ++i) {
            float a = float(i) * 1.26 + 0.4;
            vec2 at = vec2(cos(a), sin(a)) * r * 0.55;
            acc = layerFill(acc, sdCircle(q - at, 1.0 + 0.3 * float(i % 2)), glowC * 1.6);
        }
        acc = layerFill(acc, sdCircle(q - vec2(1.5, -1.0), 0.9), kInk);
        acc = layerFill(acc, sdCircle(q - vec2(-1.5, -1.0), 0.9), kInk);
    } else if (kind == 4) {
        // Cap hopper: a toadstool cap on two springy legs.
        float squash = state == 0u ? 1.0 : 0.0;
        float legLen = squash > 0.5 ? 2.0 : 4.0;
        acc = layerOutlined(acc, min(sdSegment(q, vec2(-2.5, 0.0), vec2(-3.5, legLen + 1.0), 0.9), sdSegment(q, vec2(2.5, 0.0), vec2(3.5, legLen + 1.0), 0.9)), mainC * 0.5, kInk, 0.5);
        float stem = sdRoundBox(q - vec2(0.0, 0.5), vec2(3.2, 2.6), 1.2);
        acc = layerOutlined(acc, stem, vec3(0.85, 0.8, 0.7), kInk, 0.6);
        float cap = max(sdEllipse(q - vec2(0.0, -1.0), vec2(7.2, 4.6)), q.y + 0.2);
        acc = layerOutlined(acc, cap, mainC, kInk, 0.7);
        for (int i = 0; i < 3; ++i) {
            acc = layerFill(acc, sdCircle(q - vec2(-3.5 + float(i) * 3.5, -3.0 + abs(float(i) - 1.0)), 0.9), glowC * 1.5);
        }
        acc = layerFill(acc, sdCircle(q - vec2(2.0, 0.6), 0.6), kInk);
    } else if (kind == 5) {
        // Creeper: a segmented crawler; upside down while on the ceiling.
        vec2 r = state == 0u ? vec2(q.x, -q.y) : q;
        for (int i = 0; i < 4; ++i) {
            float x = -5.0 + float(i) * 3.3;
            float k = sin(w * 2.0 + float(i) * 1.7) * 1.0;
            acc = layerOutlined(acc, sdSegment(r, vec2(x, 0.5), vec2(x + k, 3.8), 0.45), kInk * 2.0, kInk, 0.3);
            acc = layerOutlined(acc, sdCircle(r - vec2(x, -0.4 + sin(w * 2.0 + float(i)) * 0.3), 2.3), mainC * (0.8 + 0.1 * float(i)), kInk, 0.6);
        }
        acc = layerFill(acc, sdCircle(r - vec2(5.3, -0.8), 0.7), glowC * 2.0);
    } else if (kind == 6) {
        // Eel: a long wavy body with a fin and a lantern eye.
        float y0 = sin(q.x * 0.5 - time * 8.0) * 1.2;
        float body = sdSegment(vec2(q.x, q.y - y0 * (1.0 - (q.x + 9.0) / 18.0)), vec2(-8.0, 0.0), vec2(6.5, 0.0), 2.6 - (6.5 - q.x) * 0.08);
        acc = layerOutlined(acc, max(abs(q.y + 2.8 - y0) - 0.9, abs(q.x + 1.0) - 5.0), glowC * 0.8, kInk, 0.4);
        acc = layerOutlined(acc, body, mainC, kInk, 0.7);
        acc = layerGlow(acc, sdCircle(q - vec2(5.0, -0.6), 0.9), glowC * 2.0, 2.5, 0.7);
        acc = layerFill(acc, sdCircle(q - vec2(5.0, -0.6), 0.9), glowC * 2.0);
    } else if (kind == 7) {
        // Shellback: a squat crab under a heavy spiked shell facing forward.
        for (int i = 0; i < 3; ++i) {
            float x = -5.0 + float(i) * 4.0;
            acc = layerOutlined(acc, sdSegment(q, vec2(x, 3.0), vec2(x + sin(w * 2.0 + float(i)) * 1.2, 7.0), 0.7), mainC * 0.5, kInk, 0.4);
        }
        float body = sdEllipse(q - vec2(-1.0, 2.0), vec2(7.5, 4.0));
        acc = layerOutlined(acc, body, mainC * 0.7, kInk, 0.6);
        float shell = max(sdEllipse(q - vec2(2.5, -0.5), vec2(6.5, 7.0)), -q.x + 0.5);
        for (int i = 0; i < 3; ++i) {
            float a = -0.9 + float(i) * 0.9;
            vec2 tip = vec2(2.5, -0.5) + vec2(cos(a), sin(a)) * 8.6;
            shell = min(shell, sdSegment(q, vec2(2.5, -0.5) + vec2(cos(a), sin(a)) * 5.5, tip, 0.6));
        }
        acc = layerOutlined(acc, shell, glowC, kInk, 0.7);
        acc = layerFill(acc, sdCircle(q - vec2(-6.0, -0.5), 0.8), vec3(1.6, 1.4, 0.6));
    } else if (kind == 8) {
        // Jelly: a translucent glowing bell trailing tentacles.
        float pulse = sin(time * 3.0 + t * 6.0);
        for (int i = 0; i < 4; ++i) {
            float x = -3.6 + float(i) * 2.4;
            float k = sin(time * 4.0 + float(i) * 1.3 + q.y * 0.6) * 0.8;
            acc = layerFill(acc, max(abs(q.x - x - k) - 0.45, max(-q.y - 0.5, q.y - 6.5 - pulse)), glowC * 0.9);
        }
        float bell = max(sdEllipse(q - vec2(0.0, -1.0), vec2(6.4 + pulse * 0.4, 5.6 - pulse * 0.4)), q.y - 1.5);
        acc = layerGlow(acc, bell, glowC, 4.0, 0.5);
        acc = over(vec4(mainC * 1.3, coverage(bell) * 0.7), acc);
        acc = over(vec4(glowC * 1.8, coverage(abs(bell + 0.6) - 0.5) * 0.9), acc);
    } else if (kind == 9) {
        // Shardling: a crystal-backed beetle that charges.
        for (int i = 0; i < 3; ++i) {
            float x = -4.0 + float(i) * 4.0;
            float k = sin(w * (state == 1u ? 6.0 : 2.0) + float(i) * 2.0) * 1.3;
            acc = layerOutlined(acc, sdSegment(q, vec2(x, 2.0), vec2(x + k, 5.6), 0.55), kInk * 2.0, kInk, 0.4);
        }
        float body = sdEllipse(q - vec2(0.0, 1.8), vec2(7.0, 3.6));
        acc = layerOutlined(acc, body, mainC * 0.6, kInk, 0.6);
        float shards = 1e3;
        for (int i = 0; i < 3; ++i) {
            float x = -4.0 + float(i) * 3.5;
            float hgt = 5.0 + float(i % 2) * 2.0;
            shards = min(shards, max(abs(q.x - x - (q.y + 1.0) * 0.3) - (q.y + hgt) * 0.35, -(q.y + hgt)));
        }
        shards = max(shards, q.y - 1.0);
        acc = layerOutlined(acc, shards, glowC * 1.3, kInk, 0.6);
        acc = layerFill(acc, sdCircle(q - vec2(5.6, 1.2), 0.8), state == 1u ? vec3(2.4, 0.5, 0.4) : vec3(1.6, 1.6, 1.6));
    } else if (kind == 10) {
        // Prism eye: a floating faceted gem around a staring eye.
        float spin = time * 1.2;
        vec2 r = rotate2d(q, spin * 0.3);
        float gem = max(abs(r.x) * 0.87 + abs(r.y) * 0.5, abs(r.y)) - 7.0;
        acc = layerGlow(acc, gem, glowC, 4.0, 0.45);
        acc = layerOutlined(acc, gem, mainC, kInk, 0.7);
        acc = layerFill(acc, max(gem + 1.5, -r.y), mainC * 1.3);
        float eye = sdEllipse(q, vec2(3.4, 2.4));
        acc = layerFill(acc, eye, vec3(0.95, 0.95, 1.0));
        acc = layerFill(acc, sdCircle(q - vec2(0.9, 0.0), 1.3), glowC * 2.0);
        acc = layerFill(acc, sdCircle(q - vec2(1.1, 0.0), 0.6), kInk);
    } else if (kind == 11) {
        // Crystal moth: glassy wings fluttering about a dark body.
        float flap = abs(sin(time * 10.0 + t * 5.0));
        for (int side = -1; side <= 1; side += 2) {
            vec2 r = vec2(q.x, q.y * float(side));
            float wing = sdEllipse(rotate2d(r - vec2(-1.0, -3.0 * flap - 1.0), 0.6), vec2(3.6, 2.2 * flap + 0.6));
            acc = over(vec4(glowC * 1.4, coverage(wing) * 0.75), acc);
            acc = over(vec4(glowC * 2.0, coverage(abs(wing) - 0.4) * 0.9), acc);
        }
        float body = sdSegment(q, vec2(-3.5, 0.0), vec2(3.0, 0.0), 1.4);
        acc = layerOutlined(acc, body, mainC, kInk, 0.6);
        acc = layerOutlined(acc, sdSegment(q, vec2(3.0, -0.5), vec2(5.0, -3.0), 0.3), kInk, kInk, 0.1);
    } else if (kind == 12) {
        // Ember wisp: a flickering flame with a hot core.
        vec2 r = q;
        float flick = valueNoise(vec2(time * 8.0, t * 10.0));
        float flame = sdEllipse(r - vec2(0.0, 1.0), vec2(4.0, 4.2));
        flame = min(flame, sdSegment(r, vec2(0.0, 0.0), vec2(sin(time * 7.0) * 1.5, -6.5 - flick * 1.5), 1.4));
        acc = layerGlow(acc, flame, glowC, 4.0, 0.6);
        acc = layerFill(acc, flame, glowC * 1.6);
        acc = layerFill(acc, sdCircle(r - vec2(0.0, 1.0), 2.2), vec3(2.6, 2.2, 1.2));
        acc = layerFill(acc, min(sdCircle(r - vec2(-1.0, 0.6), 0.5), sdCircle(r - vec2(1.0, 0.6), 0.5)), kInk);
    } else if (kind == 13) {
        // Drone: a soot-black hovering lantern on a spinning rotor.
        float rotor = abs(sin(time * 30.0)) * 7.0 + 1.0;
        acc = layerFill(acc, sdBox(q - vec2(0.0, -5.6), vec2(rotor, 0.5)), mainC * 1.4);
        acc = layerOutlined(acc, sdSegment(q, vec2(0.0, -5.0), vec2(0.0, -3.0), 0.5), mainC, kInk, 0.3);
        float hull = sdRoundBox(q - vec2(0.0, 0.0), vec2(7.2, 3.6), 2.6);
        acc = layerOutlined(acc, hull, mainC, kInk, 0.7);
        float lamp = sdCircle(q - vec2(0.0, 3.6), 1.8);
        acc = layerGlow(acc, lamp, glowC, 4.0, 0.6);
        acc = layerFill(acc, lamp, glowC * 2.0);
        acc = layerFill(acc, sdBox(q - vec2(0.0, -0.5), vec2(5.0, 0.6)), glowC * 1.2);
    } else {
        // Magma lobber: a lump of cooling rock with fire showing through its cracks.
        float lump = sdCircle(q - vec2(0.0, 1.0), 7.0);
        lump = min(lump, sdCircle(q - vec2(-2.5, -4.0), 3.8));
        acc = layerGlow(acc, lump, glowC, 3.0, 0.35);
        acc = layerOutlined(acc, lump, mainC, kInk, 0.7);
        float crack = abs(warpedNoise(q / 5.0 + vec2(3.0), 1.0, 2) - 0.5);
        acc = over(vec4(glowC * (1.6 + 0.6 * sin(time * 4.0)), smoothstep(0.08, 0.02, crack) * coverage(lump + 1.0)), acc);
        float mouth = sdEllipse(q - vec2(3.0, 0.5), vec2(2.2, 1.0 + t * 1.5));
        acc = layerFill(acc, mouth, vec3(2.6, 1.4, 0.4));
        acc = layerFill(acc, min(sdCircle(q - vec2(2.0, -2.5), 0.8), sdCircle(q - vec2(4.6, -2.0), 0.7)), vec3(2.4, 2.0, 0.8));
    }
    if (flash > 0.0) {
        acc.rgb = mix(acc.rgb, vec3(2.4, 2.4, 2.4), flash * 0.8);
    }
    outColor = acc;
}
