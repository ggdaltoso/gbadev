#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_garden.h"
#include "bn_sprite_items_bomb.h"
#include "bn_sprite_items_flame.h"
#include "bn_sprite_items_hero.h"

#include "hero_walk.h"

// The grid snippet's garden: # is a bush, . is grass, one character per 16x16 cell.
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
constexpr int speed = 2;
constexpr int feet_offset = -6;

constexpr int range = 3;           // How many cells each arm of the cross can reach.
constexpr int fuse_frames = 90;    // From A to the blast.
constexpr int flame_frames = 24;   // How long the flames stay: four frames of animation, six each.
constexpr int max_bombs = 3;

// The four arms of the cross, as steps from one cell to the next.
constexpr int arms[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };

struct bomb
{
    bn::sprite_ptr sprite;
    int x;
    int y;
    int fuse;
};

struct flame
{
    bn::sprite_ptr sprite;
    int x;
    int y;
    int life;
};

bool is_wall(int x, int y)
{
    return level[y][x] == '#';
}

bn::fixed_point cell_position(int x, int y)
{
    return bn::fixed_point(x * cell_size - 112, y * cell_size - 72);
}

bool burning(const bn::ivector<flame>& flames, int x, int y)
{
    for(const flame& fire : flames)
    {
        if(fire.x == x && fire.y == y)
        {
            return true;
        }
    }

    return false;
}

void add_flame(bn::ivector<flame>& flames, int x, int y)
{
    flames.push_back(flame{ bn::sprite_items::flame.create_sprite(cell_position(x, y)), x, y, flame_frames });
}

// The whole blast, worked out once: the bomb's cell, then each arm walking out until a bush or the range.
void explode(const bomb& blast, bn::ivector<flame>& flames)
{
    add_flame(flames, blast.x, blast.y);

    for(const auto& arm : arms)
    {
        for(int reach = 1; reach <= range; ++reach)
        {
            int x = blast.x + arm[0] * reach;
            int y = blast.y + arm[1] * reach;

            // Checked before the flame is added, and the arm ends there: no fire in the bush or past it.
            if(is_wall(x, y))
            {
                break;
            }

            add_flame(flames, x, y);
        }
    }
}

int main()
{
    bn::core::init();

    bn::regular_bg_ptr garden = bn::regular_bg_items::garden.create_bg(0, 0);

    int cell_x = 1;
    int cell_y = 1;
    bn::fixed_point feet(0, feet_offset);
    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(cell_position(cell_x, cell_y) + feet);
    hero_walk walk;
    int safe_frames = 0;

    bn::vector<bomb, max_bombs> bombs;
    bn::vector<flame, max_bombs * (1 + 4 * range)> flames;   // Every bomb blowing at once, full size.

    while(true)
    {
        bn::fixed_point target = cell_position(cell_x, cell_y) + feet;

        if(hero.position() == target)
        {
            int step_x = bn::keypad::right_held() - bn::keypad::left_held();
            int step_y = step_x == 0 ? bn::keypad::down_held() - bn::keypad::up_held() : 0;

            if((step_x != 0 || step_y != 0) && ! is_wall(cell_x + step_x, cell_y + step_y))
            {
                cell_x += step_x;
                cell_y += step_y;
                target = cell_position(cell_x, cell_y) + feet;
            }
        }

        bn::fixed_point delta = target - hero.position();
        bn::fixed_point step(bn::clamp(delta.x(), bn::fixed(-speed), bn::fixed(speed)),
                             bn::clamp(delta.y(), bn::fixed(-speed), bn::fixed(speed)));
        hero.set_position(hero.position() + step);
        walk.update(hero, (step.x() > 0) - (step.x() < 0), (step.y() > 0) - (step.y() < 0), speed);

        // The bomb goes on the cell nearest the sprite: mid-step, cell_x and cell_y are already the next cell.
        if(bn::keypad::a_pressed() && ! bombs.full())
        {
            bn::fixed_point drawn = hero.position() - feet;
            int x = (drawn.x().integer() + 112 + cell_size / 2) / cell_size;
            int y = (drawn.y().integer() + 72 + cell_size / 2) / cell_size;
            bn::sprite_ptr sprite = bn::sprite_items::bomb.create_sprite(cell_position(x, y));
            sprite.set_z_order(1);   // Under the hero, who stands on it.
            bombs.push_back(bomb{ sprite, x, y, fuse_frames });
        }

        for(bomb& waiting : bombs)
        {
            if(--waiting.fuse <= 0)
            {
                explode(waiting, flames);
            }
        }

        bn::erase_if(bombs, [](const bomb& gone) { return gone.fuse <= 0; });

        // The flames are cells, so whatever is on one of those cells is hit. A bomb goes off on the next frame.
        for(bomb& waiting : bombs)
        {
            if(burning(flames, waiting.x, waiting.y))
            {
                waiting.fuse = 1;
            }
        }

        if(safe_frames > 0)
        {
            --safe_frames;
        }
        else if(burning(flames, cell_x, cell_y))
        {
            safe_frames = 60;
        }

        hero.set_visible(safe_frames / 4 % 2 == 0);

        for(flame& fire : flames)
        {
            --fire.life;
            fire.sprite.set_tiles(bn::sprite_items::flame.tiles_item(), (flame_frames - 1 - fire.life) / 6);
        }

        bn::erase_if(flames, [](const flame& out) { return out.life <= 0; });
        bn::core::update();
    }
}
