#include "bn_core.h"
#include "bn_fixed_rect.h"
#include "bn_format.h"
#include "bn_keypad.h"
#include "bn_random.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_world.h"
#include "bn_sprite_items_coin.h"
#include "bn_sprite_items_hero.h"

#include "hero_walk.h"

#include "font.h"

// An easing curve takes the fraction of time gone (0 to 1) and returns the fraction of the way done.
using easing = bn::fixed(*)(bn::fixed);

bn::fixed linear(bn::fixed t)
{
    return t;
}

// Fast at the start, slow at the end.
bn::fixed ease_out(bn::fixed t)
{
    bn::fixed left = 1 - t;
    return 1 - left.safe_multiplication(left);
}

// Slow at both ends: 3t² - 2t³.
bn::fixed smoothstep(bn::fixed t)
{
    return t.safe_multiplication(t).safe_multiplication(3 - 2 * t);
}

struct curve
{
    const char* name;
    easing function;
};

constexpr curve curves[] = {
    { "Linear", linear },
    { "Ease out", ease_out },
    { "Smoothstep", smoothstep }
};

// The point a fraction t of the way from one point to another. t = 0 gives from, t = 1 gives to.
bn::fixed_point lerp(const bn::fixed_point& from, const bn::fixed_point& to, bn::fixed t)
{
    bn::fixed_point way = to - from;
    return from + bn::fixed_point(way.x().safe_multiplication(t), way.y().safe_multiplication(t));
}

// A coin on its way to the counter: where it started, and how many frames ago.
struct flight
{
    bn::sprite_ptr sprite;
    bn::fixed_point from;
    easing function;
    int frame = 0;
};

constexpr int flight_frames = 40;
constexpr bn::fixed_point counter_icon(-104, -70);

int main()
{
    bn::core::init();

    bn::regular_bg_ptr world = bn::regular_bg_items::world.create_bg(0, 0);
    bn::sprite_text_generator text_generator(hud::font);
    bn::random random;

    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(0, 30);
    hero_walk walk;
    bn::sprite_ptr icon = bn::sprite_items::coin.create_sprite(counter_icon);

    bn::vector<bn::sprite_ptr, 3> coins;
    coins.push_back(bn::sprite_items::coin.create_sprite(-60, 32));
    coins.push_back(bn::sprite_items::coin.create_sprite(50, 32));
    coins.push_back(bn::sprite_items::coin.create_sprite(100, 32));

    bn::vector<flight, 8> flights;
    int curve_index = 0;
    int coins_taken = 0;

    bn::vector<bn::sprite_ptr, 4> counter_sprites;
    bn::vector<bn::sprite_ptr, 8> curve_sprites;
    bool counter_changed = true;
    bool curve_changed = true;

    while(true)
    {
        if(bn::keypad::a_pressed())
        {
            curve_index = (curve_index + 1) % 3;
            curve_changed = true;
        }

        int step_x = bn::keypad::right_held() - bn::keypad::left_held();
        int step_y = bn::keypad::down_held() - bn::keypad::up_held();
        hero.set_position(hero.position() + bn::fixed_point(step_x, step_y));

        walk.update(hero, step_x, step_y);

        bn::fixed_rect hero_box(hero.position() + bn::fixed_point(0, 4), bn::fixed_size(12, 14));

        for(bn::sprite_ptr& coin : coins)
        {
            bn::fixed_rect coin_box(coin.position() + bn::fixed_point(-1, -1), bn::fixed_size(10, 10));

            if(coin_box.intersects(hero_box) && ! flights.full())
            {
                // A new sprite flies to the counter, and the coin on the map moves somewhere else.
                bn::sprite_ptr flying = bn::sprite_items::coin.create_sprite(coin.position());
                flights.push_back(flight{ flying, coin.position(), curves[curve_index].function });
                coin.set_position(random.get_int(-100, 101), random.get_int(-40, 61));
            }
        }

        for(flight& coin : flights)
        {
            ++coin.frame;
            bn::fixed t = bn::fixed(coin.frame) / flight_frames;
            coin.sprite.set_position(lerp(coin.from, counter_icon, coin.function(t)));
        }

        // A coin counts when it gets there.
        int arrived = bn::erase_if(flights, [](const flight& coin)
        {
            return coin.frame == flight_frames;
        });

        if(arrived)
        {
            coins_taken += arrived;
            counter_changed = true;
        }

        if(counter_changed)
        {
            counter_changed = false;
            counter_sprites.clear();
            text_generator.set_left_alignment();
            text_generator.generate(-94, -70, bn::format<8>("{}", coins_taken), counter_sprites);
        }

        if(curve_changed)
        {
            curve_changed = false;
            curve_sprites.clear();
            text_generator.set_center_alignment();
            text_generator.generate(0, 70, bn::format<24>("A: {}", curves[curve_index].name), curve_sprites);
        }

        bn::core::update();
    }
}
