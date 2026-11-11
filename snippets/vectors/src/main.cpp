#include "bn_core.h"
#include "bn_format.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_animate_actions.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_world.h"
#include "bn_sprite_items_coin.h"
#include "bn_sprite_items_hero.h"
#include "bn_sprite_items_log.h"

#include "font.h"

// A coin in flight: where it is, and how far it moves each frame.
struct shot
{
    bn::sprite_ptr sprite;
    bn::fixed_point velocity;
};

// The direction from one point to another, as a vector of length 1.
bn::fixed_point direction(const bn::fixed_point& from, const bn::fixed_point& to)
{
    bn::fixed_point difference = to - from;

    // atan2 takes ints and answers from -180 to 180. The sine table wants 0 to 360.
    bn::fixed angle = bn::degrees_atan2(difference.y().round_integer(), difference.x().round_integer());

    if(angle < 0)
    {
        angle += 360;
    }

    auto [sin, cos] = bn::degrees_lut_sin_and_cos(angle);
    return bn::fixed_point(cos, sin);
}

// Compares squared lengths, so there's no square root to compute.
bool closer_than(const bn::fixed_point& a, const bn::fixed_point& b, int distance)
{
    bn::fixed_point difference = b - a;
    bn::fixed squared_length = difference.x() * difference.x() + difference.y() * difference.y();
    return squared_length < distance * distance;
}

int main()
{
    bn::core::init();

    bn::regular_bg_ptr world = bn::regular_bg_items::world.create_bg(0, 0);
    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(0, 40);
    bn::sprite_ptr log = bn::sprite_items::log.create_sprite(0, -24);
    bn::sprite_animate_action<4> log_animation = bn::create_sprite_animate_action_forever(
            log, 8, bn::sprite_items::log.tiles_item(), 0, 1, 2, 3);

    bn::sprite_text_generator text_generator(hud::font);
    text_generator.set_center_alignment();

    constexpr bn::fixed hero_speed = 1.5;
    constexpr bn::fixed shot_speed = 2;
    constexpr bn::fixed one_over_root_two = 0.7071; // Shrinks a (1, 1) diagonal to length 1.

    bn::vector<shot, 8> shots;
    int frames_to_next_shot = 60;
    int hits = 0;
    bool diagonal_fix = true;
    int shown_speed = 0;

    bn::vector<bn::sprite_ptr, 4> hits_sprites;
    bn::vector<bn::sprite_ptr, 12> speed_sprites;
    bool hits_changed = true;
    bool speed_changed = true;

    while(true)
    {
        if(bn::keypad::a_pressed())
        {
            diagonal_fix = ! diagonal_fix;
            speed_changed = true;
        }

        // One step: each axis is -1, 0 or 1, so a diagonal is longer than a straight line.
        int step_x = bn::keypad::right_held() - bn::keypad::left_held();
        int step_y = bn::keypad::down_held() - bn::keypad::up_held();
        bn::fixed_point step = bn::fixed_point(step_x, step_y) * hero_speed;

        if(diagonal_fix && step_x != 0 && step_y != 0)
        {
            step *= one_over_root_two;
        }

        hero.set_position(hero.position() + step);

        if(step_x != 0 || step_y != 0)
        {
            // The real length of the step, times 100 so it shows without decimals.
            bn::fixed squared_speed = step.x().safe_multiplication(step.x()) + step.y().safe_multiplication(step.y());
            int speed = bn::sqrt((squared_speed * 10000).round_integer());

            if(speed != shown_speed)
            {
                shown_speed = speed;
                speed_changed = true;
            }
        }

        log.set_horizontal_flip(hero.x() < log.x());
        log_animation.update();

        if(--frames_to_next_shot == 0)
        {
            frames_to_next_shot = 60;

            if(! shots.full())
            {
                bn::fixed_point velocity = direction(log.position(), hero.position()) * shot_speed;
                shots.push_back(shot{ bn::sprite_items::coin.create_sprite(log.position()), velocity });
            }
        }

        for(shot& flying : shots)
        {
            flying.sprite.set_position(flying.sprite.position() + flying.velocity);
        }

        // A shot goes away when it hits the hero or leaves the screen.
        bn::erase_if(shots, [&](const shot& flying)
        {
            if(closer_than(flying.sprite.position(), hero.position(), 10))
            {
                ++hits;
                hits_changed = true;
                return true;
            }

            return bn::abs(flying.sprite.x()) > 128 || bn::abs(flying.sprite.y()) > 88;
        });

        if(hits_changed)
        {
            hits_changed = false;
            hits_sprites.clear();
            text_generator.generate(0, -70, bn::format<16>("Hits: {}", hits), hits_sprites);
        }

        if(speed_changed)
        {
            speed_changed = false;
            speed_sprites.clear();
            text_generator.generate(0, 54, bn::format<24>("A: diagonal fix {}", diagonal_fix ? "on" : "off"),
                    speed_sprites);
            text_generator.generate(0, 70, bn::format<32>("Pixels per 100 frames: {}", shown_speed), speed_sprites);
        }

        bn::core::update();
    }
}
