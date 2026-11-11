#include "bn_core.h"
#include "bn_format.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_timer.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_world.h"
#include "bn_sprite_items_coin.h"
#include "bn_sprite_items_hero.h"

#include "font.h"

// volatile: read from memory every time, so the compiler can't do the loops below at compile time.
volatile float float_factor = 1.0001f;
volatile int fixed_factor_data = 4096 + 1;
volatile int sink;

constexpr int repetitions = 1000;

int time_float_multiplications()
{
    bn::timer timer;
    float value = 1;

    for(int i = 0; i < repetitions; ++i)
    {
        value *= float_factor;
    }

    sink = int(value);
    return timer.elapsed_ticks();
}

int time_fixed_multiplications()
{
    bn::timer timer;
    bn::fixed factor = bn::fixed::from_data(fixed_factor_data);
    bn::fixed value = 1;

    for(int i = 0; i < repetitions; ++i)
    {
        value *= factor;
    }

    sink = value.data();
    return timer.elapsed_ticks();
}

int time_fixed_divisions()
{
    bn::timer timer;
    bn::fixed factor = bn::fixed::from_data(fixed_factor_data);
    bn::fixed value = 1000;

    for(int i = 0; i < repetitions; ++i)
    {
        value /= factor;
    }

    sink = value.data();
    return timer.elapsed_ticks();
}

int main()
{
    bn::core::init();

    bn::regular_bg_ptr world = bn::regular_bg_items::world.create_bg(0, 0);
    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(0, 0);

    bn::sprite_text_generator text_generator(hud::font);
    text_generator.set_center_alignment();

    // Timer ticks for 1000 of each. A frame has 4389.
    bn::vector<bn::sprite_ptr, 16> timing_sprites;
    text_generator.generate(0, -70, bn::format<40>("Multiply: float {}  fixed {}",
            time_float_multiplications(), time_fixed_multiplications()), timing_sprites);
    text_generator.generate(0, -54, bn::format<40>("Divide: fixed {}", time_fixed_divisions()), timing_sprites);

    constexpr int coins_count = 8;
    bn::vector<bn::sprite_ptr, coins_count> coins;

    for(int index = 0; index < coins_count; ++index)
    {
        coins.push_back(bn::sprite_items::coin.create_sprite(0, 0));
    }

    // Degrees per frame: a quarter of a degree is a fine speed, because bn::fixed keeps the fraction.
    bn::fixed speed = 1;
    bn::fixed angle = 0;
    constexpr int radius = 48;

    bn::vector<bn::sprite_ptr, 8> speed_sprites;
    bool speed_changed = true;

    while(true)
    {
        bn::fixed wanted_speed = speed + bn::fixed(0.25) * (bn::keypad::up_pressed() - bn::keypad::down_pressed());
        bn::fixed new_speed = bn::clamp(wanted_speed, bn::fixed(0), bn::fixed(4));

        if(new_speed != speed)
        {
            speed = new_speed;
            speed_changed = true;
        }

        // degrees_lut_sin_and_cos only takes 0 to 360, so the angle wraps by hand.
        angle += speed;

        if(angle >= 360)
        {
            angle -= 360;
        }

        for(int index = 0; index < coins_count; ++index)
        {
            bn::fixed coin_angle = angle + index * (360 / coins_count);

            if(coin_angle >= 360)
            {
                coin_angle -= 360;
            }

            bn::pair<bn::fixed, bn::fixed> sin_and_cos = bn::degrees_lut_sin_and_cos(coin_angle);
            bn::fixed x = sin_and_cos.second * radius;
            bn::fixed y = sin_and_cos.first * radius;
            coins[index].set_position(hero.position() + bn::fixed_point(x, y));
        }

        if(speed_changed)
        {
            speed_changed = false;
            speed_sprites.clear();
            text_generator.generate(0, 70, bn::format<32>("Up, Down: {} degrees", speed), speed_sprites);
        }

        bn::core::update();
    }
}
