#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_animate_actions.h"

#include "bn_regular_bg_items_room.h"
#include "bn_sprite_items_coin.h"

#include "room.h"

// Is this screen position inside a crate? Outside the room counts as solid.
bool is_solid(bn::fixed x, bn::fixed y)
{
    // (0, 0) is the center of the screen, but the cells start at its top-left corner.
    // floor_integer, not integer: -0.5 must be pixel -1, outside the room, not pixel 0.
    int pixel_x = (x + 120).floor_integer();
    int pixel_y = (y + 80).floor_integer();

    // Checked in pixels, before dividing: in C++, -1 / 16 is 0, the first cell.
    if(pixel_x < 0 || pixel_y < 0 || pixel_x >= room::width || pixel_y >= room::height)
    {
        return true;
    }

    return room::cells[pixel_y / room::cell_size][pixel_x / room::cell_size] == '#';
}

// The coin's box is 10x10, smaller than a cell, so its four corners are enough:
// no crate fits between them.
bool hits_crate(bn::fixed_point center)
{
    constexpr bn::fixed half = 5;
    return is_solid(center.x() - half, center.y() - half) || is_solid(center.x() + half - 1, center.y() - half) ||
            is_solid(center.x() - half, center.y() + half - 1) || is_solid(center.x() + half - 1, center.y() + half - 1);
}

int main()
{
    bn::core::init();

    bn::regular_bg_ptr room_bg = bn::regular_bg_items::room.create_bg(0, 0);
    bn::sprite_ptr coin = bn::sprite_items::coin.create_sprite(-88, -48);
    bn::sprite_animate_action<4> spin = bn::create_sprite_animate_action_forever(
                coin, 6, bn::sprite_items::coin.tiles_item(), 0, 1, 2, 3);

    constexpr bn::fixed speed = 1.25;
    bn::point direction(1, 1);

    while(true)
    {
        // Kick: the D-pad sends the coin toward that side.
        int kick_x = bn::keypad::right_pressed() - bn::keypad::left_pressed();
        int kick_y = bn::keypad::down_pressed() - bn::keypad::up_pressed();

        if(kick_x)
        {
            direction.set_x(kick_x);
        }

        if(kick_y)
        {
            direction.set_y(kick_y);
        }

        // One axis at a time: compute the next position, check it, and only then move.
        // A crate on one axis bounces that axis and leaves the other one alone.
        bn::fixed_point next(coin.x() + direction.x() * speed, coin.y());

        if(hits_crate(next))
        {
            direction.set_x(-direction.x());
        }
        else
        {
            coin.set_position(next);
        }

        next = bn::fixed_point(coin.x(), coin.y() + direction.y() * speed);

        if(hits_crate(next))
        {
            direction.set_y(-direction.y());
        }
        else
        {
            coin.set_position(next);
        }

        spin.update();
        bn::core::update();
    }
}
