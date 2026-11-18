# gbadev

An unofficial, task-oriented guide to making Game Boy Advance games with [Butano](https://github.com/GValiente/butano).

Butano's [official docs](https://gvaliente.github.io/butano/) are a complete API reference: every class and method is there. What they don't say is *how* and *when* to use them. That's usually learned by reading the examples and the source. [Tonc](https://gbadev.net/tonc/) covers the hardware in depth, but at a very low level (registers, pointers, memory layout), not through Butano. This guide sits between the two. It starts from "I want to do X" and shows which Butano pieces you need, in what order, and where they bite.

It covers topics any GBA game can use. When a topic fits one kind of game best, its example comes from that genre: gravity is shown in a platformer, an enemy formation in a shooter. It started from the questions that came up in [gg-game](https://github.com/ggdaltoso/gg-game), a set of lessons for learning GBA development with Butano.

> Status: the first fifty-three pages are live at [gbadev.ggdaltoso.dev](https://gbadev.ggdaltoso.dev).

## Building

```sh
cd snippets && make                              # needs devkitARM; Butano expected at ../../butano/butano
                                                 # commit the rebuilt .gba files: the site runs them
make LIBBUTANO=/path/to/butano/butano            # or point it elsewhere
cd site && pnpm install && pnpm dev              # the site (build the snippets first: pages run their ROMs)
```

## How each page works

Each module gets one page, always with the same five sections:

1. **When to use it:** the problem it solves, and the alternatives.
2. **How to think about it:** the mental model, for example "the camera is the center of the screen" or "sprites with the same colors share a palette".
3. **The smallest thing that works:** a short, complete snippet.
4. **Pitfalls:** the mistakes people actually make, with the symptom you see and the fix.
5. **Read more:** the Butano example, the headers, and the Tonc chapter worth reading.

Most pages suit any kind of game. A page that suits one genre best (shooter, platformer, top-down, runner, puzzle and grid) shows its icon in the sidebar and under its title; the home page explains the icons.

The official reference stays the source of truth for signatures. Pages link to it instead of copying it.

## Rules

- **Every snippet compiles.** Each snippet is its own ROM in a folder under `snippets/`, and `make` there builds them all. Pages import the source straight from that folder, so what you read is what was built. A guide with code that doesn't build is worse than no guide.
- **Examples are our own.** Each one is written for the guide, small and complete, not copied from another project.
- **Only CC0 art** goes in `assets/`, with a `CREDITS.md` per pack. No assets with "use in games, don't redistribute" licenses. Good sources: [Kenney.nl](https://kenney.nl), where everything is CC0, and [OpenGameArt.org](https://opengameart.org) filtered by CC0 (it also has CC-BY, which needs credit).
- **English** for pages, code and comments.

## Layout

```
site/        Astro Starlight: the pages, in Markdown
snippets/    one Butano ROM per snippet; `make` builds them all
assets/      CC0 art only, with CREDITS.md per pack
```

## Roadmap

### Pilot: the basics

1. **First project and the game loop:** `bn::core::init`, `bn::core::update`, why everything runs inside `while(true)`, and what 60 frames per second means. There are no events: you ask for the state every frame.
2. **Input:** `pressed`, `held` and `released`, and when to use each one.
3. **Sprites and animation:**
   - importing a `.bmp` with its `.json` (frames can sit side by side: `width` and `height` cut the image);
   - `bn::sprite_ptr`;
   - positions as `bn::fixed`;
   - `sprite_animate_action`;
   - horizontal flip.

4. **Backgrounds:** regular backgrounds, the 4 layers, priorities, scrolling.
5. **Camera:** world and screen coordinates, `set_camera`, keeping the camera inside the map, and dead zone or smooth follow.
6. **Map collision:** turning a position into a cell, checking before moving, and one axis at a time.
7. **Palettes and colors:** 15-bit colors, the transparent index 0, shared palettes, the ways to change colors, and palette rotation.
8. **Text and HUD:** sprite fonts, alignment, regenerating only on change, and text that ignores the camera.
9. **Timers and random numbers:** frame counters, seeding from the player, and replaying a seed.
10. **SRAM saves:** a save struct, recognizing it with a format value, and restarting the game.
11. **Music and sound:** modules and recordings, sound effects, volume, and what a recording can't do.
29. **Dialog boxes:** a box over the game, text wrapped by measuring it, letters one by one, pages, and one button that does what fits the moment.
33. **Depth sorting:** a z order from each sprite's feet, a big thing made of several sprites that sorts as one, and the limit of 16 sort layers.
37. **Big maps:** a world bigger than 512x512 kept in video memory as a ring of 32x32 cells, a new column copied in every 8 pixels, and the 1024 different tiles a map can use.
38. **Minimap:** the world shrunk to a 64x64 sprite by the art script, markers placed by dividing world positions by the scale, and the screen's box from the camera.
39. **Save slots:** several saves at offsets worked out from the struct's size, a checksum that tells a good slot from a damaged one, and a save cut in half.
40. **Music by region:** rectangles of the world with a song each, one song at a time with a fade out and a fade in, and a walk back that doesn't restart the song.
49. **Parallax:** background layers that move a fraction of what the camera moves, how wide each one has to be, and the priorities that keep the ground in front of the sky and the player in front of the ground.
51. **Endless run:** a hero that stays put while a repeating track scrolls under it, obstacles placed by distance along the track, and a lap that keeps every number small.
52. **Jump seen from above:** height as a third axis that only changes where the hero is drawn, a shadow that stays on the ground and shrinks as the hero rises, and a bump that needs the hero low enough as well as in the way.
53. **Difficulty curve:** how hard the game is, as a table of rows by distance, blended between rows and held after the last one, and the time between obstacles that the player actually feels.

### Effects

12. **Blending:** transparency, intensity and fade.
42. **Shadows and reflections:** a half see-through shadow at the feet and behind the hero, a reflection mirrored on the feet, and a ground layer with a hole that cuts the reflection to the water.
13. **Windows:** a rectangle that chooses which layers show, and the four kinds of window.
14. **Mosaic:** bigger pixels, and a teleport transition.
15. **Rotation and scaling:** affine sprites and backgrounds.
16. **H-Blank effects:** a setting that changes line by line.
17. **HDMA:** a hardware copy on every line, for what H-Blank effects don't cover.
28. **Particles:** sparks as small tweens with random numbers, a fixed number of slots where the oldest makes room, and what particles cost per frame.
30. **Screen shake:** trauma that adds up with each hit and drains, a random offset the size of trauma squared, and a camera that gets the shake last.
31. **Animated tiles:** water and fountains that move all over a background by copying a few tiles per frame, copies that share tiles, and phases that need their own.
35. **Hitstop:** a few frozen frames on each hit, a counter that skips the world while the spark and the buttons keep running, and one hit per swing.
36. **Iris transition:** a circle-shaped window from an H-Blank effect, a transition as a state with input waiting, and the swap while the screen is black.

### Structure

18. **Scenes:** a title, a game and a result as functions, what each one owns, and fading between them.
19. **Game objects:** a class per kind of thing, a `bn::vector` of them, and removing them with `erase_if`.
43. **Object pools:** every stone made at the start and kept hidden, a throw that wakes one up and resets it, a size worked out from one stone's life and the time between throws, and what it saves, measured.
20. **Files and headers:** a class in its own `.h` and `.cpp`, include guards, and the compiler and linker errors.
21. **State machines:** an enemy with an `enum class`, one `switch`, and a single place where the state changes.
44. **Scripted cutscenes:** a cutscene as a list of steps played one at a time, each step done when its own condition says so, and a skip that leaves the world as the cutscene would.
50. **Hazards with a warning:** dangers that start on a schedule written as data, each one warning before it hurts, its phase worked out from its age, and a warning long enough to get out of the way.
22. **Performance:** CPU usage on screen, `bn::profiler`, and fixing what it finds.
45. **Input replays:** the seed and the buttons instead of what happened, a game that reads its buttons from one place, and buttons stored as runs of frames.
46. **Formations:** a wave of enemies as one point plus each enemy's cell, edges from the outermost living enemies, and steps that come more often as fewer are left.
47. **Sprite budget:** Butano's 128 sprites shared by enemies, shots and text, shot capacity worked out from what's left, the per-line limit that drops the oldest sprites, and when a background is the better deal.

### Math

23. **Fixed-point numbers:** why `bn::fixed` and not `float`, sine and cosine from a table, and where precision and range run out.
24. **Vectors and angles:** the same speed in every direction, aiming with `atan2`, and distance without a square root.
25. **Hitboxes:** `bn::fixed_rect` between moving things, boxes that match the art, and safe frames after a hit.
26. **Velocity and acceleration:** a velocity kept between frames, acceleration and friction as steps toward a target, and surfaces as data.
27. **Interpolation and easing:** a fraction of the way from one point to another, curves that change the feel without changing the duration, and arriving exactly.
32. **Line of sight:** a view cone from a dot product, a ray that walks the map cell by cell, and the cheap checks first.
34. **Cave generation:** noise from a seed, cellular automaton passes with two grids, a background map written by the code, and seeds that don't overlap.
41. **Pathfinding:** a breadth-first search out from the goal, one distance map that answers every start, and a trail that walks downhill to the goal.
48. **Gravity and jumping:** gravity as a constant pull on the vertical velocity, a top speed for falling, a jump that's higher the longer A is held, and landing on platforms by crossing their top.

## Ideas to study

- **Touch controls for the emulator:** a D-pad and buttons on screen, so the Run button works on a phone without a keyboard.

## Credits

- [Butano](https://github.com/GValiente/butano), by Gustavo Valiente (zlib license).
- [Tonc](https://gbadev.net/tonc/), by Jasper Vijn.
