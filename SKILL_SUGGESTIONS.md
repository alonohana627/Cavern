# Skill suggestions

Skills that would have made work on this project go better. See `AGENTS.md` for
how this file is maintained.

## Already installed

In `~/.claude/skills/` (checked 2026-10-11):

- action-combat-frame-data
- action-rpg-progression-design
- arcade-action-physics
- articulated-2d-character-rigs
- chiptune-music-sequencing
- chiptune-sfx-synthesis
- ci-cross-platform
- cmake-build-developer
- cmake-library-consumption
- computer-graphics-developer
- cozy-sim-design
- cpp-debugging-profiling
- cpp-performance-developer
- cpp-testing
- cpp26-developer
- deterministic-replay-testing
- game-audio-developer
- game-engine-developer
- game-feel-designer
- game-library-extraction
- game-persistence
- game-ui-text-and-dialogue
- game-visual-design
- glfw-platform-developer
- glsl-shader-build-embedding
- grid-algorithms-for-games
- headless-balance-sweeps
- input-rebinding-ui
- learning-mentor
- logic-puzzle-demo-players
- metroidvania-world-design
- platformer-boss-design
- platformer-controller-feel
- platformer-demo-bots
- platformer-level-design
- pong-arcade-rules
- procedural-dungeon-generation
- procedural-landscape-shaders
- puzzle-game-design-undo
- retro-crt-screen-shader
- sdf-2d-shape-shaders
- shader-developer
- software-architecture-developer
- stroke-font-design
- technical-docs-writer
- text-level-authoring-tools
- tilemap-and-camera-2d
- vector-arcade-neon-look
- vulkan-app-smoke-testing
- vulkan-developer
- vulkan-offscreen-postprocess
- vulkan-text-and-ui
- win32-game-window-automation
- window-screenshot-bursts
- windows-release-packaging

## Suggested

### metroidvania-reachability-checker

A ready-made offline checker for a hand-authored Metroidvania world: reads
the game's room data (tile rows per room, doors, cracked tiles, keys and
ability pickups) and the controller's tuning, explores standable positions
with the real platformer step function and a small set of macro actions
(walk, jump with several holds and directions, dash, wall jump chains,
double jump, pound), opens doors and breaks tiles the current ability set
allows, and iterates to a fixpoint over pickups; reports which items and
rooms each ability stage reaches, the first unreachable required pickup,
one-way drops that strand the player, and the shortest route between two
rooms. Include caching per room and ability set so 60 rooms run in seconds.

Why: Cavern's sixty rooms were proven completable with a throwaway driver
written from scratch around GameLib's controller; the same tool would serve
09-wayfarer and 10-realm.
