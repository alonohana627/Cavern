# GameLib revision

Cavern fetches GameLib from GitHub at a pinned commit (`CMakeLists.txt`,
`FetchContent_Declare(GameLib ... GIT_TAG <hash>)`). Each row is a GameLib
commit the game was built and run with (zero warnings, zero validation
messages); bump the pin only together with a new row.

| Date | GameLib commit | Why |
|---|---|---|
| 2026-10-11 | `f68910fbb17155302eb3bebcdd2a0ca39c7bf183` (release 0.0.9) | the newest GameLib main when Cavern started |
