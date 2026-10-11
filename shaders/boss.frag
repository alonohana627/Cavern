#version 450
#include "gamelib/sprite.glsl"
#include "gamelib/sdf.glsl"
#include "gamelib/noise.glsl"

// The guardians, original creatures of the caves, each a layered SDF body:
//   0 the Sporemother - a bloated mushroom matron with glowing gills and a
//     ring of small eyes on root legs
//   1 the Tidewarden - an armoured crab-knight with one great claw
//   2 the Gate Colossus - a stone golem with a slit of light for a face
//   3 the Prism Wyrm - a winged crystal serpent's head (the view trails its
//     body segments behind it)
//   4 the Molten Heart - a pulsing heart of magma in plates of rock
// The quad is the body box plus a margin, centered on the body.
//   color   main      color2  glow
//   params  x kind, y facing (-1, 1), z animation 0..1, w flash 0..1
//   bits.x  bits 0..7 state (0 idle, 1 move, 2 attack, 3 attack2, 4 attack3,
//           5 recover, 6 dead), bits 8..15 phase 0..2, bits 16..23 tell 0..255

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
    uint state = s.bits.x & 0xffu;
    float phase = float((s.bits.x >> 8) & 0xffu);
    float tell = float((s.bits.x >> 16) & 0xffu) / 255.0;
    vec3 mainC = s.color.rgb;
    vec3 glowC = s.color2.rgb * (1.0 + phase * 0.25);
    float time = mod(s.time, 3600.0);
    vec2 q = vec2(p.x * facing, p.y);
    vec4 acc = vec4(0.0);

    if (kind == 0) {
        // Sporemother. Body 40 x 36: legs to y = 18, cap up to y = -18.
        float breathe = sin(time * 2.0) * 0.6;
        for (int i = 0; i < 5; ++i) {
            float x = -14.0 + float(i) * 7.0;
            float k = sin(time * 3.0 + float(i) * 1.7) * 1.5;
            acc = layerOutlined(acc, sdSegment(q, vec2(x * 0.6, 6.0), vec2(x + k, 18.0), 1.6 - abs(float(i) - 2.0) * 0.2), mainC * 0.45, kInk, 0.7);
        }
        float body = sdEllipse(q - vec2(0.0, 4.0 + breathe * 0.3), vec2(13.0, 10.0));
        acc = layerOutlined(acc, body, vec3(0.86, 0.8, 0.68) * 0.85, kInk, 0.8);
        for (int i = 0; i < 5; ++i) {
            vec2 at = vec2(-8.0 + float(i) * 4.0, 5.0 + abs(float(i) - 2.0) * 1.2);
            float eye = sdCircle(q - at, 1.3 - abs(float(i) - 2.0) * 0.15);
            acc = layerFill(acc, eye, kInk);
            acc = layerFill(acc, sdCircle(q - at - vec2(0.3, -0.2), 0.45), glowC * 2.0);
        }
        float cap = max(sdEllipse(q - vec2(0.0, -6.0 + breathe), vec2(20.0, 12.5)), q.y - (-2.0 + breathe));
        acc = layerGlow(acc, cap, glowC, 6.0, 0.25 + tell * 0.5);
        acc = layerOutlined(acc, cap, mainC, kInk, 0.9);
        // Glowing gills under the cap rim.
        float gills = max(abs(fract((q.x + 20.0) / 2.6) - 0.5) * 2.6 - 0.45, abs(q.y - (-3.2 + breathe)) - 1.2);
        gills = max(gills, abs(q.x) - 18.0);
        acc = layerFill(acc, gills, glowC * (1.5 + tell));
        for (int i = 0; i < 6; ++i) {
            vec2 at = vec2(-14.0 + float(i) * 5.6, -10.0 + breathe + sin(float(i) * 2.3) * 2.5);
            acc = layerFill(acc, sdCircle(q - at, 1.2 + float(i % 3) * 0.4), glowC * 1.3);
        }
    } else if (kind == 1) {
        // Tidewarden. Body 44 x 30.
        for (int i = 0; i < 4; ++i) {
            float x = -14.0 + float(i) * 6.5;
            float k = sin(time * 5.0 * (state == 1u ? 2.0 : 1.0) + float(i) * 1.8) * 1.8;
            acc = layerOutlined(acc, min(sdSegment(q, vec2(x, 6.0), vec2(x + 3.0 + k, 10.0), 1.1), sdSegment(q, vec2(x + 3.0 + k, 10.0), vec2(x + 2.0 + k, 15.0), 0.9)), mainC * 0.6, kInk, 0.6);
        }
        float shell = sdEllipse(q - vec2(-2.0, 0.0), vec2(16.0, 9.5));
        shell = max(shell, q.y - 7.0);
        // A cool halo and a pale rim lift the shell off the dark blue stone.
        acc = layerGlow(acc, shell, glowC, 5.0, 0.35 + tell * 0.4);
        acc = layerOutlined(acc, shell, mainC, kInk, 0.9);
        // Darker underneath, lighter on top where it catches the light.
        float top = smoothstep(4.0, -9.0, q.y);
        acc = layerFill(acc, max(shell + 1.0, 0.5 - q.y), mainC * 0.75);
        acc = layerFill(acc, max(sdEllipse(q - vec2(-4.0, -3.5), vec2(11.0, 4.6)), shell + 1.0), mix(mainC * 1.3, glowC, 0.35 + 0.25 * top));
        acc = layerFill(acc, max(abs(shell + 1.4) - 0.55, q.y + 1.0), glowC * 1.35);
        for (int i = 0; i < 4; ++i) {
            acc = layerOutlined(acc, sdCircle(q - vec2(-12.0 + float(i) * 6.0, -8.5 + abs(float(i) - 1.5) * 0.8), 1.4), glowC, kInk, 0.4);
        }
        // Eyestalks.
        acc = layerOutlined(acc, min(sdSegment(q, vec2(8.0, -4.0), vec2(10.0, -11.0), 0.7), sdSegment(q, vec2(11.0, -3.0), vec2(14.0, -9.0), 0.7)), mainC * 0.8, kInk, 0.4);
        acc = layerFill(acc, min(sdCircle(q - vec2(10.0, -11.5), 1.4), sdCircle(q - vec2(14.0, -9.5), 1.4)), vec3(1.0));
        acc = layerFill(acc, min(sdCircle(q - vec2(10.4, -11.5), 0.7), sdCircle(q - vec2(14.4, -9.5), 0.7)), kInk);
        // The great claw, raised while it attacks.
        float raise = state == 2u || state == 3u ? 1.0 : tell;
        vec2 arm = vec2(16.0, 2.0 - raise * 8.0);
        acc = layerOutlined(acc, sdSegment(q, vec2(10.0, 3.0), arm, 2.4), mainC * 0.85, kInk, 0.7);
        float open = 0.4 + 0.6 * abs(sin(time * 6.0)) * raise;
        float claw = min(sdEllipse(rotate2d(q - arm - vec2(2.0, -1.5), -0.4 - open * 0.5), vec2(5.0, 2.4)),
                         sdEllipse(rotate2d(q - arm - vec2(2.0, 1.5), 0.4 + open * 0.5), vec2(5.0, 2.0)));
        acc = layerOutlined(acc, claw, mainC * 1.1, kInk, 0.7);
    } else if (kind == 2) {
        // Gate Colossus. Body 40 x 52.
        float stomp = state == 1u ? abs(sin(time * 4.0)) : 0.0;
        acc = layerOutlined(acc, min(sdRoundBox(q - vec2(-7.0, 19.0 - stomp * 2.0), vec2(5.0, 7.0), 1.5), sdRoundBox(q - vec2(7.0, 19.0), vec2(5.0, 7.0), 1.5)), mainC * 0.7, kInk, 0.8);
        float torso = sdRoundBox(q - vec2(0.0, 2.0), vec2(14.0, 12.0), 3.0);
        acc = layerOutlined(acc, torso, mainC, kInk, 0.9);
        float rune = abs(sdCircle(q - vec2(0.0, 2.0), 5.0)) - 0.8;
        acc = layerGlow(acc, rune, glowC, 3.0, 0.4 + tell * 0.5);
        acc = layerFill(acc, rune, glowC * (1.2 + tell));
        float crack = abs(warpedNoise(q / 9.0, 1.0, 2) - 0.5);
        acc = over(vec4(glowC * 1.4, smoothstep(0.05, 0.015, crack) * coverage(torso + 1.0) * (0.3 + phase * 0.3)), acc);
        float head = sdRoundBox(q - vec2(1.0, -15.0), vec2(8.0, 6.5), 2.0);
        acc = layerOutlined(acc, head, mainC * 1.1, kInk, 0.8);
        float slit = sdBox(q - vec2(2.5, -15.5), vec2(5.0, 0.9 + tell * 0.6));
        acc = layerGlow(acc, slit, glowC, 3.0, 0.7);
        acc = layerFill(acc, slit, glowC * 2.0);
        // Arms: the fist lifts during a slam's tell.
        float lift = (state == 2u ? 1.0 : tell) * 16.0;
        acc = layerOutlined(acc, sdSegment(q, vec2(13.0, -6.0), vec2(17.0, 6.0 - lift), 4.0), mainC * 0.85, kInk, 0.8);
        acc = layerOutlined(acc, sdRoundBox(q - vec2(17.5, 10.0 - lift), vec2(5.5, 5.0), 2.0), mainC * 0.95, kInk, 0.8);
        acc = layerOutlined(acc, sdSegment(q, vec2(-13.0, -6.0), vec2(-16.0, 6.0), 4.0), mainC * 0.8, kInk, 0.8);
        acc = layerOutlined(acc, sdRoundBox(q - vec2(-16.5, 10.0), vec2(5.0, 4.6), 2.0), mainC * 0.9, kInk, 0.8);
    } else if (kind == 3) {
        // Prism Wyrm's head. Body 34 x 30: crystal wings beating behind a horned head.
        float beat = sin(time * 7.0);
        for (int side = -1; side <= 1; side += 2) {
            vec2 r = vec2(q.x + 4.0, q.y * float(side) + 2.0);
            float wing = sdEllipse(rotate2d(r - vec2(-4.0, -7.0 - beat * 3.0 * float(side)), 0.9 * float(side) - 0.5), vec2(9.0, 4.0));
            acc = over(vec4(glowC * 1.3, coverage(wing) * 0.7), acc);
            acc = over(vec4(glowC * 2.2, coverage(abs(wing) - 0.5)), acc);
        }
        float skull = sdEllipse(q - vec2(2.0, 0.0), vec2(11.0, 7.5));
        float snout = sdSegment(q, vec2(4.0, 1.0), vec2(15.0, 2.5), 3.4);
        float headD = min(skull, snout);
        acc = layerGlow(acc, headD, glowC, 5.0, 0.35);
        acc = layerOutlined(acc, headD, mainC, kInk, 0.8);
        acc = layerFill(acc, max(headD + 1.5, q.y - 0.5), mainC * 1.35);
        float horns = min(sdSegment(q, vec2(-2.0, -5.0), vec2(-10.0, -13.0), 1.4), sdSegment(q, vec2(2.0, -6.0), vec2(-3.0, -15.0), 1.2));
        acc = layerOutlined(acc, horns, glowC * 1.4, kInk, 0.6);
        float jaw = state == 2u || state == 3u ? 2.5 : tell * 2.5;
        acc = layerFill(acc, sdSegment(q, vec2(6.0, 3.0), vec2(15.0, 3.0 + jaw), 0.6), kInk);
        float eye = sdEllipse(q - vec2(6.0, -2.0), vec2(1.8, 1.2));
        acc = layerGlow(acc, eye, vec3(2.4, 2.4, 2.6), 2.0, 0.7);
        acc = layerFill(acc, eye, vec3(2.6, 2.6, 2.8));
    } else {
        // Molten Heart. Body 56 x 56: a beating core behind plates of rock.
        float beat = pow(abs(sin(time * (2.5 + phase))), 6.0);
        float core = sdCircle(q, 15.0 + beat * 2.0);
        acc = layerGlow(acc, core, glowC, 14.0, 0.5 + beat * 0.3);
        float n = fbm(q / 6.0 + vec2(time * 0.4, -time * 0.2), 3);
        acc = layerFill(acc, core, mix(glowC * 1.4, vec3(2.8, 2.0, 0.8), smoothstep(0.4, 0.8, n)));
        float veins = abs(warpedNoise(q / 8.0 + vec2(time * 0.1), 1.2, 2) - 0.5);
        acc = over(vec4(glowC * 0.6, smoothstep(0.05, 0.0, veins) * coverage(core)), acc);
        // Plates: open while it is exposed (state 2..4 and the tell).
        float open = state >= 2u && state <= 4u ? 1.0 : tell;
        for (int i = 0; i < 6; ++i) {
            float a = float(i) * kPi / 3.0 + time * 0.2;
            vec2 dir = vec2(cos(a), sin(a));
            vec2 c = dir * (19.0 + open * 6.0);
            vec2 r = rotate2d(q - c, -a);
            float plate = sdRoundBox(r, vec2(4.5, 10.0), 2.5);
            acc = layerOutlined(acc, plate, mainC, kInk, 0.9);
            acc = layerFill(acc, sdBox(r - vec2(-1.0, 0.0), vec2(0.6, 6.0)), glowC * (0.8 + open));
        }
        // Tendrils reaching down.
        for (int i = 0; i < 3; ++i) {
            float x = -12.0 + float(i) * 12.0;
            float k = sin(time * 2.0 + float(i) * 2.0) * 3.0;
            acc = layerOutlined(acc, sdSegment(q, vec2(x, 16.0), vec2(x + k, 27.0), 2.0 - float(i % 2) * 0.5), mainC * 0.7, kInk, 0.6);
        }
    }
    if (state == 6u) {
        acc.rgb = mix(acc.rgb, vec3(2.0), 0.3 + 0.3 * sin(time * 30.0));
    }
    if (flash > 0.0) {
        acc.rgb = mix(acc.rgb, vec3(2.6), flash * 0.75);
    }
    outColor = acc;
}
