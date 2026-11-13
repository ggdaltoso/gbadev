#include "bn_core.h"
#include "bn_fixed_rect.h"
#include "bn_format.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_world.h"
#include "bn_sprite_items_hero.h"

#include "font.h"

// How the ground feels, in pixels per frame: top speed, how fast you get there, and how fast you stop.
struct surface
{
    const char* name;
    bn::fixed max_speed;
    bn::fixed acceleration;
    bn::fixed friction;
};

constexpr surface grass = { "Grass", 1.5, 0.125, 0.0625 };
constexpr surface sand = { "Sand", 0.75, 0.0625, 0.25 };

// The sand on the map, from its left edge to past the right side of the screen.
constexpr bn::fixed_rect sand_area(bn::fixed_point(80, 0), bn::fixed_size(120, 224));

// Moves value toward target by at most step, and never past it. Works for any type with < + - .
template<typename Number>
Number approach(Number value, Number target, Number step)
{
    if(value < target)
    {
        return bn::min(value + step, target);
    }

    return bn::max(value - step, target);
}

bn::fixed squared_length(const bn::fixed_point& vector)
{
    return vector.x().safe_multiplication(vector.x()) + vector.y().safe_multiplication(vector.y());
}

// A speed times 100, so it shows without decimals. Only for small vectors: times 10000 overflows past 7.
int speed_times_100(const bn::fixed_point& velocity)
{
    return bn::sqrt((squared_length(velocity) * 10000).round_integer());
}

// A distance in whole pixels.
int distance_in_pixels(const bn::fixed_point& from, const bn::fixed_point& to)
{
    return bn::sqrt(squared_length(to - from).round_integer());
}

int main()
{
    bn::core::init();

    bn::regular_bg_ptr world = bn::regular_bg_items::world.create_bg(0, 0);
    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(-60, 0);

    bn::sprite_text_generator text_generator(hud::font);
    text_generator.set_center_alignment();

    constexpr bn::fixed one_over_root_two = 0.7071;

    bn::fixed_point velocity;
    bool inertia = true;

    // Measuring the stop: where the hero was when you let go, and the frames since.
    bool braking = false;
    bn::fixed_point brake_start;
    int brake_frames = 0;

    const surface* shown_surface = nullptr;
    int shown_speed = -1;
    bn::vector<bn::sprite_ptr, 8> speed_sprites;
    bn::vector<bn::sprite_ptr, 12> stop_sprites;
    bn::vector<bn::sprite_ptr, 8> mode_sprites;
    bool mode_changed = true;

    while(true)
    {
        if(bn::keypad::a_pressed())
        {
            inertia = ! inertia;
            mode_changed = true;
        }

        int input_x = bn::keypad::right_held() - bn::keypad::left_held();
        int input_y = bn::keypad::down_held() - bn::keypad::up_held();
        bool pushing = input_x != 0 || input_y != 0;
        bn::fixed_point direction(input_x, input_y);

        if(input_x != 0 && input_y != 0)
        {
            direction *= one_over_root_two;
        }

        const surface& ground = sand_area.contains(hero.position()) ? sand : grass;
        bn::fixed_point target_velocity = direction * ground.max_speed;

        if(inertia)
        {
            // Pushing speeds you up toward the target. Letting go, friction brings you back to 0.
            bn::fixed step = pushing ? ground.acceleration : ground.friction;
            velocity = bn::fixed_point(approach(velocity.x(), target_velocity.x(), step),
                                       approach(velocity.y(), target_velocity.y(), step));
        }
        else
        {
            velocity = target_velocity;
        }

        if(pushing)
        {
            braking = false;
        }
        else if(! braking && velocity != bn::fixed_point())
        {
            braking = true;
            brake_start = hero.position();
            brake_frames = 0;
        }

        if(braking)
        {
            if(velocity == bn::fixed_point())
            {
                braking = false;
                int distance = distance_in_pixels(brake_start, hero.position());
                stop_sprites.clear();
                text_generator.generate(0, 54, bn::format<40>("Stopped in {} px, {} frames",
                        distance, brake_frames), stop_sprites);
            }
            else
            {
                ++brake_frames;
            }
        }

        hero.set_position(hero.position() + velocity);

        if(velocity.x() != 0)
        {
            hero.set_horizontal_flip(velocity.x() < 0);
        }

        int speed = speed_times_100(velocity);

        if(&ground != shown_surface || speed != shown_speed)
        {
            shown_surface = &ground;
            shown_speed = speed;
            speed_sprites.clear();
            text_generator.generate(0, -70, bn::format<40>("{}  Pixels per 100 frames: {}",
                    ground.name, speed), speed_sprites);
        }

        if(mode_changed)
        {
            mode_changed = false;
            mode_sprites.clear();
            text_generator.generate(0, 70, bn::format<24>("A: inertia {}", inertia ? "on" : "off"), mode_sprites);
        }

        bn::core::update();
    }
}
