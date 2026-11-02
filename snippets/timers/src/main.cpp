#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"
#include "bn_seed_random.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_animate_actions.h"
#include "bn_sprite_text_generator.h"
#include "bn_format.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_room.h"
#include "bn_sprite_items_coin.h"

#include "font.h"
#include "room.h"

struct coin
{
    bn::sprite_ptr sprite;
    bn::sprite_animate_action<4> spin;
    int frames_left;
};

int main()
{
    bn::core::init();

    bn::regular_bg_ptr room_bg = bn::regular_bg_items::room.create_bg(0, 0);
    bn::sprite_text_generator text_generator(hud::font);
    text_generator.set_center_alignment();

    // Title: count frames until the player presses START. Nobody presses it on the
    // same frame twice, so the count makes a different seed every time.
    bn::vector<bn::sprite_ptr, 8> text_sprites;
    text_generator.generate(0, 0, "Press START", text_sprites);
    unsigned frames = 0;

    while(! bn::keypad::start_pressed())
    {
        ++frames;
        bn::core::update();
    }

    // bn::seed_random adds 1 to its seed for each number, so seeds 137 and 138 would give the
    // same numbers shifted by one. Multiplying by a big odd number spreads close counts far apart.
    const unsigned seed = frames * 2654435761u;
    bn::seed_random random(seed);
    text_sprites.clear();
    text_generator.generate(0, -72, bn::format<32>("Seed {}   B: replay", seed), text_sprites);

    bn::vector<coin, 6> coins;
    int frames_to_next_coin = 0;

    while(true)
    {
        // The same seed gives the same numbers, in the same order: the same rain again.
        if(bn::keypad::b_pressed())
        {
            random.set_seed(seed);
            coins.clear();
            frames_to_next_coin = 0;
        }

        // Timers are frame counters: count down every frame, act at zero.
        if(frames_to_next_coin > 0)
        {
            --frames_to_next_coin;
        }
        else if(! coins.full())
        {
            // A random free cell: try again while it's a crate or already has a coin.
            int x, y;
            bool taken;

            do
            {
                int column = random.get_int(room::columns);
                int row = random.get_int(room::rows);
                x = column * room::cell_size + room::cell_size / 2 - 120;
                y = row * room::cell_size + room::cell_size / 2 - 80;
                taken = room::cells[row][column] == '#';

                for(const coin& other : coins)
                {
                    taken = taken || (other.sprite.x() == x && other.sprite.y() == y);
                }
            }
            while(taken);

            bn::sprite_ptr sprite = bn::sprite_items::coin.create_sprite(x, y);
            coins.push_back({ sprite, bn::create_sprite_animate_action_forever(
                                  sprite, 6, bn::sprite_items::coin.tiles_item(), 0, 1, 2, 3), 120 });

            // From 20 to 59 frames: between a third of a second and one second.
            frames_to_next_coin = random.get_int(20, 60);
        }

        for(coin& c : coins)
        {
            --c.frames_left;

            // The last half second, blink: 4 frames on, 4 off.
            c.sprite.set_visible(c.frames_left > 30 || c.frames_left % 8 >= 4);
            c.spin.update();
        }

        bn::erase_if(coins, [](const coin& c) { return c.frames_left == 0; });
        bn::core::update();
    }
}
