#include "bn_algorithm.h"
#include "bn_core.h"
#include "bn_fixed.h"
#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_level.h"
#include "bn_sprite_items_player.h"

#include "font.h"

constexpr bn::fixed gravity = 0.25;
constexpr bn::fixed jump_speed = -5.5;
constexpr bn::fixed max_fall_speed = 4;
constexpr bn::fixed walk_speed = 1.5;
constexpr int feet = 12;       // From the sprite's center down to its feet.
constexpr int step_up = 4;     // How far above the feet a slope is looked for: a 45° slope rises 1.5 per step.
constexpr int stick_down = 4;  // Walking down, ground this close below the feet pulls them down to it.

// The level, one character per 18x18 cell: '#' is solid, '/' and '\' are slopes going up to the right and
// to the left, one pixel up per pixel across. snippets/tools/slopes_assets.py draws it from here.
constexpr int cell = 18;
constexpr int columns = 14;
constexpr int rows = 9;
constexpr int origin_x = -126;   // Screen position of the level's top-left corner.
constexpr int origin_y = -81;

constexpr const char* level[rows] = {
    "..............",
    "..............",
    "..............",
    "..............",
    "..............",
    "....../#\\.....",
    "...../###\\....",
    "..../#####\\...",
    "##############",
};

constexpr int no_ground = 1000;

// The highest ground in column x at or below row y: the top of a block, or a slope's surface in that column.
// One column, under the middle of the feet: on a slope, the corners of a box would stand on air.
int ground(int x, int y)
{
    int column = (x - origin_x) / cell;
    int local_x = (x - origin_x) % cell;

    for(int row = bn::max((y - origin_y) / cell, 0); row < rows; ++row)
    {
        int top = origin_y + row * cell;
        char kind = level[row][column];
        int surface = kind == '#' ? top :
                      kind == '/' ? top + cell - 1 - local_x :
                      kind == '\\' ? top + local_x : no_ground;

        if(surface >= y && surface != no_ground)
        {
            return surface;
        }
    }

    return no_ground;
}

int main()
{
    bn::core::init();

    bn::regular_bg_ptr background = bn::regular_bg_items::level.create_bg(0, 0);
    bn::sprite_ptr player = bn::sprite_items::player.create_sprite(-100, 63 - feet);
    bn::sprite_text_generator text_generator(hud::font);
    bn::vector<bn::sprite_ptr, 8> text;
    const char* shown = "";

    bn::fixed velocity_y = 0;
    bool on_ground = true;

    while(true)
    {
        int direction = bn::keypad::right_held() - bn::keypad::left_held();
        bn::fixed x = bn::clamp(player.x() + direction * walk_speed, bn::fixed(-118), bn::fixed(117));

        if(bn::keypad::a_pressed() && on_ground)
        {
            velocity_y = jump_speed;
        }

        velocity_y = bn::min(velocity_y + gravity, max_fall_speed);
        bn::fixed old_feet = player.y() + feet;
        bn::fixed new_feet = old_feet + velocity_y;

        // Look for ground from a little above the feet, so walking up a slope finds the next pixel up.
        int below = ground(x.floor_integer(), old_feet.floor_integer() - step_up);
        bool falling = velocity_y >= 0;
        bool reaches = new_feet >= below;                                   // Fell onto it.
        bool sticks = on_ground && below - old_feet.floor_integer() <= stick_down;   // Walked down onto it.
        on_ground = falling && (reaches || sticks);

        if(on_ground)
        {
            new_feet = below;
            velocity_y = 0;
        }

        player.set_position(x, new_feet - feet);

        if(direction != 0)
        {
            player.set_horizontal_flip(direction > 0);   // The art faces left.
        }

        int column = (x.floor_integer() - origin_x) / cell;
        int row = (new_feet.floor_integer() - origin_y) / cell;
        char under = row < rows ? level[row][column] : '.';
        const char* on = ! on_ground ? "Air" : under == '/' || under == '\\' ? "Slope" : "Ground";

        if(on != shown)
        {
            shown = on;
            text.clear();
            text_generator.generate(-110, -71, on, text);
        }

        bn::core::update();
    }
}
