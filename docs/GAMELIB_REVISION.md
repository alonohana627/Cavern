# GameLib revision

Cavern fetches GameLib from GitHub at a pinned commit (`CMakeLists.txt`,
`FetchContent_Declare(GameLib ... GIT_TAG <hash>)`). Each row is a GameLib
commit the game was built and run with (zero warnings, zero validation
messages); bump the pin only together with a new row.

| Date | GameLib commit | Why |
|---|---|---|
| 2026-10-11 | `f68910fbb17155302eb3bebcdd2a0ca39c7bf183` (release 0.0.9) | the newest GameLib main when Cavern started |
| 2026-10-11 | `425aad145b7ea431cd1a945321e1fdb1ea775fb8` | GameLib's attack timelines (`core::Attacker`), ground pound (`core::GroundPound`) and connected crumble patches, built from Cavern's first P0 report; the game's own copies are gone |
| 2026-10-11 | `9e37db65844220a033d66d6810aa319e33b78e84` | GameLib's room grid (`layoutGrid`), tile groups and sealed borders, built from Cavern's second P0 report: the game's edge check, door and breakable grouping and room seal are gone (unsealed water now stays water) |
| 2026-10-11 | `4ee5fb972354d6b4a7d15051de3aa5678aa170b3` | GameLib's swimming (`TileShape::Water`), no-cling walls, `liftIntoRoom`, `SafeGround`, `inReach` and `Vulnerable::guardedHit`, built from Cavern's second P0 report: the game's swim tuning and surface leap, glass check, room lift, safe-floor tracking, station reach test and guard overlap test are gone |
