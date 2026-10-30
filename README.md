# gbadev

An unofficial, task-oriented guide to making Game Boy Advance games with [Butano](https://github.com/GValiente/butano).

Butano's [official docs](https://gvaliente.github.io/butano/) are a complete API reference: every class and method is there. What they don't say is *how* and *when* to use them. That's usually learned by reading the examples and the source. [Tonc](https://gbadev.net/tonc/) covers the hardware in depth, but at a very low level (registers, pointers, memory layout), not through Butano. This guide sits between the two. It starts from "I want to do X" and shows which Butano pieces you need, in what order, and where they bite.

It grew out of [gg-game](https://github.com/ggdaltoso/gg-game), a set of lessons for learning GBA development with Butano. The questions that came up there drive what gets written here.

> Status: the first seven pages are live at [gbadev.ggdaltoso.dev](https://gbadev.ggdaltoso.dev).

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

The official reference stays the source of truth for signatures. Pages link to it instead of copying it.

## Rules

- **Every snippet compiles.** Each snippet is its own ROM in a folder under `snippets/`, and `make` there builds them all. Pages import the source straight from that folder, so what you read is what was built. A guide with code that doesn't build is worse than no guide.
- **Examples are our own.** They show the tool with a different example than the gg-game lesson challenges, so the guide never spoils a lesson.
- **Only CC0 art** goes in `assets/`, with a `CREDITS.md` per pack. No assets with "use in games, don't redistribute" licenses.
- **English** for pages, code and comments.

## Planned layout

```
site/        Astro Starlight: the pages, in Markdown
snippets/    one Butano ROM per snippet; `make` builds them all
assets/      CC0 art only, with CREDITS.md per pack
```

The site is planned for `gbadev.ggdaltoso.dev`, with an "unofficial guide" notice on the first page.

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

### Then, following the gg-game lessons

4. **Backgrounds:** regular backgrounds, the 4 layers, priorities, scrolling.
5. **Camera:** world and screen coordinates, `set_camera`, keeping the camera inside the map, and dead zone or smooth follow.
6. **Map collision:** turning a position into a cell, checking before moving, and one axis at a time.
7. **Palettes and colors:** 15-bit colors, the transparent index 0, shared palettes, the ways to change colors, and palette rotation.
8. Later: text and HUD, timers and random numbers, SRAM saves, audio, and effects (blending, windows, mosaic, affine, HDMA).

### Pitfalls already collected

These come from gg-game and belong in the pages above:

- **`integer()` and negatives.** `integer()` truncates toward zero, while `floor_integer()` rounds down: −0.5 becomes 0 and −1. In C++, `-1 / 16` is `0`, so do the bounds check before dividing.
- **Jitter.** Calling `.integer()` to make `bn::clamp`'s arguments the same type drops the fraction and makes a camera jitter.
- **`bn::sprite_ptr` has no default constructor.** Members of that type must go in the constructor's initializer list.
- **`-Wswitch-default`.** Butano builds with it, so every `switch` needs a `default`, even when all enum values are covered.
- **Flip leaks between animations.** Every animation must set the horizontal flip, even to `false`, or it inherits the previous one.
- **Map dimensions are in cells.** `map_item().dimensions()` is in 8x8 cells, not pixels.
- **Background wrap-around.** A camera past the map edge shows the background wrapping around.

## To do

- [x] Find CC0 assets: [Zelda-like tilesets and sprites](https://opengameart.org/content/zelda-like-tilesets-and-sprites) by ArMM1998, in `assets/zelda-like/`. Other good sources:
  - [Kenney.nl](https://kenney.nl): everything there is CC0. *Tiny Town* and *Tiny Dungeon* are 16x16 tiles with characters.
  - [OpenGameArt.org](https://opengameart.org), filtered by CC0. Beware: it also has CC-BY, which needs credit.
- [x] Set up Starlight in `site/` and the Butano project in `snippets/`.
- [x] Write the three pilot pages.
- [x] Import the repository in Netlify (settings come from `netlify.toml`).
- [x] Point `gbadev.ggdaltoso.dev` at the published site.

## Credits

- [Butano](https://github.com/GValiente/butano), by Gustavo Valiente (zlib license).
- [Tonc](https://gbadev.net/tonc/), by Jasper Vijn.
