#include "bn_core.h"
#include "bn_format.h"
#include "bn_keypad.h"
#include "bn_memory.h"
#include "bn_regular_bg_item.h"
#include "bn_regular_bg_map_cell_info.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_regular_bg_ptr.h"
#include "bn_seed_random.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_unique_ptr.h"
#include "bn_vector.h"

#include "bn_bg_palette_items_cave_palette.h"
#include "bn_regular_bg_tiles_items_cave_tiles.h"

#include "font.h"

// The cave is one screen of 8x8 cells: true is rock, false is floor.
constexpr int columns = 30;
constexpr int rows = 20;
constexpr int rock_percent = 45;     // Of the first noise. More fills the cave in, less opens it up.
constexpr int passes = 4;
constexpr int frames_per_pass = 20;  // Slow enough to watch each pass.

struct cave
{
    bool rock[rows][columns];
};

// Every cell is rock or floor at random, and the edges are always rock. The same seed gives the
// same noise, and so the same cave, because nothing else draws numbers from this generator.
void fill_noise(cave& noise, unsigned seed)
{
    // bn::seed_random counts up from its seed and scrambles the count. The noise uses 504 counts,
    // so seeds 1024 counts apart never share one: seed 2 isn't seed 1 shifted by a cell.
    bn::seed_random random(seed * 1024);

    for(int y = 0; y < rows; ++y)
    {
        for(int x = 0; x < columns; ++x)
        {
            bool edge = x == 0 || y == 0 || x == columns - 1 || y == rows - 1;
            noise.rock[y][x] = edge || random.get_int(100) < rock_percent;
        }
    }
}

int rock_around(const cave& from, int x, int y)
{
    int count = 0;

    for(int dy = -1; dy <= 1; ++dy)
    {
        for(int dx = -1; dx <= 1; ++dx)
        {
            int nx = x + dx;
            int ny = y + dy;
            bool outside = nx < 0 || ny < 0 || nx >= columns || ny >= rows;

            if((dx != 0 || dy != 0) && (outside || from.rock[ny][nx]))
            {
                ++count;
            }
        }
    }

    return count;
}

// One pass: a cell with more than 4 rock neighbors becomes rock, fewer than 4 becomes floor,
// exactly 4 stays as it was. It reads only `from` and writes only `to`, so every cell
// sees its neighbors as they were before the pass.
void smooth(const cave& from, cave& to)
{
    for(int y = 0; y < rows; ++y)
    {
        for(int x = 0; x < columns; ++x)
        {
            int count = rock_around(from, x, y);
            to.rock[y][x] = count > 4 || (count == 4 && from.rock[y][x]);
        }
    }
}

// The background's map lives in RAM, so the code can write its cells.
struct cave_map
{
    static constexpr int map_columns = 32;
    static constexpr int map_rows = 32;

    alignas(int) bn::regular_bg_map_cell cells[map_columns * map_rows];
    bn::regular_bg_map_item map_item;

    cave_map() :
        map_item(cells[0], bn::size(map_columns, map_rows))
    {
        bn::memory::clear(cells);   // Tile 0 everywhere: solid rock around the screen.
    }

    // A 256x256 map at (0, 0) shows its cells 1 to 30 across and 6 to 25 down.
    void draw(const cave& shown)
    {
        for(int y = 0; y < rows; ++y)
        {
            for(int x = 0; x < columns; ++x)
            {
                bn::regular_bg_map_cell_info info;
                info.set_tile_index(shown.rock[y][x] ? 0 : 1 + (x * 7 + y * 3) % 4);
                cells[map_item.cell_index(x + 1, y + 6)] = info.cell();
            }
        }
    }
};

int main()
{
    bn::core::init();

    bn::unique_ptr<cave_map> map(new cave_map());
    bn::regular_bg_item bg_item(bn::regular_bg_tiles_items::cave_tiles, bn::bg_palette_items::cave_palette,
                                map->map_item);
    bn::regular_bg_ptr bg = bg_item.create_bg(0, 0);
    bn::regular_bg_map_ptr bg_map = bg.map();

    bn::sprite_text_generator text_generator(hud::font);
    bn::vector<bn::sprite_ptr, 16> text_sprites;

    // Two caves: each pass reads one and writes the other, then they swap.
    cave caves[2];
    int current = 0;
    unsigned seed = 1;
    int done_passes = 0;
    int frames = 0;
    bool regenerate = true;

    while(true)
    {
        if(bn::keypad::a_pressed())
        {
            ++seed;
            regenerate = true;
        }
        else if(bn::keypad::b_pressed() && seed > 1)
        {
            --seed;
            regenerate = true;
        }

        bool changed = false;

        if(regenerate)
        {
            fill_noise(caves[current], seed);
            done_passes = 0;
            frames = 0;
            regenerate = false;
            changed = true;
        }
        else if(done_passes < passes && ++frames == frames_per_pass)
        {
            smooth(caves[current], caves[1 - current]);
            current = 1 - current;
            ++done_passes;
            frames = 0;
            changed = true;
        }

        if(changed)
        {
            map->draw(caves[current]);
            bg_map.reload_cells_ref();

            text_sprites.clear();
            text_generator.generate(-112, -72, bn::format<32>("Seed {}  Pass {} of {}", seed, done_passes, passes),
                                    text_sprites);
        }

        bn::core::update();
    }
}
