#include "bn_algorithm.h"
#include "bn_backdrop.h"
#include "bn_color.h"
#include "bn_core.h"
#include "bn_format.h"
#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_stage.h"
#include "bn_sprite_items_hero.h"
#include "bn_sprite_items_icons.h"

#include "font.h"
#include "hero_walk.h"

// How long each press is remembered, in frames: the whole feel of the controls is in these numbers.
constexpr int jump_buffer_frames = 8;   // A pressed this soon before landing still jumps.
constexpr int double_tap_frames = 15;   // A second tap this soon after the first one runs.
constexpr int repeat_delay = 20;        // Held this long, the cursor starts moving by itself...
constexpr int repeat_every = 4;         // ...one step every this many frames.

constexpr bn::fixed gravity = 0.25;
constexpr bn::fixed jump_speed = 4.5;
constexpr bn::fixed ground_y = 28;      // The hero's y when standing on the grass.
constexpr int items = 30;

int main()
{
    bn::core::init();
    bn::backdrop::set_color(bn::color(17, 25, 31));

    bn::regular_bg_ptr stage = bn::regular_bg_items::stage.create_bg(0, 0);
    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(-60, ground_y);
    hero_walk walk;
    bn::sprite_ptr marker = bn::sprite_items::icons.create_sprite(-87, -64, 2);
    bn::sprite_text_generator text_generator(hud::font);
    bn::vector<bn::sprite_ptr, 16> text;

    bn::fixed velocity_y = 0;
    int jump_buffer = 0;     // Frames left in which a jump will still happen.
    int last_tap = 0;        // -1 left, 1 right: the direction last tapped, and when.
    int last_tap_frame = -1000;
    bool running = false;
    int cursor = 0;
    int held_frames = 0;
    int frame = 0;
    bn::string<48> shown;

    while(true)
    {
        // Double tap: the same direction pressed twice, close enough together.
        int tap = bn::keypad::right_pressed() - bn::keypad::left_pressed();

        if(tap != 0)
        {
            running = tap == last_tap && frame - last_tap_frame <= double_tap_frames;
            last_tap = tap;
            last_tap_frame = frame;
        }

        int step_x = bn::keypad::right_held() - bn::keypad::left_held();
        running = running && step_x != 0;   // Letting go stops the run.
        int speed = running ? 2 : 1;
        hero.set_x(bn::clamp(hero.x() + step_x * speed, bn::fixed(-108), bn::fixed(108)));
        walk.update(hero, step_x, 0, speed);

        // Jump buffer: a press in the air is kept for a few frames, and spent on the first frame on the ground.
        if(bn::keypad::a_pressed())
        {
            jump_buffer = jump_buffer_frames;
        }

        if(hero.y() >= ground_y && jump_buffer > 0)
        {
            velocity_y = -jump_speed;
            jump_buffer = 0;
        }
        else if(jump_buffer > 0)
        {
            --jump_buffer;
        }

        velocity_y += gravity;
        hero.set_y(bn::min(hero.y() + velocity_y, ground_y));

        if(hero.y() == ground_y)
        {
            velocity_y = 0;
        }

        // Cursor repeat: one step on the press, then nothing for a while, then a step every few frames.
        int cursor_step = bn::keypad::r_held() - bn::keypad::l_held();

        if(bn::keypad::r_pressed() || bn::keypad::l_pressed())
        {
            cursor += bn::keypad::r_pressed() - bn::keypad::l_pressed();
            held_frames = 0;
        }
        else if(cursor_step != 0)
        {
            ++held_frames;

            if(held_frames >= repeat_delay && (held_frames - repeat_delay) % repeat_every == 0)
            {
                cursor += cursor_step;
            }
        }

        cursor = bn::clamp(cursor, 0, items - 1);
        marker.set_x(cursor * 6 - 87);

        bn::string<48> wanted = bn::format<48>("Item {}   Buffer {}   {}", cursor, jump_buffer,
                                               running ? "Run" : "Walk");

        if(wanted != shown)
        {
            shown = wanted;
            text.clear();
            text_generator.generate(-104, -36, shown, text);
        }

        ++frame;
        bn::core::update();
    }
}
