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
#include "bn_sprite_items_boom.h"
#include "bn_sprite_items_boss.h"
#include "bn_sprite_items_missile.h"
#include "bn_sprite_items_ship.h"

#include "font.h"

constexpr bn::fixed missile_speed = 1.5;
constexpr bn::fixed max_turn = 1.5;      // Degrees per frame: how hard a missile can turn.
constexpr int missile_life = 180;      // Frames of fuel, so a missile that missed doesn't circle forever.
constexpr int launch_every = 50;

// Angles in degrees: 0 is right, 90 is down, because the screen's y grows downward.
struct missile
{
    bn::sprite_ptr sprite;
    bn::fixed heading;
    int life = missile_life;
};

struct boom
{
    bn::sprite_ptr sprite;
    int frames = 12;
};

// Keeps an angle in [0, 360).
bn::fixed wrap(bn::fixed degrees)
{
    while(degrees >= 360) { degrees -= 360; }
    while(degrees < 0) { degrees += 360; }
    return degrees;
}

// The shortest turn from one heading to another, in [-180, 180): negative turns one way, positive the other.
bn::fixed turn_between(bn::fixed from, bn::fixed to)
{
    bn::fixed difference = to - from;
    while(difference >= 180) { difference -= 360; }
    while(difference < -180) { difference += 360; }
    return difference;
}

int main()
{
    bn::core::init();

    bn::regular_bg_ptr sky = bn::regular_bg_items::sky.create_bg(0, 0);
    bn::sprite_ptr boss = bn::sprite_items::boss.create_sprite(0, -44);
    bn::sprite_ptr ship = bn::sprite_items::ship.create_sprite(0, 50);
    bn::vector<missile, 8> missiles;
    bn::vector<boom, 8> booms;

    bn::sprite_text_generator text_generator(hud::font);
    bn::vector<bn::sprite_ptr, 8> text;
    int hits = 0;
    int shown_hits = -1;
    int timer = 0;

    while(true)
    {
        int step_x = bn::keypad::right_held() - bn::keypad::left_held();
        int step_y = bn::keypad::down_held() - bn::keypad::up_held();
        ship.set_position(bn::clamp(ship.x() + step_x * 2, bn::fixed(-108), bn::fixed(108)),
                          bn::clamp(ship.y() + step_y * 2, bn::fixed(-20), bn::fixed(64)));

        if(--timer <= 0 && ! missiles.full())
        {
            timer = launch_every;
            bn::sprite_ptr sprite = bn::sprite_items::missile.create_sprite(boss.position());
            missiles.push_back({ sprite, 90 });   // Launched straight down; it turns from there.
        }

        for(missile& flying : missiles)
        {
            // Where the ship is, as an angle, and the shortest way to it, limited to max_turn.
            bn::fixed_point to_ship = ship.position() - flying.sprite.position();
            bn::fixed target = bn::degrees_atan2(to_ship.y().round_integer(), to_ship.x().round_integer());
            bn::fixed turn = bn::clamp(turn_between(flying.heading, target), -max_turn, max_turn);
            flying.heading = wrap(flying.heading + turn);

            bn::fixed_point velocity(bn::degrees_lut_cos(flying.heading), bn::degrees_lut_sin(flying.heading));
            flying.sprite.set_position(flying.sprite.position() + velocity * missile_speed);

            // The art points up, heading 270. Butano rotates counterclockwise, our headings go clockwise.
            flying.sprite.set_rotation_angle(wrap(270 - flying.heading));
            --flying.life;

            if(bn::abs(to_ship.x()) < 8 && bn::abs(to_ship.y()) < 8)
            {
                ++hits;
                flying.life = 0;
            }

            if(flying.life == 0 && ! booms.full())
            {
                booms.push_back({ bn::sprite_items::boom.create_sprite(flying.sprite.position()) });
            }
        }

        bn::erase_if(missiles, [](const missile& flying) { return flying.life <= 0; });

        for(boom& burst : booms)
        {
            --burst.frames;
        }

        bn::erase_if(booms, [](const boom& burst) { return burst.frames <= 0; });

        if(hits != shown_hits)
        {
            shown_hits = hits;
            text.clear();
            text_generator.generate(-112, -72, bn::format<16>("Hits {}", hits), text);
        }

        bn::core::update();
    }
}
