# Cavern

A small Metroidvania built on [GameLib](https://github.com/alonohana627/GameLib):
a hooded wanderer falls through the floor of a ruined court into the caves
below and must find a way back to the daylight. One connected world in five
areas - the surface ruins, the fungal caves, the flooded halls, the crystal
depths and the core - opens up as the wanderer finds abilities. Everything on
screen is drawn by shaders; every sound and the music are synthesized.

## Controls

| Action | Keys | Gamepad |
|---|---|---|
| Move | Arrows / A D | D-pad, left stick |
| Jump (hold for height) | Z, Space, K | A |
| Slash (hold Up / Down to aim) | X, J | X |
| Shoot a spark bolt | C, I | Y |
| Dash (once found) | Left Shift, L | RB, B |
| Ground pound (once found) | Down + Jump in the air | Down + A |
| Read a sign, rest at a shrine, study a map | Up / W | D-pad up |
| Map | M, Tab | Back |
| Pause | Escape, P | Start |

All keys can be rebound under CONTROLS. F11 toggles fullscreen.

## Build and run

Requirements are GameLib's (C++26 Clang, CMake 3.28+, Ninja, Vulkan 1.3,
glslc). GameLib is downloaded from GitHub at the commit pinned in
`CMakeLists.txt`.

```powershell
cmake --preset debug
cmake --build --preset debug
build/debug/bin/cavern.exe          # --demo plays the recorded demo
ctest --preset debug                # smoke run: 120 frames with the validation layer
```

Settings and bests are kept in `cavern.ini`, the three journeys in
`saves/cavern-<n>.sav` in the user data folder (`%APPDATA%\Cavern` on
Windows; `CAVERN_DATA_DIR` moves them).
