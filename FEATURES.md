# Features

What Cavern does today, verified by running it.

## World

- Fifty-seven rooms in five areas (surface ruins, fungal caves, flooded
  halls, crystal depths, the core), from single screens to a 2 x 2 arena,
  a 3-screen sluice and a 4-screen vent, placed on a grid of map cells read
  from room text; the loader checks that every opening on a room edge meets
  an opening of the room behind it.
- Rooms join through their edges with a short screen scroll (GameLib's
  `RoomScroll`), with a lift onto the floor when entering a room from below;
  the camera stays inside the current room and glides to the next.
- The game opens on the cliff road; the floor of the sunken court collapses
  into the long fall to the fungal caves.
- Abilities as gates: the shadow dash crosses 7-tile thorn gaps, the cling
  claws climb narrow shafts, the quake stomp breaks cracked floors, the echo
  wings reach 5-tile ledges of glassy crystal (which cannot be clung to);
  four key-colored doors, plain doors struck open, one-way doors that open
  only from the far side (shortcuts back to the shrines). Each ability
  reopens routes in areas already visited.
- Six hidden vitality vessels and six spark shards (behind cracked walls,
  key doors, cracked floors, up chimneys and on glass ledges), four keys
  and four abilities: twenty items counted for the completion percentage.
- Persistent world state: opened doors, broken walls and floors, items
  taken, guardians beaten and rooms visited (GameLib's `WorldFlags` and
  `MapReveal`).
- Water to swim in (GameLib's swimming: light, slow, every jump press a
  stroke, a leap out of the surface), thorns and lava that hurt and put the
  wanderer back on the last safe floor (GameLib's `SafeGround`).
- Signs and lore tablets read in a dialogue box, their key names following
  the player's bindings.
- An offline explorer with the real controller (kept outside the
  repository) reaches every room, every item and the way out in the
  intended order: dash, cling claws, quake stomp, echo wings.

## The wanderer

- GameLib's platformer controller tuned for a nimble explorer: quick starts
  and stops, a 3.4-tile variable jump with an apex hang, coyote time, a jump
  buffer, a corner nudge, ledges to drop through; the dash, wall slide and
  wall jump, double jump and ground pound switched on as they are found.
- A slash with frame data (GameLib's `Attacker`): forward, upward, and
  downward in the air with a bounce off whatever it strikes, chaining on a
  well-timed press; it cuts enemy shots out of the air and opens doors and
  cracked walls. A limited spark bolt (forward or upward).
- Masks of health with knockback and invincibility flicker; death returns
  to the last shrine with the world as it is.

## Creatures and guardians

- Fifteen creatures that return whenever their room is entered again:
  crawler beetles, bats, stone sentries, spore puffs that burst into spores,
  cap hoppers, ceiling creepers that drop, leaping eels, shellbacks shielded
  in front, jellies, charging shardlings, prism eyes with beam fans, crystal
  moths, ember wisps, bomb-dropping drones and magma lobbers. Health and
  bolt drops.
- Four area guardians and the molten heart (StateMachine + PhaseByHealth):
  the Sporemother (spore volleys, leaps, budding puffs), the Tidewarden
  (claw charges, bubbles, geysers), the Gate Colossus (fist slams with
  shockwaves, falling rubble, leaps), the Prism Wyrm (flies, shard fans,
  dives, falling shards) and the Molten Heart (plates that open and close,
  orb rings, magma from the vault). Wind-ups are drawn, strikes from above
  are announced; the room seals during the fight; a boss bar on a plate.
- The ability pedestal appears when the guardian falls; the "ability get"
  scene names the ability and its keys.
- The escape: the heart's fall opens the vent and starts a 2:30 clock;
  reaching the daylight ends the game.

## Saves, map and menus

- Three save slots (GameLib's `SaveSlots`: versioned, checksummed, with a
  backup; the world flags and map fog through GameLib's world file), chosen
  on GameLib's slot list with a summary per slot and erase asked twice;
  shrines heal, refill bolts and save.
- A map screen of rounded room blocks per area (GameLib's `WorldMap`), fogged
  by visits, with map stations charting an area's outline; markers for
  shrines, map stations, guardians not yet beaten, items seen and the
  wanderer; a minimap in the HUD corner.
- Title menu (GameLib's `ArcadeMenus`) with sound, music, screen shake,
  fullscreen, vsync and rebindable controls; pause menu with the map;
  F11 fullscreen; settings, bindings and bests in `cavern.ini`.
- The ending at sunrise with the run's time, completion and deaths, and the
  best escape time.
- The attract demo: a recorded run (GameLib's `InputRecording` and
  `Playback`) through the cliff road, the colonnade door, the collapsing
  court and the long fall to the first shrine, behind the title menu and
  full screen with `--demo`.

## Look and sound

- Shader-drawn rock per area (sandstone blocks, mossy rock with glowcaps,
  wet stone with hanging weed, violet rock sprouting crystal, basalt with
  magma veins), glassy crystal, water, lava, thorns, ledges, cracked tiles
  with visible cracks, collapsing flagstones, doors and seals; ankle-deep
  shallows on the floor of the Tidewarden's pool.
- Three parallax layers per area: a dusk sky with stars, a moon and ruined
  towers; giant glowing mushrooms; a drowned colonnade standing in rippling
  water; crystal spires; chimneys and lava falls; stalactites and
  stalagmites in front.
- A hooded wanderer with a trailing scarf and a lamp; creatures and
  guardians as layered SDF bodies; items with pulsing auras.
- Lights (the wanderer's lamp, shrines, items, glowing creatures and shots),
  bloom and a dark ambient per area; ambient motes (GameLib's `Ambient`:
  dust, spores, drips, glints, embers); shards, sparks, dust, rings, screen
  shake, flashes and hit stop.
- Synthesized effects (slash, bolt, dash whoosh, pound thud, doors, save
  chime, item and ability fanfares, guardians) and ten tunes: an ambient
  track per area crossfading at the borders, the guardians, the heart, the
  escape, the title and the ending.
