#include "bn_core.h"
#include "bn_format.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_memory.h"
#include "bn_regular_bg_item.h"
#include "bn_regular_bg_map_cell_info.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_unique_ptr.h"
#include "bn_vector.h"

#include "bn_bg_palette_items_bar_palette.h"
#include "bn_regular_bg_items_garden.h"
#include "bn_regular_bg_tiles_items_bar_tiles.h"

#include "font.h"

constexpr int max_hp = 37;
constexpr int bar_cells = 10;                // The inside of the bar, in 8x8 cells.
constexpr int bar_width = bar_cells * 8;     // 80 pixels.

// In bar_tiles.bmp: tile 1 + n is an inside cell with n of its 8 columns filled.
constexpr int empty_tile = 1;
constexpr int left_end_tile = 10;
constexpr int right_end_tile = 11;

// Where the bar starts in the map. A 256x256 map at (0, 0) shows map cells 1 to 30 across and
// 6 to 25 down, so this is 8 pixels from the screen's left edge and 8 from its top.
constexpr int bar_x = 2;
constexpr int bar_y = 7;

// The bar's own background map, in RAM so the code can write it. Cleared to tile 0: see-through.
struct bar_map
{
    alignas(int) bn::regular_bg_map_cell cells[32 * 32];
    bn::regular_bg_map_item map_item;

    bar_map() :
        map_item(cells[0], bn::size(32, 32))
    {
        bn::memory::clear(cells);
        set(-1, left_end_tile);
        set(bar_cells, right_end_tile);
        draw(bar_width);
    }

    void set(int cell, int tile)
    {
        bn::regular_bg_map_cell_info info;
        info.set_tile_index(tile);
        cells[map_item.cell_index(bar_x + 1 + cell, bar_y)] = info.cell();
    }

    // Full cells, then one cell with what's left, then empty ones.
    void draw(int filled_pixels)
    {
        for(int cell = 0; cell < bar_cells; ++cell)
        {
            set(cell, empty_tile + bn::clamp(filled_pixels - cell * 8, 0, 8));
        }
    }
};

int main()
{
    bn::core::init();

    bn::regular_bg_ptr garden = bn::regular_bg_items::garden.create_bg(0, 0);

    bn::unique_ptr<bar_map> map(new bar_map());
    bn::regular_bg_item bar_item(bn::regular_bg_tiles_items::bar_tiles, bn::bg_palette_items::bar_palette,
                                 map->map_item);
    bn::regular_bg_ptr bar = bar_item.create_bg(0, 0);
    bn::regular_bg_map_ptr bar_cells_map = bar.map();
    bar.set_priority(0);   // In front of the garden, which keeps the default priority of 3.

    bn::sprite_text_generator text_generator(hud::font);
    bn::vector<bn::sprite_ptr, 4> text_sprites;

    int hp = max_hp;
    int shown = bar_width;   // The pixels the bar shows now: they walk toward the hp's pixels.
    int shown_hp = -1;

    while(true)
    {
        if(bn::keypad::a_pressed())
        {
            hp = bn::max(hp - 7, 0);
        }
        else if(bn::keypad::b_pressed())
        {
            hp = bn::min(hp + 10, max_hp);
        }

        // HP and pixels are different units. Multiply first: hp / max_hp alone is 0 until it's full.
        int target = hp * bar_width / max_hp;

        if(shown != target)
        {
            shown += (target > shown) - (target < shown);   // One pixel a frame, either way.
            map->draw(shown);
            bar_cells_map.reload_cells_ref();
        }

        if(hp != shown_hp)
        {
            shown_hp = hp;
            text_sprites.clear();
            text_generator.generate(-4, -68, bn::format<16>("HP {} of {}", hp, max_hp), text_sprites);
        }

        bn::core::update();
    }
}
