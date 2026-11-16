# AI Deco

Describe a decoration, get a finished design.

## How to use
1. Open a level in the editor.
2. **Hover a block**: a small <cy>AI</c> button appears on it. Click it (or press **Alt+G**).
3. Type what it should look like, e.g. *"mossy stone bricks with vines"*, *"neon platform"*, *"a glowing street lamp at night"*.
4. Set the **object limit** and **size** sliders, then press **Generate**.

- Hovering a block decorates the whole connected structure (touching solid blocks).
- Select several objects first and hover one of them to decorate exactly your selection.
- Hover empty space (or use the toolbar button with nothing selected) to create a free-standing design.
- **Reroll last** replaces the previous design with a new variation. Everything is one undo step.

## Styles it knows
Themes: neon, lava, ice, forest, ocean, space, crystal, desert, candy, spooky, castle, gold, toxic, night, sky, tech, hell, sunset, cozy, garden, sakura, modern. Plus any color words ("red and gold").

Subjects: trees (oak, pine, palm, dead, cherry), bushes, crystals, rocks, mushrooms, flowers, grass, cactus, floating islands, mountains, lamps, torches, campfires, candles, lanterns, towers, castles, houses, city skylines, pillars, arches, portals, pyramids, fences, signs, clouds, sun, moon, planets, stars, gears, chains, spikes, waterfalls, waves, orbs, hearts, ghosts, skulls, rockets, UFOs, crowns, snowmen and abstract art.

Surfaces: bricks, stripes, grid, dots, panels, checker, cracks, tiles, grass, moss, snow, icicles, crystals, spikes, drips, vines, flowers, outline, glow. Add "pixel" for pixel art.

## Claude (optional)
Add an Anthropic API key in the settings and turn on **Claude AI** in the popup to have Claude design it instead. The built-in design is sent as a starting draft.

## Learn from examples
Select a decoration you like, type a name, press **Save example**. Describing something with the same words places your example. Example packs (`.json`) can be dropped into the examples folder (folder button).
