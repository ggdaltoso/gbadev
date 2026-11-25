#include "bn_algorithm.h"
#include "bn_bg_palettes.h"
#include "bn_colors.h"
#include "bn_core.h"
#include "bn_format.h"
#include "bn_keypad.h"
#include "bn_music.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_animate_actions.h"
#include "bn_sprite_palette_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "bn_music_items.h"
#include "bn_regular_bg_items_arena.h"
#include "bn_sprite_items_heart.h"
#include "bn_sprite_items_log.h"

#include "font.h"
#include "hero_walk.h"

constexpr int round_frames = 30 * 60;   // A 30-second round, counted in frames.
constexpr bn::fixed dim = 0.6;          // How dark the game gets behind the menu.

// Inside the hedge.
constexpr bn::fixed min_x = -88, max_x = 88, min_y = -56, max_y = 50;

struct enemy
{
    bn::sprite_ptr sprite;
    bn::sprite_animate_action<4> walk;
    bn::fixed_point velocity;

    void update()
    {
        bn::fixed_point position = sprite.position() + velocity;

        if(position.x() < min_x || position.x() > max_x)
        {
            velocity.set_x(-velocity.x());
        }

        if(position.y() < min_y || position.y() > max_y)
        {
            velocity.set_y(-velocity.y());
        }

        sprite.set_position(sprite.position() + velocity);
        sprite.set_horizontal_flip(velocity.x() < 0);
        walk.update();   // The animation only moves when this is called.
    }
};

enemy make_enemy(bn::fixed x, bn::fixed y, bn::fixed_point velocity)
{
    bn::sprite_ptr sprite = bn::sprite_items::log.create_sprite(x, y);
    bn::sprite_animate_action<4> walk = bn::create_sprite_animate_action_forever(
                sprite, 8, bn::sprite_items::log.tiles_item(), 0, 1, 2, 3);
    return { sprite, walk, velocity };
}

int main()
{
    bn::core::init();
    bn::music_items::field.play(0.5);

    bn::regular_bg_ptr arena = bn::regular_bg_items::arena.create_bg(0, 0);
    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(0, 0);
    hero_walk walk;
    bn::vector<enemy, 2> enemies;
    int time_left = 0;

    // A new round: everything back where it started.
    auto restart = [&]()
    {
        hero.set_position(-70, 40);
        enemies.clear();
        enemies.push_back(make_enemy(-60, -30, bn::fixed_point(0.75, 0.5)));
        enemies.push_back(make_enemy(60, 30, bn::fixed_point(-0.5, 0.75)));
        time_left = round_frames;
    };

    restart();

    // Handles to the game's own palettes, so the menu can darken them and nothing else.
    bn::sprite_palette_ptr hero_palette = hero.palette();
    bn::sprite_palette_ptr log_palette = enemies[0].sprite.palette();   // Both logs share it.

    bn::sprite_text_generator text_generator(hud::font);
    text_generator.set_center_alignment();
    bn::vector<bn::sprite_ptr, 8> menu;
    text_generator.generate(0, -24, "Paused", menu);
    text_generator.generate(0, 0, "Resume", menu);
    text_generator.generate(0, 16, "Restart", menu);
    bn::sprite_ptr cursor = bn::sprite_items::heart.create_sprite(-36, 0);

    text_generator.set_left_alignment();
    bn::vector<bn::sprite_ptr, 4> clock;
    int shown_seconds = -1;

    bool paused = false;
    int choice = 0;        // 0 is Resume, 1 is Restart.
    int menu_frames = 0;   // The menu's own clock, for the cursor's blink.

    while(true)
    {
        if(bn::keypad::start_pressed())
        {
            paused = ! paused;
            choice = 0;
            menu_frames = 0;
        }
        else if(paused)
        {
            // Only the menu runs: its own keys and its own counter.
            if(bn::keypad::up_pressed() || bn::keypad::down_pressed())
            {
                choice = 1 - choice;
            }

            if(bn::keypad::a_pressed())
            {
                if(choice == 1)
                {
                    restart();
                }

                paused = false;
            }

            ++menu_frames;
        }
        else
        {
            // The game: skipped whole while paused, so everything in it waits, its clock too.
            for(enemy& log : enemies)
            {
                log.update();
            }

            int step_x = bn::keypad::right_held() - bn::keypad::left_held();
            int step_y = bn::keypad::down_held() - bn::keypad::up_held();
            hero.set_position(bn::clamp(hero.x() + step_x, min_x, max_x), bn::clamp(hero.y() + step_y, min_y, max_y));
            walk.update(hero, step_x, step_y);

            if(--time_left == 0)
            {
                restart();
            }
        }

        // Butano plays the music by itself, outside this loop, so it has to be paused apart.
        if(paused && ! bn::music::paused())
        {
            bn::music::pause();
        }
        else if(! paused && bn::music::paused())
        {
            bn::music::resume();
        }

        bn::fixed fade = paused ? dim : 0;
        bn::bg_palettes::set_fade(bn::colors::black, fade);
        hero_palette.set_fade(bn::colors::black, fade);
        log_palette.set_fade(bn::colors::black, fade);

        for(bn::sprite_ptr& letter : menu)
        {
            letter.set_visible(paused);
        }

        cursor.set_visible(paused && menu_frames % 32 < 24);
        cursor.set_y(choice * 16);

        int seconds = (time_left + 59) / 60;   // Rounded up: "Time 1" until the very last frame.

        if(seconds != shown_seconds)
        {
            shown_seconds = seconds;
            clock.clear();
            text_generator.generate(-104, -72, bn::format<12>("Time {}", seconds), clock);
        }

        bn::core::update();
    }
}
