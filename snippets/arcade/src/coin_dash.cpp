#include "games.h"

#include "bn_algorithm.h"
#include "bn_core.h"
#include "bn_format.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_world.h"
#include "bn_sprite_items_coin.h"
#include "bn_sprite_items_hero.h"

#include "hero_walk.h"

// Take the five coins before ten seconds run out. The score is the coins taken.
int coin_dash(bn::sprite_text_generator& text_generator)
{
    bn::regular_bg_ptr world = bn::regular_bg_items::world.create_bg(0, 0);
    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(0, 8);
    hero_walk walk;

    constexpr int coins_count = 5;
    constexpr bn::fixed_point spots[coins_count] = { {-88, -40}, {88, -40}, {-72, 48}, {72, 48}, {0, -48} };
    bn::vector<bn::sprite_ptr, coins_count> coins;

    for(bn::fixed_point spot : spots)
    {
        coins.push_back(bn::sprite_items::coin.create_sprite(spot));
    }

    bn::vector<bn::sprite_ptr, 8> text;
    int frames_left = 10 * 60;
    int shown_seconds = -1;
    int shown_coins = -1;

    while(frames_left > 0 && ! coins.empty())
    {
        int step_x = bn::keypad::right_held() - bn::keypad::left_held();
        int step_y = bn::keypad::down_held() - bn::keypad::up_held();
        hero.set_position(bn::clamp(hero.x() + step_x * 2, bn::fixed(-108), bn::fixed(108)),
                          bn::clamp(hero.y() + step_y * 2, bn::fixed(-56), bn::fixed(64)));
        walk.update(hero, step_x, step_y, 2);
        --frames_left;

        bn::fixed_point feet = hero.position();
        bn::erase_if(coins, [feet](const bn::sprite_ptr& coin)
        {
            bn::fixed_point gap = coin.position() - feet;
            return bn::abs(gap.x()) < 12 && bn::abs(gap.y()) < 14;
        });

        int seconds = (frames_left + 59) / 60;
        int taken = coins_count - coins.size();

        if(seconds != shown_seconds || taken != shown_coins)
        {
            shown_seconds = seconds;
            shown_coins = taken;
            text.clear();
            text_generator.generate(0, -70, bn::format<24>("Coins {}  Time {}", taken, seconds), text);
        }

        bn::core::update();
    }

    for(int frame = 0; frame < 60; ++frame)   // A second to see how it ended.
    {
        bn::core::update();
    }

    return coins_count - coins.size();
}
