#include "bn_algorithm.h"
#include "bn_backdrop.h"
#include "bn_color.h"
#include "bn_colors.h"
#include "bn_core.h"
#include "bn_format.h"
#include "bn_keypad.h"
#include "bn_span.h"
#include "bn_sprite_palette_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "bn_sprite_items_hero.h"
#include "bn_sprite_items_icons.h"
#include "bn_sprite_items_shot.h"

#include "font.h"

using key = bn::keypad::key_type;

constexpr unsigned up = unsigned(key::UP), down = unsigned(key::DOWN), left = unsigned(key::LEFT);
constexpr unsigned right = unsigned(key::RIGHT), a = unsigned(key::A), b = unsigned(key::B);
constexpr unsigned l = unsigned(key::L), r = unsigned(key::R);
constexpr unsigned start = unsigned(key::START), select = unsigned(key::SELECT);
constexpr unsigned all_keys[] = { up, down, left, right, a, b, l, r, start, select };
constexpr unsigned dpad = up | down | left | right;

// Moves: the events the history has to end with, and the frames they all have to fit in.
constexpr unsigned fire[] = { down, down | right, right, a };
constexpr int fire_window = 20;
constexpr unsigned gold[] = { l, r, l, r, up, down, a };
constexpr int gold_window = 180;
constexpr unsigned reset_chord = l | r | start | select;   // Held together, in any order.

// Something that happened: the D-pad turned to a new direction, or a button was pressed. And when.
struct event
{
    unsigned keys;
    int frame;
};

bn::vector<event, 16> history;
int events_count = 0;   // Ever remembered, so the screen knows when there is a new one.

void remember(unsigned keys, int frame)
{
    if(history.full())
    {
        history.erase(history.begin());   // The oldest one goes.
    }

    history.push_back({ keys, frame });
    ++events_count;
}

// Only the end of the history counts: whatever was pressed before the move doesn't get in its way.
bool ends_with(bn::span<const unsigned> move, int window, int frame)
{
    int count = move.size();

    if(history.size() < count || frame - history[history.size() - count].frame > window)
    {
        return false;
    }

    for(int index = 0; index < count; ++index)
    {
        if(history[history.size() - count + index].keys != move[index])
        {
            return false;
        }
    }

    return true;
}

// The icon for an event: one of 8 arrows, from right going clockwise, or a button.
int icon(unsigned keys)
{
    constexpr unsigned order[] = { right, down | right, down, down | left, left, up | left, up, up | right,
                                   a, b, l, r, start, select };

    for(int frame = 0; frame < int(sizeof(order) / sizeof(order[0])); ++frame)
    {
        if(order[frame] == keys)
        {
            return frame;
        }
    }

    return 0;
}

void show_keys(bn::span<const unsigned> keys, int x, int y, bn::ivector<bn::sprite_ptr>& sprites)
{
    for(unsigned value : keys)
    {
        sprites.push_back(bn::sprite_items::icons.create_sprite(x, y, icon(value)));
        x += 18;
    }
}

int main()
{
    bn::core::init();
    bn::backdrop::set_color(bn::color(3, 4, 9));

    bn::sprite_text_generator text_generator(hud::font);
    bn::vector<bn::sprite_ptr, 40> labels;
    text_generator.generate(-112, -66, "Fire", labels);
    text_generator.generate(-112, -48, "Gold", labels);
    text_generator.generate(-112, -30, "Reset", labels);
    text_generator.generate(-112, 62, "Last", labels);

    constexpr unsigned chord_keys[] = { l, r, start, select };
    show_keys(fire, -60, -66, labels);
    show_keys(gold, -60, -48, labels);
    show_keys(chord_keys, -60, -30, labels);
    text_generator.generate(14, -30, "held", labels);

    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(-72, 20);
    bn::sprite_palette_ptr hero_palette = hero.palette();
    bool golden = false;
    bn::vector<bn::sprite_ptr, 4> shots;
    bn::vector<bn::sprite_ptr, 8> last_icons;
    bn::vector<bn::sprite_ptr, 8> time_text;
    unsigned direction = 0;
    int frame = 0;
    int shown_events = 0;

    while(true)
    {
        unsigned held = 0;
        unsigned pressed = 0;

        for(unsigned value : all_keys)
        {
            held |= bn::keypad::held(key(value)) ? value : 0;
            pressed |= bn::keypad::pressed(key(value)) ? value : 0;
        }

        // All four held, and one of them pressed just now: holding them through the reset doesn't reset again.
        if((held & reset_chord) == reset_chord && (pressed & reset_chord))
        {
            bn::core::reset();
        }

        // The D-pad is kept as a direction, so down and right held together are one event: down-right.
        if((held & dpad) != direction)
        {
            direction = held & dpad;

            if(direction)
            {
                remember(direction, frame);
            }
        }

        for(unsigned value : all_keys)
        {
            if((pressed & value) && ! (value & dpad))
            {
                remember(value, frame);
            }
        }

        if((pressed & a) && ends_with(fire, fire_window, frame) && ! shots.full())
        {
            shots.push_back(bn::sprite_items::shot.create_sprite(hero.x() + 16, hero.y()));
        }

        if((pressed & a) && ends_with(gold, gold_window, frame))
        {
            golden = ! golden;
            hero_palette.set_fade(bn::colors::yellow, golden ? 0.5 : 0);
        }

        for(bn::sprite_ptr& shot : shots)
        {
            shot.set_x(shot.x() + 4);
        }

        bn::erase_if(shots, [](const bn::sprite_ptr& shot) { return shot.x() > 128; });

        if(events_count != shown_events)
        {
            shown_events = events_count;
            last_icons.clear();
            int first = bn::max(history.size() - last_icons.max_size(), 0);

            for(int index = first; index < history.size(); ++index)
            {
                last_icons.push_back(bn::sprite_items::icons.create_sprite(-60 + (index - first) * 18, 62,
                                                                           icon(history[index].keys)));
            }
        }

        if(frame % 60 == 0)
        {
            time_text.clear();
            text_generator.generate(70, -66, bn::format<16>("Time {}s", frame / 60), time_text);
        }

        ++frame;
        bn::core::update();
    }
}
