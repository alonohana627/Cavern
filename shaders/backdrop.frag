#version 450
#include "gamelib/sprite.glsl"
#include "gamelib/sdf.glsl"
#include "gamelib/noise.glsl"

// The parallax backdrop of each area, three layers per area
// (draw::drawParallax: one full-canvas quad per layer, the scroll offset in
// params.xy, canvas units):
//   layer 0: the far wall or sky - a dusk sky with stars and a moon over
//            the ruins; in the caves a dark wall of rock with faint glows
//            (fungal spores, light shafts in the flooded halls, crystal
//            gleams, magma light rising in the core)
//   layer 1: silhouettes - ruined towers and arches, giant mushrooms with
//            glowing caps, a drowned colonnade, crystal spires, chimneys
//   layer 2: near rock - stalactites and stalagmites (hanging vines and
//            rubble on the surface)
//   color   main color   color2  highlight
//   params  xy scroll (canvas units), z area 0..4, w layer 0..2

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

const float kPi = 3.14159265;

float h1(float x, float salt)
{
    return hashCell(vec2(x, salt));
}

// Mid-layer silhouettes: returns the distance (negative inside) and a glow
// distance for lit parts.
vec2 silhouette(int area, vec2 q, float time)
{
    float w = 420.0;
    float slot = floor(q.x / w);
    float lx = q.x - (slot + 0.5) * w;
    float r = h1(slot, 1.0 + float(area));
    float r2 = h1(slot, 9.0 + float(area));
    float d = 1e3;
    float g = 1e3;
    if (area == 0) {
        // Broken towers and arches on a ridge, a few windows still lit.
        float ground = 900.0 - 30.0 * sin(q.x * 0.004 + 1.3) - 18.0 * sin(q.x * 0.011);
        d = ground - q.y;
        float x = lx + (r2 - 0.5) * 150.0;
        if (r > 0.35) {
            float hw = 30.0 + 22.0 * r2;
            float top = ground - 200.0 - 260.0 * r;
            float tower = sdBox(vec2(x, q.y - (top + ground) * 0.5), vec2(hw, (ground - top) * 0.5));
            float merlon = max(abs(fract((x + hw) / 20.0) - 0.5) * 20.0 - 5.0, abs(q.y - top) - 7.0);
            tower = max(tower, -merlon);
            float broken = sdCircle(vec2(x - hw * (r2 > 0.5 ? 1.0 : -1.0), q.y - top), hw * 0.9);
            tower = max(tower, -broken);
            d = min(d, tower);
            float window = sdRoundBox(vec2(x, q.y - (top + 70.0)), vec2(6.0, 14.0), 6.0);
            g = r > 0.6 ? max(window, tower) : 1e3;
        } else {
            float pier = min(sdBox(vec2(x - 60.0, q.y - (ground - 80.0)), vec2(14.0, 80.0)),
                             sdBox(vec2(x + 60.0, q.y - (ground - 80.0)), vec2(14.0, 80.0)));
            float span = max(sdBox(vec2(x, q.y - (ground - 175.0)), vec2(74.0, 16.0)),
                             -sdCircle(vec2(x, q.y - (ground - 140.0)), 46.0));
            d = min(d, min(pier, span));
        }
    } else if (area == 1) {
        // Giant mushrooms with glowing caps on a cave floor.
        float floorY = 1080.0 - 150.0 - 30.0 * sin(q.x * 0.006);
        d = floorY - q.y;
        float x = lx + (r2 - 0.5) * 160.0;
        float hgt = 220.0 + 260.0 * r;
        float stem = sdBox(vec2(x + sin(q.y * 0.01) * 10.0, q.y - (floorY - hgt * 0.5)), vec2(14.0 + 10.0 * r, hgt * 0.5));
        float cap = max(sdEllipse(vec2(x, q.y - (floorY - hgt)), vec2(90.0 + 50.0 * r2, 50.0 + 20.0 * r)), q.y - (floorY - hgt) - 8.0);
        d = min(d, min(stem, cap));
        g = sdEllipse(vec2(x, q.y - (floorY - hgt) - 2.0), vec2(80.0 + 50.0 * r2, 10.0));
    } else if (area == 2) {
        // A drowned colonnade: pillars and arches.
        float base = 1080.0 - 120.0;
        float px = mod(q.x, 210.0) - 105.0;
        float pillar = sdBox(vec2(px, q.y - 600.0), vec2(22.0, 500.0));
        float arch = max(sdBox(vec2(px - 105.0, q.y - 260.0), vec2(105.0, 40.0)), -sdCircle(vec2(px - 105.0, q.y - 330.0), 80.0));
        float broken = step(0.7, h1(floor(q.x / 210.0), 4.0));
        pillar = max(pillar, broken > 0.5 ? -(q.y - (300.0 + 300.0 * h1(floor(q.x / 210.0), 5.0))) : -1e3);
        d = min(pillar, arch);
        d = min(d, base - q.y);
    } else if (area == 3) {
        // Crystal spires rising from below and hanging from above.
        float x = lx + (r2 - 0.5) * 180.0;
        float hgt = 300.0 + 380.0 * r;
        float lean = (r2 - 0.5) * 0.4;
        vec2 u = vec2(x - (1080.0 - q.y) * lean, 1080.0 - q.y);
        float spire = max(abs(u.x) - (hgt - u.y) * 0.16, -u.y);
        vec2 v = vec2(x * 0.7 + 60.0, q.y);
        float hang = max(abs(v.x - 120.0) - (200.0 * r2 - v.y) * 0.14, -v.y);
        d = min(spire, hang);
        g = d;
    } else {
        // The core: chimneys, pipes and falls of lava.
        float x = lx + (r2 - 0.5) * 150.0;
        float hgt = 300.0 + 350.0 * r;
        float chimney = sdBox(vec2(x, q.y - (1080.0 - hgt * 0.5)), vec2(30.0 + 20.0 * r2, hgt * 0.5));
        float pipe = sdBox(vec2(lx, q.y - (300.0 + 300.0 * r2)), vec2(w * 0.5, 9.0));
        d = min(chimney, pipe);
        float fall = sdBox(vec2(x + 90.0, q.y - 540.0), vec2(6.0 + 4.0 * sin(q.y * 0.05 + time * 4.0), 540.0));
        g = r > 0.55 ? fall : 1e3;
    }
    return vec2(d, g);
}

