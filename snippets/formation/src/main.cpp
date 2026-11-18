#include "bn_core.h"
#include "bn_fixed_rect.h"
#include "bn_format.h"
#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_sky.h"
#include "bn_sprite_items_enemies.h"
#include "bn_sprite_items_ship.h"
#include "bn_sprite_items_shot.h"

#include "font.h"

constexpr int columns = 7;
constexpr int rows = 4;
constexpr int spacing_x = 26;
constexpr int spacing_y = 22;
constexpr int half_width = 12;   // Half of the widest enemy, in pixels.
constexpr int step_x = 2;
constexpr int step_down = 8;
constexpr int row_kinds[rows] = { 2, 1, 0, 0 };   // Alien, bat, bug, bug: frames 4-5, 2-3 and 0-1.

// An enemy only remembers its cell. Where it is on screen comes from the formation.
struct enemy
{
    bn::sprite_ptr sprite;
    int column;
    int row;
};

// The fewer enemies left, the fewer frames between two steps: 15 frames with all 28, every frame with 1.
int frames_per_step(int alive)
{
    return 1 + alive / 2;
}

void fill(bn::vector<enemy, columns * rows>& enemies)
{
    enemies.clear();

    for(int row = 0; row < rows; ++row)
    {
        for(int column = 0; column < columns; ++column)
        {
            bn::sprite_ptr sprite = bn::sprite_items::enemies.create_sprite(0, 0, row_kinds[row] * 2);
            enemies.push_back(enemy{ sprite, column, row });
        }
    }
}

int main()
{
    bn::core::init();
    bn::regular_bg_ptr sky = bn::regular_bg_items::sky.create_bg(0, 0);

    bn::vector<enemy, columns * rows> enemies;
    fill(enemies);

    // The formation is one point: the center of the top-left cell. Everything else is an offset.
    bn::fixed_point origin(-(columns - 1) * spacing_x / 2, -52);
    int direction = 1;
    int wait = 0;
    int phase = 0;
    bool landed = false;

    bn::sprite_ptr ship = bn::sprite_items::ship.create_sprite(0, 64);
    bn::sprite_ptr shot = bn::sprite_items::shot.create_sprite(0, 0);
    shot.set_visible(false);

    bn::sprite_text_generator text_generator(hud::font);
    text_generator.set_center_alignment();
    bn::vector<bn::sprite_ptr, 16> text_sprites;
    int shown_alive = -1;

    while(true)
    {
        // The ship and its one shot.
        bn::fixed ship_x = ship.x() + (bn::keypad::right_held() - bn::keypad::left_held()) * 2;
        ship.set_x(bn::clamp(ship_x, bn::fixed(-108), bn::fixed(108)));

        if(bn::keypad::a_pressed() && ! shot.visible())
        {
            shot.set_position(ship.x(), ship.y() - 12);
            shot.set_visible(true);
        }

        if(shot.visible())
        {
            shot.set_y(shot.y() - 4);
            bn::fixed_rect shot_box(shot.position(), bn::fixed_size(2, 12));

            for(auto it = enemies.begin(); it != enemies.end(); ++it)
            {
                if(shot_box.intersects(bn::fixed_rect(it->sprite.position(), bn::fixed_size(20, 16))))
                {
                    enemies.erase(it);
                    shot.set_visible(false);
                    break;
                }
            }

            if(shot.y() < -88)
            {
                shot.set_visible(false);
            }
        }

        // A new wave when they're all gone, or when they reach the ship.
        if(enemies.empty() || landed)
        {
            fill(enemies);
            origin = bn::fixed_point(-(columns - 1) * spacing_x / 2, -52);
            direction = 1;
            wait = 0;
            landed = false;
        }

        // The formation takes one step every few frames.
        if(wait > 0)
        {
            --wait;
        }
        else
        {
            wait = frames_per_step(enemies.size()) - 1;

            // The edges are the outermost living enemies, not the grid's.
            int first_column = columns;
            int last_column = -1;
            int last_row = -1;

            for(const enemy& alive : enemies)
            {
                first_column = bn::min(first_column, alive.column);
                last_column = bn::max(last_column, alive.column);
                last_row = bn::max(last_row, alive.row);
            }

            // Look before stepping: if the step would cross the screen's edge, go down and turn instead.
            bn::fixed next_x = origin.x() + direction * step_x;
            bn::fixed left = next_x + first_column * spacing_x - half_width;
            bn::fixed right = next_x + last_column * spacing_x + half_width;

            if(left < -120 || right > 120)
            {
                origin.set_y(origin.y() + step_down);
                direction = -direction;
                landed = origin.y() + last_row * spacing_y > 44;
            }
            else
            {
                origin.set_x(next_x);
            }

            phase = 1 - phase;

            for(enemy& alive : enemies)
            {
                alive.sprite.set_position(origin + bn::fixed_point(alive.column * spacing_x, alive.row * spacing_y));
                alive.sprite.set_tiles(bn::sprite_items::enemies.tiles_item(), row_kinds[alive.row] * 2 + phase);
            }
        }

        if(enemies.size() != shown_alive)
        {
            shown_alive = enemies.size();
            text_sprites.clear();
            text_generator.generate(0, -70, bn::format<32>("Alive: {}  Frames per step: {}",
                                                           shown_alive, frames_per_step(shown_alive)), text_sprites);
        }

        bn::core::update();
    }
}
