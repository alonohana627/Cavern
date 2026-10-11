#version 450
#include "gamelib/sprite.glsl"
#include "gamelib/sdf.glsl"
#include "gamelib/noise.glsl"

// Everything that flies or swings: the hero's spark bolt, the creatures'
// bolts, spores, bubbles, prism beams, bombs, magma, shards and the
// guardians' orbs, the hero's slash (a crescent swept across its progress)
// and the warning glow of a strike to come. The sprite's rotation points
// the shot along its flight (+x forward).
//   color   main    color2  glow
//   params  x kind (0 hero bolt, 1 bolt, 2 spore, 3 bubble, 4 beam, 5 bomb,
//           6 magma, 7 shard, 8 orb, 9 slash, 10 warning), y age (ticks),
//           z progress 0..1 (slash, warning), w -

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

const vec3 kInk = vec3(0.02, 0.02, 0.04);

void main()
{
    SpriteData s = sprite;
    vec2 p = spriteLocal(inUv);
    int kind = int(s.params.x + 0.5);
    float age = s.params.y;
    float progress = s.params.z;
    vec3 mainC = s.color.rgb;
    vec3 glowC = s.color2.rgb;
    float time = mod(s.time, 3600.0);
    vec4 acc = vec4(0.0);
    vec2 half_ = s.size * 0.5;

    if (kind == 0) {
        // A spark bolt: a hot head and a fading tail.
        float head = sdCircle(p - vec2(half_.x - 4.0, 0.0), 2.6);
        float tail = max(abs(p.y) - (p.x + half_.x) / (2.0 * half_.x) * 2.2, -(p.x + half_.x));
        tail = max(tail, p.x - half_.x + 4.0);
        acc = layerGlow(acc, min(head, tail), glowC, 4.0, 0.7);
        acc = layerFill(acc, tail, glowC * 1.5);
        acc = layerFill(acc, head, vec3(2.6, 2.8, 3.0));
    } else if (kind == 9) {
        // The slash: a crescent sweeping from the top of the arc to its end.
        float r = half_.x * 0.82;
        float d = length(p - vec2(-half_.x * 0.55, 0.0));
        float ring = abs(d - r) - 2.2 * (1.0 - abs(atan(p.y, p.x + half_.x * 0.55)) * 0.55);
        float ang = atan(p.y, p.x + half_.x * 0.55);
        float sweep = mix(-1.3, 1.3, clamp(progress * 1.6, 0.0, 1.0));
        float lit = smoothstep(0.0, 0.25, sweep - ang) * smoothstep(-1.4, -1.0, ang) * smoothstep(1.4, 1.0, ang);
        float fade = 1.0 - smoothstep(0.55, 1.0, progress);
        acc = layerGlow(acc, ring, glowC, 3.5, 0.6 * lit * fade);
        acc = over(vec4(mix(glowC, vec3(2.4, 2.4, 2.2), 0.6), coverage(ring) * lit * fade), acc);
    } else if (kind == 10) {
        // A warning: a pulsing glow where something will strike.
        float d = sdRoundBox(p, half_ - 2.0, min(half_.x, half_.y) * 0.6);
        float blink = 0.5 + 0.5 * sin(time * (8.0 + progress * 20.0));
        acc = vec4(glowC * 1.4, (1.0 - smoothstep(-half_.x, 0.0, d)) * (0.25 + 0.45 * progress) * (0.6 + 0.4 * blink));
        acc = over(vec4(glowC * 2.0, coverage(abs(d) - 0.8) * (0.5 + 0.5 * progress)), acc);
    } else {
        float r = min(half_.x, half_.y) - 1.5;
        float d = sdCircle(p, r);
        if (kind == 1) {
            d = sdEllipse(p, vec2(half_.x - 1.0, r * 0.7));
            acc = layerGlow(acc, d, glowC, 3.0, 0.7);
            acc = layerFill(acc, d, glowC * 1.6);
            acc = layerFill(acc, sdCircle(p - vec2(1.0, 0.0), r * 0.4), vec3(2.6, 2.2, 1.6));
        } else if (kind == 2) {
            float wob = valueNoise(vec2(age * 0.2, p.x * 0.4));
            acc = layerGlow(acc, d + wob, glowC, 3.0, 0.5);
            acc = over(vec4(glowC * 1.3, coverage(d + wob * 0.8) * 0.8), acc);
        } else if (kind == 3) {
            acc = over(vec4(glowC * 1.4, coverage(abs(d) - 0.6)), acc);
            acc = over(vec4(glowC, coverage(d) * 0.25), acc);
            acc = layerFill(acc, sdCircle(p - vec2(-1.0, -1.2), 0.8), vec3(2.0));
        } else if (kind == 4) {
            d = max(abs(p.x) * 0.6 + abs(p.y) - r * 0.8, -1e3);
            acc = layerGlow(acc, d, glowC, 3.5, 0.7);
            acc = layerFill(acc, d, mix(glowC, vec3(2.5), 0.5));
        } else if (kind == 5) {
            float blink = step(0.5, fract(age / 10.0));
            acc = layerOutlined(acc, d, mainC, kInk, 0.6);
            acc = layerFill(acc, sdCircle(p - vec2(0.0, -r), 1.0), mix(glowC, vec3(2.6, 0.6, 0.3), blink));
        } else if (kind == 6) {
            float n = valueNoise(p * 0.8 + vec2(age * 0.3));
            acc = layerGlow(acc, d, glowC, 4.0, 0.6);
            acc = layerFill(acc, d, mix(glowC * 1.8, vec3(2.8, 2.0, 0.6), n));
        } else if (kind == 7) {
            d = max(abs(p.x) * 0.5 + abs(p.y) - r * 0.7, -1e3);
            acc = layerOutlined(acc, d, mainC, kInk, 0.5);
            acc = layerGlow(acc, d, glowC, 2.0, 0.6);
        } else {
            float pulse = 0.5 + 0.5 * sin(time * 12.0 + age);
            acc = layerGlow(acc, d, glowC, 5.0, 0.6 + 0.2 * pulse);
            acc = layerFill(acc, d, glowC * 1.6);
            acc = layerFill(acc, sdCircle(p, r * 0.45), vec3(2.6, 2.6, 2.4));
        }
    }
    outColor = acc;
}
