#version 450
#include "gamelib/sprite.glsl"
#include "gamelib/sdf.glsl"

// The wanderer: a small hooded figure in a deep blue cloak, a pale face
// plate with two amber eyes in the hood, a red scarf that trails behind
// and a little lamp at the belt. SDF parts with dark outlines, posed per
// tick. The quad is the 11 x 22 body plus a margin, feet at its bottom
// (minus the margin).
//   color   cloak       color2  trim (face plate, boots)
//   params  x facing (-1, 1), y pose (0 stand, 1 run, 2 rise, 3 fall,
//           4 wall slide, 5 dash, 6 pound, 7 hurt, 8 swim, 9 slash),
//           z run phase 0..1 (or the slash's progress), w margin
//   bits.x  bits 0..7 glow 0..255 (an ability just found)

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

const vec3 kInk = vec3(0.02, 0.02, 0.05);
const vec3 kScarf = vec3(0.78, 0.13, 0.12);
const vec3 kEyes = vec3(1.5, 1.0, 0.3);
const vec3 kLamp = vec3(2.4, 1.6, 0.6);
const float kPi = 3.14159265;

void main()
{
    SpriteData s = sprite;
    vec2 p = spriteLocal(inUv);
    float margin = s.params.w;
    float height = s.size.y - 2.0 * margin;
    float facing = s.params.x < 0.0 ? -1.0 : 1.0;
    int pose = int(s.params.y + 0.5);
    float phase = s.params.z;
    float glow = float(s.bits.x & 0xffu) / 255.0;
    vec3 cloak = s.color.rgb;
    vec3 trim = s.color2.rgb;
    float time = mod(s.time, 3600.0);

    // Feet at y = 0, up negative, facing +x.
    vec2 q = vec2(p.x * facing, p.y - height * 0.5);
    float lean = 0.0;
    float bob = 0.0;
    float crouch = 0.0;
    float stride = 0.0;
    float scarfLift = 0.0;
    if (pose == 0) {
        bob = sin(time * 2.4) * 0.35;
    } else if (pose == 1) {
        float w = phase * 2.0 * kPi;
        stride = sin(w);
        bob = abs(cos(w)) * -0.8;
        lean = 0.12;
    } else if (pose == 2) {
        scarfLift = -0.6;
        crouch = 1.0;
    } else if (pose == 3) {
        scarfLift = 0.8;
    } else if (pose == 4) {
        lean = -0.08;
        scarfLift = 0.6;
    } else if (pose == 5) {
        lean = 0.45;
        crouch = 2.0;
    } else if (pose == 6) {
        crouch = 6.0;
    } else if (pose == 7) {
        lean = -0.3;
    } else if (pose == 8) {
        lean = 0.6;
        stride = sin(time * 6.0) * 0.6;
    } else if (pose == 9) {
        lean = 0.15 * sin(phase * kPi);
    }
    // Lean around the feet.
    vec2 b = q;
    b.x += (b.y) * lean;
    b.y -= bob;

    vec4 acc = vec4(0.0);
    // The scarf, behind everything: a ribbon trailing back from the neck.
    vec2 neck = vec2(1.0, -15.0 + crouch * 0.6);
    {
        vec2 r = b - neck;
        float t = clamp(-r.x / 10.0, 0.0, 1.0);
        float wave = sin(time * 9.0 - r.x * 0.6) * (0.8 + t * 1.6) + scarfLift * t * 6.0 + t * t * 3.0;
        float ribbon = max(abs(r.y - wave) - (1.3 - t * 0.5), max(r.x, -r.x - 11.0));
        acc = layerOutlined(acc, ribbon, kScarf, kInk, 0.7);
    }
    // Legs.
    float legY = -3.5 + crouch * 0.4;
    vec2 footA = vec2(1.6 + stride * 2.6, -0.8);
    vec2 footB = vec2(-1.6 - stride * 2.6, -0.8);
    if (pose == 2 || pose == 6) {
        footA = vec2(2.0, -2.5 - crouch * 0.3);
        footB = vec2(-1.5, -2.0 - crouch * 0.3);
    } else if (pose == 3) {
        footA = vec2(2.2, -0.5);
        footB = vec2(-2.0, -1.5);
    } else if (pose == 8) {
        footA = vec2(-4.0 + stride * 2.0, -4.0);
        footB = vec2(-5.0 - stride * 2.0, -2.0);
    }
    float legs = min(sdSegment(b, vec2(1.2, legY), footA, 1.25), sdSegment(b, vec2(-1.2, legY), footB, 1.25));
    acc = layerOutlined(acc, legs, cloak * 0.55, kInk, 0.7);
    float boots = min(sdCircle(b - footA, 1.5), sdCircle(b - footB, 1.5));
    acc = layerOutlined(acc, boots, trim * 0.55, kInk, 0.6);
    // The cloak: a bell from the shoulders, flaring when falling.
    float flare = pose == 3 ? 1.6 : (pose == 5 ? -0.5 : 0.0);
    vec2 c = b - vec2(0.0, -9.5 + crouch * 0.5);
    float halfW = 3.2 + clamp((c.y + 6.0) / 12.0, 0.0, 1.0) * (2.4 + flare);
    float body = max(abs(c.x) - halfW, abs(c.y) - 6.0 + crouch * 0.25);
    body = min(body, sdCircle(c - vec2(0.0, 3.5), halfW * 0.9));
    acc = layerOutlined(acc, body - 0.4, cloak, kInk, 0.8);
    // The lamp at the belt.
    vec2 lampAt = vec2(2.8, -6.5 + crouch * 0.4);
    acc = layerGlow(acc, sdCircle(b - lampAt, 1.2), kLamp, 4.0, 0.5);
    acc = layerOutlined(acc, sdCircle(b - lampAt, 1.2), kLamp, kInk, 0.5);
    // The hood: a rounded cowl with a point at the back.
    vec2 h = b - vec2(0.4, -17.0 + crouch * 0.8);
    float hood = sdCircle(h, 4.6);
    hood = min(hood, sdSegment(h, vec2(-2.0, -2.0), vec2(-5.8, 1.8 + scarfLift), 1.2));
    acc = layerOutlined(acc, hood, cloak * 1.1, kInk, 0.8);
    // The face plate and eyes.
    vec2 f = h - vec2(1.6, 0.6);
    float face = sdEllipse(f, vec2(2.6, 3.0));
    acc = layerFill(acc, face, vec3(0.05, 0.05, 0.09)); // the shadow inside the hood
    acc = layerFill(acc, abs(face + 0.35) - 0.35, trim * 0.6); // its pale rim
    float blink = step(0.06, fract(time * 0.23 + 0.31));
    float eyes = min(sdEllipse(f - vec2(-0.2, -0.3), vec2(0.45, 0.9 * blink + 0.1)),
                     sdEllipse(f - vec2(1.5, -0.3), vec2(0.45, 0.9 * blink + 0.1)));
    if (pose == 7) {
        eyes = min(sdSegment(f, vec2(-0.6, -0.8), vec2(0.4, 0.2), 0.3), sdSegment(f, vec2(1.0, -0.8), vec2(2.0, 0.2), 0.3));
    }
    acc = layerGlow(acc, eyes, kEyes, 1.2, 0.35);
    acc = layerFill(acc, eyes, kEyes);
    // A faint pale rim around the silhouette keeps the dark cloak readable on dark rock.
    float sil = min(min(legs, boots), min(body - 0.4, hood)) - 0.8;
    acc = over(acc, vec4(vec3(0.75, 0.82, 1.0), glowFalloff(max(sil, 0.0), 2.2) * 0.45 * step(0.0, sil)));
    if (glow > 0.0) {
        float aura = sdRoundBox(b - vec2(0.0, -11.0), vec2(5.5, 11.0), 5.0);
        acc = over(acc, vec4(vec3(1.4, 1.2, 0.7), glowFalloff(max(aura, 0.0), 8.0) * glow * 0.8));
    }
    outColor = acc;
}
