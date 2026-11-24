#include "bn_algorithm.h"
#include "bn_core.h"
#include "bn_fixed_point.h"
#include "bn_format.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_sky.h"
#include "bn_sprite_items_boss.h"
#include "bn_sprite_items_bullet.h"
#include "bn_sprite_items_ship.h"

#include "font.h"

// A pattern is a few numbers: the code that fires them is the same for all.
// Angles are in degrees: 0 is right and 90 is down, because the screen's y grows downward.
struct pattern
{
    const char* name;
    int every;          // Frames between volleys.
    int shots;          // Bullets per volley.
    bn::fixed spread;   // Degrees between the bullets of one volley.
    bn::fixed turn;     // Degrees the whole volley turns after each one.
    bn::fixed speed;    // Pixels per frame.
    bool aimed;         // Centered on the player instead of on the pattern's own angle.
};

constexpr pattern patterns[] = {
    { "Ring", 40, 16, 22.5, 0, 1, false },        // 16 x 22.5 = 360: the whole circle.
    { "Spiral", 4, 2, 180, 13, 1.25, false },     // Two arms, turning 13 degrees per volley.
    { "Flower", 24, 8, 45, 9, 1, false },         // A ring that turns.
    { "Aimed fan", 36, 5, 12, 0, 1.5, true },     // Five bullets, the middle one at the player.
};

constexpr int pattern_count = sizeof(patterns) / sizeof(patterns[0]);
constexpr int max_bullets = 96;   // The sprite budget left after the planes and the text.

struct bullet
{
    bn::sprite_ptr sprite;
    bn::fixed_point velocity;
};

// Keeps an angle in [0, 360), the range Butano's sine and cosine tables accept.
bn::fixed wrap(bn::fixed degrees)
{
    while(degrees >= 360)
    {
        degrees -= 360;
    }

    while(degrees < 0)
    {
        degrees += 360;
    }

    return degrees;
}

int main()
{
    bn::core::init();

    bn::regular_bg_ptr sky = bn::regular_bg_items::sky.create_bg(0, 0);
    bn::sprite_ptr boss = bn::sprite_items::boss.create_sprite(0, -44);
    bn::sprite_ptr ship = bn::sprite_items::ship.create_sprite(0, 56);
    bn::vector<bullet, max_bullets> bullets;

    bn::sprite_text_generator text_generator(hud::font);
    bn::vector<bn::sprite_ptr, 16> text;
    int shown_count = -1;

    int current = 0;
    int timer = 0;
    bn::fixed angle = 90;   // The pattern's own angle, turned after each volley.

    while(true)
    {
        int step_x = bn::keypad::right_held() - bn::keypad::left_held();
        int step_y = bn::keypad::down_held() - bn::keypad::up_held();
        ship.set_position(bn::clamp(ship.x() + step_x * 2, bn::fixed(-108), bn::fixed(108)),
                          bn::clamp(ship.y() + step_y * 2, bn::fixed(-20), bn::fixed(64)));

        if(bn::keypad::a_pressed())
        {
            current = (current + 1) % pattern_count;
            bullets.clear();
            timer = 0;
            angle = 90;
            shown_count = -1;
        }

        const pattern& now = patterns[current];

        if(--timer <= 0)
        {
            timer = now.every;
            bn::fixed first = angle;

            if(now.aimed)
            {
                bn::fixed_point to_ship = ship.position() - boss.position();
                bn::fixed at_ship = bn::degrees_atan2(to_ship.y().round_integer(), to_ship.x().round_integer());
                first = at_ship - now.spread * (now.shots - 1) / 2;   // So the middle bullet is the aimed one.
            }

            for(int index = 0; index < now.shots && ! bullets.full(); ++index)
            {
                bn::fixed degrees = wrap(first + now.spread * index);
                bn::fixed_point velocity(bn::degrees_lut_cos(degrees) * now.speed,
                                         bn::degrees_lut_sin(degrees) * now.speed);
                bullets.push_back({ bn::sprite_items::bullet.create_sprite(boss.position()), velocity });
            }

            angle = wrap(angle + now.turn);
        }

        for(bullet& flying : bullets)
        {
            flying.sprite.set_position(flying.sprite.position() + flying.velocity);
        }

        bn::erase_if(bullets, [](const bullet& flying)
        {
            return bn::abs(flying.sprite.x()) > 124 || bn::abs(flying.sprite.y()) > 84;
        });

        if(bullets.size() != shown_count)
        {
            shown_count = bullets.size();
            text.clear();
            text_generator.generate(-112, -72, now.name, text);
            text_generator.generate(40, -72, bn::format<16>("Bullets {}", shown_count), text);
        }

        bn::core::update();
    }
}
