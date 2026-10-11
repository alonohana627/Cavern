# Features

What Cavern does today, verified by running it.

## World

- A world of rooms on a grid of map cells (one cell = one screen of 32 x 18
  tiles), read from room text; the loader checks that every opening on a
  room edge meets an opening of the room behind it.
- Rooms join through their edges with a short screen scroll (GameLib's
  `RoomScroll`); the camera stays inside the current room.
- Fifty-seven rooms in five areas (surface ruins, fungal caves, flooded
  halls, crystal depths, the core), from single screens to a 2 x 2 arena
  and a 4-screen vent: the game opens with the floor of a ruined court
  collapsing into the long fall to the fungal caves.
- Abilities as gates: the shadow dash crosses 7-tile thorn gaps, the cling
  claws climb narrow shafts, the quake stomp breaks cracked floors, the echo
  wings reach 5-tile glass ledges; four key-colored doors and one-way doors
  that open only from the far side (shortcuts).

## The wanderer

- GameLib's platformer controller tuned for a nimble explorer: quick starts
  and stops, a variable jump with an apex hang, coyote time and a jump
  buffer, a corner nudge, ledges to drop through.
- Thorns hurt and put the wanderer back on the last safe floor; masks of
  health; death returns to the last shrine.

## Presentation

- Shader-drawn rock per area, thorns, ledges, collapsing flagstones, doors,
  signs and shrines; a hooded wanderer with a trailing scarf and a lamp.
- A dusk sky with stars, a moon and ruined towers over the ruins (three
  parallax layers per area), lights and bloom.
- Title menu (GameLib's `ArcadeMenus`), three save slots, area banners,
  masks, bolts and a minimap.
- The attract demo: a recorded run (GameLib's `InputRecording` and
  `Playback`) through the cliff road, the colonnade door, the collapsing
  court and the long fall to the first shrine, behind the title menu and
  full screen with `--demo`.
- Five guardians with wind-ups, phases and strikes announced from above;
  the room seals while they fight; a boss bar on a plate below the floor.