void main()
{
    SpriteData s = sprite;
    vec2 q = inUv * s.size + s.params.xy; // canvas units, scrolled
    vec2 uv = inUv;
    int area = int(s.params.z + 0.5);
    int layer = int(s.params.w + 0.5);
    const bool dawn = area == 5; // the ending: the ruins at sunrise
    if (dawn) {
        area = 0;
    }
    if (area != 0 && layer > 0) {
        q.y = mod(q.y, 1080.0); // the caves repeat downwards: every room has a floor and a ceiling
    }
    float time = mod(s.time, 3600.0);
    vec3 main = s.color.rgb;
    vec3 high = s.color2.rgb;
    vec4 c = vec4(0.0);

    if (layer == 0) {
        if (area == 0) {
            // Dusk sky: a gradient, stars, a pale moon, thin clouds.
            vec3 col = mix(main, high, smoothstep(0.0, 1.0, uv.y));
            vec2 cellSize = vec2(37.0, 31.0);
            vec2 sc = floor(q / cellSize);
            float star = hashCell(sc);
            vec2 sp = (fract(q / cellSize) - 0.5) * cellSize;
            float tw = 0.6 + 0.4 * sin(time * 2.0 + star * 40.0);
            col += vec3(0.9, 0.9, 1.0) * step(0.93, star) * coverage(length(sp) - 1.2) * tw * (1.0 - uv.y);
            if (dawn) {
                // A low sun rising behind the ruins.
                float sun = length(inUv * s.size - vec2(1440.0, 860.0)) - 80.0;
                col += vec3(1.0, 0.7, 0.35) * glowFalloff(sun, 520.0) * 0.6;
                col = mix(col, vec3(1.8, 1.4, 0.9), coverage(sun));
            } else {
                float moon = length(q - vec2(1450.0, 240.0)) - 70.0;
                col = mix(col, vec3(1.0, 0.95, 0.85), coverage(moon) * 0.9);
                col += vec3(0.6, 0.55, 0.7) * glowFalloff(moon, 180.0) * 0.25;
            }
            float cloud = fbm(vec2(q.x * 0.002 + time * 0.01, q.y * 0.008), 4);
            col = mix(col, high * 1.2, smoothstep(0.55, 0.8, cloud) * 0.35 * smoothstep(0.2, 0.6, uv.y));
            c = vec4(dither(col, gl_FragCoord.xy, 0.05), 1.0);
        } else {
            // A cave's far wall: dark rock blotches and faint area glows.
            float n = fbm(q / 260.0, 4);
            float n2 = fbm(q / 90.0 + 7.0, 3);
            vec3 col = mix(main, main * 1.8, smoothstep(0.35, 0.75, n)) * (0.85 + 0.25 * n2);
            col = mix(col, main * 0.6, smoothstep(0.5, 0.1, uv.y) * 0.5);
            if (area == 1) {
                // Drifting spores glowing in the dark.
                vec2 cellSize = vec2(53.0, 47.0);
                vec2 sc = floor((q + vec2(0.0, time * 6.0)) / cellSize);
                float h = hashCell(sc);
                vec2 sp = (fract((q + vec2(0.0, time * 6.0)) / cellSize) - 0.5) * cellSize;
                float pulse = 0.5 + 0.5 * sin(time * 1.5 + h * 30.0);
                col += high * step(0.9, h) * glowFalloff(length(sp) - 1.0, 8.0) * pulse * 0.6;
            } else if (area == 2) {
                // Shafts of pale light from above.
                float shaft = sin(q.x * 0.008 + 0.7) * 0.5 + 0.5;
                shaft = smoothstep(0.75, 1.0, shaft) * (1.0 - uv.y) * (0.7 + 0.3 * sin(time * 0.6 + q.x * 0.001));
                col += high * shaft * 0.18;
            } else if (area == 3) {
                float gleam = smoothstep(0.62, 0.8, fbm(q / 150.0 + vec2(time * 0.02, 0.0), 3));
                col += high * gleam * 0.18;
            } else if (area == 4) {
                col += high * smoothstep(0.4, 1.0, uv.y) * (0.25 + 0.05 * sin(time * 1.3));
            }
            c = vec4(dither(col, gl_FragCoord.xy, 0.06), 1.0);
        }
    } else if (layer == 1) {
        vec2 sil = silhouette(area, q, time);
        float d = sil.x;
        float a = coverage(d);
        vec3 col = main * (0.9 + 0.2 * valueNoise(q / 40.0));
        // A rim light on the silhouettes' edges (wider and brighter on the
        // drowned colonnade, whose blue stone would sink into the dark).
        if (area == 2) {
            col = mix(col * 1.25, high * 0.8, smoothstep(-10.0, 0.0, d) * 0.65);
        } else {
            col = mix(col, high * 0.5, smoothstep(-6.0, 0.0, d) * 0.4);
        }
        c = vec4(col, a);
        if (sil.y < 1e2) {
            float pulse = 0.7 + 0.3 * sin(time * 1.2 + q.x * 0.01);
            vec4 glow = vec4(high * 1.1, glowFalloff(max(sil.y, 0.0), 40.0) * 0.5 * pulse);
            c = over(vec4(high * 1.3, coverage(sil.y) * 0.9 * pulse), over(glow, c));
        }
        if (area == 2) {
            // The flood: the colonnade stands in water up to a rippling
            // surface line, with caustics playing below it.
            float wave = sin(q.x * 0.02 + time * 1.1) * 4.0 + sin(q.x * 0.007 - time * 0.7) * 6.0;
            float below = q.y - (760.0 + wave);
            float caustic = abs(sin(q.x * 0.035 + time * 0.9 + sin(q.y * 0.04 + time * 0.6)) * sin(q.y * 0.05 - time * 0.8));
            vec3 waterC = main * 0.9 + high * (0.05 + smoothstep(0.8, 1.0, caustic) * 0.14);
            c = over(vec4(waterC, coverage(-below) * coverage(q.y - 960.0) * 0.5), c); // down to the colonnade floor
            c = over(vec4(high * 0.7, coverage(abs(below) - 1.5) * 0.6), c);
        }
    } else {
        // Near rock: stalactites hanging from the top and stalagmites rising from
        // the bottom (vines and rubble on the surface).
        float w = 150.0;
        float slot = floor(q.x / w);
        float lx = q.x - (slot + 0.5) * w;
        float r = h1(slot, 21.0 + float(area));
        float r2 = h1(slot, 31.0 + float(area));
        float topBand = 60.0 + 40.0 * valueNoise(vec2(q.x / 90.0, 3.0));
        float botBand = 50.0 + 40.0 * valueNoise(vec2(q.x / 80.0, 9.0));
        float d;
        if (area == 0) {
            float vine = abs(lx + sin(q.y * 0.03 + r * 6.0) * 8.0) - 3.0;
            vine = max(vine, q.y - (120.0 + 280.0 * r));
            float rubble = (1080.0 - botBand * 0.6) - q.y;
            d = min(r2 > 0.5 ? vine : 1e3, rubble);
        } else {
            float lenT = 100.0 + 260.0 * r;
            float tite = max(abs(lx) - (lenT - (q.y - topBand)) * 0.12, q.y - topBand - lenT);
            float lenM = 80.0 + 200.0 * r2;
            float mite = max(abs(lx - 40.0) - (lenM - (1080.0 - botBand - q.y)) * 0.16, (1080.0 - botBand - lenM) - q.y);
            d = min(min(tite, mite), min(q.y - topBand, (1080.0 - botBand) - q.y));
        }
        vec3 col = main * (0.8 + 0.3 * valueNoise(q / 25.0));
        col = mix(col, high * 0.35, smoothstep(-4.0, 0.0, d) * 0.5);
        c = vec4(col, coverage(d));
    }
    outColor = c;
}
