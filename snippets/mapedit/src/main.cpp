#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_memory.h"
#include "bn_regular_bg_item.h"
#include "bn_regular_bg_map_cell_info.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_unique_ptr.h"

#include "bn_bg_palette_items_garden_palette.h"
#include "bn_regular_bg_tiles_items_garden_tiles.h"
#include "bn_sprite_items_barrel.h"
#include "bn_sprite_items_hero.h"

#include "hero_walk.h"

constexpr int columns = 15;
constexpr int rows = 10;

// Not constexpr: the game writes to it. # is a bush, o is a barrel, . is grass.
char level[rows][columns + 1] = {
    "###############",
    "#..o..#.......#",
    "#.###.#.#####.#",
    "#.#...o.#.....#",
    "#.#.###.#.###.#",
    "#...#.....#.o.#",
    "###.#.###.#.#.#",
    "#.....#...#.#.#",
    "#.###...#.oo..#",
    "###############",
};

constexpr int cell_size = 16;
constexpr int speed = 2;
constexpr int feet_offset = -6;

// The first of the four 8x8 tiles of each kind of cell, in garden_tiles.bmp.
constexpr int grass_tiles = 0;   // Three kinds of grass: 0, 4 and 8.
constexpr int bush_tiles = 12;
constexpr int barrel_tiles = 16;

bn::fixed_point cell_position(int x, int y)
{
    return bn::fixed_point(x * cell_size - 112, y * cell_size - 72);
}

// The background's map, in RAM so the code can write it.
struct garden_map
{
    static constexpr int map_columns = 32;
    static constexpr int map_rows = 32;

    alignas(int) bn::regular_bg_map_cell cells[map_columns * map_rows];
    bn::regular_bg_map_item map_item;

    garden_map() :
        map_item(cells[0], bn::size(map_columns, map_rows))
    {
        bn::memory::clear(cells);
    }

    // Draws one cell of the level: a 16x16 cell is 2x2 map cells. A 256x256 map at (0, 0)
    // shows map cells 1 to 30 across and 6 to 25 down, so level cell (0, 0) starts at map cell (1, 6).
    void draw(int x, int y)
    {
        char kind = level[y][x];
        int first_tile = kind == '#' ? bush_tiles : kind == 'o' ? barrel_tiles : grass_tiles + (x * 7 + y * 3) % 3 * 4;

        for(int part = 0; part < 4; ++part)
        {
            bn::regular_bg_map_cell_info info;
            info.set_tile_index(first_tile + part);
            cells[map_item.cell_index(1 + x * 2 + part % 2, 6 + y * 2 + part / 2)] = info.cell();
        }
    }
};

int main()
{
    bn::core::init();

    bn::unique_ptr<garden_map> map(new garden_map());

    for(int y = 0; y < rows; ++y)
    {
        for(int x = 0; x < columns; ++x)
        {
            map->draw(x, y);
        }
    }

    bn::regular_bg_item garden_item(bn::regular_bg_tiles_items::garden_tiles, bn::bg_palette_items::garden_palette,
                                    map->map_item);
    bn::regular_bg_ptr garden = garden_item.create_bg(0, 0);
    bn::regular_bg_map_ptr bg_map = garden.map();

    // Every change goes through here: the level the rules read and the map the screen shows, together.
    // The map in RAM isn't what the screen shows: reload_cells_ref asks for all of it to be copied to
    // video memory again, once, on the next bn::core::update(), however many times it's called before.
    auto put = [&](int x, int y, char kind)
    {
        level[y][x] = kind;
        map->draw(x, y);
        bg_map.reload_cells_ref();
    };

    int cell_x = 1;
    int cell_y = 1;
    bn::fixed_point feet(0, feet_offset);
    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(cell_position(cell_x, cell_y) + feet);
    hero_walk walk;

    // A barrel being pushed is a sprite: map cells can only change a whole cell at a time.
    bn::sprite_ptr sliding = bn::sprite_items::barrel.create_sprite(0, 0);
    sliding.set_visible(false);
    int slide_to_x = 0;
    int slide_to_y = 0;

    while(true)
    {
        bn::fixed_point target = cell_position(cell_x, cell_y) + feet;

        if(hero.position() == target)
        {
            // The push is over: the barrel goes back into the map, one cell further.
            if(sliding.visible())
            {
                put(slide_to_x, slide_to_y, 'o');
                sliding.set_visible(false);
            }

            int step_x = bn::keypad::right_held() - bn::keypad::left_held();
            int step_y = step_x == 0 ? bn::keypad::down_held() - bn::keypad::up_held() : 0;
            int ahead_x = cell_x + step_x;
            int ahead_y = cell_y + step_y;
            char ahead = level[ahead_y][ahead_x];

            // A barrel moves only into grass: not into a bush, and not into another barrel.
            if(ahead == 'o' && level[ahead_y + step_y][ahead_x + step_x] == '.')
            {
                put(ahead_x, ahead_y, '.');
                sliding.set_position(cell_position(ahead_x, ahead_y));
                sliding.set_visible(true);
                slide_to_x = ahead_x + step_x;
                slide_to_y = ahead_y + step_y;
                ahead = '.';
            }

            if((step_x != 0 || step_y != 0) && ahead == '.')
            {
                cell_x = ahead_x;
                cell_y = ahead_y;
                target = cell_position(cell_x, cell_y) + feet;
            }
        }

        bn::fixed_point delta = target - hero.position();
        bn::fixed_point step(bn::clamp(delta.x(), bn::fixed(-speed), bn::fixed(speed)),
                             bn::clamp(delta.y(), bn::fixed(-speed), bn::fixed(speed)));
        hero.set_position(hero.position() + step);
        sliding.set_position(sliding.position() + step);
        walk.update(hero, (step.x() > 0) - (step.x() < 0), (step.y() > 0) - (step.y() < 0), speed);

        bn::core::update();
    }
}
