# Features

What Cavern does today, verified by running it.

## World

- A world of rooms on a grid of map cells (one cell = one screen of 32 x 18
  tiles), read from room text; the loader checks that every opening on a
  room edge meets an opening of the room behind it.
- Rooms join through their edges with a short screen scroll (GameLib's
  `RoomScroll`); the camera stays inside the current room.
- The surface ruins' first rooms: the cliff road, the broken colonnade with a
  door struck open, and the sunken court whose floor collapses into the long
  fall to the fungal caves.

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
