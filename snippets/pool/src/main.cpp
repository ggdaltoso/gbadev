#include "bn_core.h"
#include "bn_format.h"
#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_world.h"
#include "bn_sprite_items_hero.h"
#include "bn_sprite_items_stone.h"

#include "hero_walk.h"

#include "font.h"

// A stone flies, then breaks into dust. Its sprite is made once, at the start, and kept for good:
// throwing a stone wakes one up, and a stone that's done goes back to sleep.
struct stone
{
    bn::sprite_ptr sprite;
    bn::fixed_point step;
    int age = 0;
    bool awake = false;
};

constexpr int fly_frames = 40;
constexpr int frames_per_dust = 4;
constexpr int life_frames = fly_frames + frames_per_dust * 6;   // Frames 1-6 of stone.bmp are the dust.
constexpr int throw_frames = 8;   // One stone every 8 frames while A is held.

// The most stones awake at once: one stone's life divided by the time between throws.
constexpr int pool_size = life_frames / throw_frames;

int main()
{
    bn::core::init();

    bn::regular_bg_ptr world = bn::regular_bg_items::world.create_bg(0, 0);
    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(0, 0);
    hero_walk walk;

    // The pool: every stone made now, hidden, and never erased. The vector is filled once and
    // keeps its size, so no stone ever moves inside it.
    bn::vector<stone, pool_size> stones;

    while(! stones.full())
    {
        bn::sprite_ptr sprite = bn::sprite_items::stone.create_sprite(0, 0);
        sprite.set_visible(false);
        stones.push_back(stone{ sprite, bn::fixed_point() });
    }

    bn::fixed_point facing(1, 0);
    int reload = 0;

    bn::sprite_text_generator text_generator(hud::font);
    text_generator.set_center_alignment();
    bn::vector<bn::sprite_ptr, 8> count_sprites;
    bn::vector<bn::sprite_ptr, 8> hint_sprites;
    text_generator.generate(0, 70, "A: throw", hint_sprites);
    int shown_asleep = -1;

    while(true)
    {
        int step_x = bn::keypad::right_held() - bn::keypad::left_held();
        int step_y = bn::keypad::down_held() - bn::keypad::up_held();
        hero.set_position(hero.position() + bn::fixed_point(step_x, step_y));
        walk.update(hero, step_x, step_y);

        // Throws go the way the hero faces. Sideways wins on diagonals, like the walk frames.
        if(step_x != 0)
        {
            facing = bn::fixed_point(step_x, 0);
        }
        else if(step_y != 0)
        {
            facing = bn::fixed_point(0, step_y);
        }

        if(reload > 0)
        {
            --reload;
        }

        if(bn::keypad::a_held() && reload == 0)
        {
            reload = throw_frames;

            // The first stone asleep wakes up. If they're all awake, there's no throw.
            for(stone& thrown : stones)
            {
                if(! thrown.awake)
                {
                    // Everything a stone had from its last throw is set again: where it is, its way,
                    // its age and its frame. Only the sprite is kept.
                    thrown.sprite.set_position(hero.position() + bn::fixed_point(0, 4));
                    thrown.sprite.set_tiles(bn::sprite_items::stone.tiles_item(), 0);
                    thrown.sprite.set_visible(true);
                    thrown.step = facing * 2;
                    thrown.age = 0;
                    thrown.awake = true;
                    break;
                }
            }
        }

        int asleep = 0;

        for(stone& thrown : stones)
        {
            if(! thrown.awake)
            {
                ++asleep;
                continue;
            }

            ++thrown.age;
            int dust_age = thrown.age - fly_frames;

            if(dust_age < 0)
            {
                thrown.sprite.set_position(thrown.sprite.position() + thrown.step);
            }
            else if(thrown.age == life_frames)
            {
                // Back to sleep: hidden, but still made, and ready for the next throw.
                thrown.sprite.set_visible(false);
                thrown.awake = false;
            }
            else if(dust_age % frames_per_dust == 0)
            {
                thrown.sprite.set_tiles(bn::sprite_items::stone.tiles_item(), 1 + dust_age / frames_per_dust);
            }
        }

        if(asleep != shown_asleep)
        {
            shown_asleep = asleep;
            count_sprites.clear();
            text_generator.generate(0, -70, bn::format<16>("Asleep: {}", asleep), count_sprites);
        }

        bn::core::update();
    }
}
