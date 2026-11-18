#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"

#include "bn_regular_bg_items_garden.h"
#include "bn_sprite_items_hero.h"

#include "hero_walk.h"

// The level, one character per 16x16 cell: # is a bush, . is grass. 15x10 cells fill the screen.
// snippets/tools/grid_assets.py reads these lines to draw the background, so the two always match.
constexpr const char* level[] = {
    "###############",
    "#.....#.......#",
    "#.###.#.#####.#",
    "#.#.....#.....#",
    "#.#.###.#.###.#",
    "#...#.....#...#",
    "###.#.###.#.#.#",
    "#.....#...#.#.#",
    "#.###...#.....#",
    "###############",
};

constexpr int cell_size = 16;
constexpr int speed = 2;          // Pixels per frame. 16 / 2 = 8 frames per cell.
constexpr int feet_offset = -6;   // The hero's boots sit 10 pixels under its center: this puts them in the cell.

bool is_wall(int x, int y)
{
    return level[y][x] == '#';
}

// The cell's center on the screen: cell (0, 0) is the top left one.
bn::fixed_point cell_position(int x, int y)
{
    return bn::fixed_point(x * cell_size - 112, y * cell_size - 72 + feet_offset);
}

int main()
{
    bn::core::init();

    bn::regular_bg_ptr garden = bn::regular_bg_items::garden.create_bg(0, 0);

    // The hero's cell is the truth: the game's rules only look at it. The sprite just follows it.
    int cell_x = 1;
    int cell_y = 1;
    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(cell_position(cell_x, cell_y));
    hero_walk walk;

    while(true)
    {
        bn::fixed_point target = cell_position(cell_x, cell_y);

        // A new step can only start once the sprite has reached its cell.
        if(hero.position() == target)
        {
            int step_x = bn::keypad::right_held() - bn::keypad::left_held();
            int step_y = step_x == 0 ? bn::keypad::down_held() - bn::keypad::up_held() : 0;   // No diagonals.

            // The wall test is on the cell it would go to, before going: it never has to back out of a bush.
            if((step_x != 0 || step_y != 0) && ! is_wall(cell_x + step_x, cell_y + step_y))
            {
                cell_x += step_x;
                cell_y += step_y;
                target = cell_position(cell_x, cell_y);
            }
        }

        // Slide toward the cell: never more than `speed`, and stopping exactly on it.
        bn::fixed_point delta = target - hero.position();
        bn::fixed_point step(bn::clamp(delta.x(), bn::fixed(-speed), bn::fixed(speed)),
                             bn::clamp(delta.y(), bn::fixed(-speed), bn::fixed(speed)));
        hero.set_position(hero.position() + step);
        walk.update(hero, (step.x() > 0) - (step.x() < 0), (step.y() > 0) - (step.y() < 0), speed);

        bn::core::update();
    }
}
