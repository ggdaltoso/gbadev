#include "bn_algorithm.h"
#include "bn_colors.h"
#include "bn_core.h"
#include "bn_fixed_rect.h"
#include "bn_format.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_animate_actions.h"
#include "bn_sprite_palette_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_arena.h"
#include "bn_sprite_items_heart.h"
#include "bn_sprite_items_log.h"

#include "font.h"
#include "hero_walk.h"

constexpr int max_health = 5;
constexpr int invincible_frames = 60;   // After a hit, a second in which nothing can hurt the hero again.
constexpr int flash_frames = 4;         // The first frames of it, all white.
constexpr int stun_frames = 16;         // The first frames of it, pushed back and not in control.
constexpr bn::fixed knockback_speed = 6;
constexpr bn::fixed knockback_friction = 0.8;   // The push loses a fifth of its speed every frame.

// Inside the hedge (snippets/tools/damage_assets.py draws it there).
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
        sprite.set_horizontal_flip(velocity.x() < 0);   // The art faces right.
        walk.update();
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

    bn::regular_bg_ptr arena = bn::regular_bg_items::arena.create_bg(0, 0);
    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(-70, 40);
    bn::sprite_palette_ptr hero_palette = hero.palette();   // Shared handle: changing it changes the hero.
    hero_walk walk;

    bn::vector<enemy, 2> enemies;
    enemies.push_back(make_enemy(-60, -30, bn::fixed_point(0.75, 0.5)));
    enemies.push_back(make_enemy(60, 30, bn::fixed_point(-0.5, 0.75)));

    bn::vector<bn::sprite_ptr, max_health> hearts;

    for(int index = 0; index < max_health; ++index)
    {
        hearts.push_back(bn::sprite_items::heart.create_sprite(-104 + index * 14, -72));
    }

    bn::sprite_text_generator text_generator(hud::font);
    bn::vector<bn::sprite_ptr, 8> text;

    int health = max_health;
    int invincible = 0;           // Frames of invincibility left.
    bn::fixed_point knockback;    // The push, losing speed every frame.

    while(true)
    {
        for(enemy& log : enemies)
        {
            log.update();
        }

        bool stunned = invincible > invincible_frames - stun_frames;
        int step_x = stunned ? 0 : bn::keypad::right_held() - bn::keypad::left_held();
        int step_y = stunned ? 0 : bn::keypad::down_held() - bn::keypad::up_held();
        bn::fixed_point position = hero.position() + bn::fixed_point(step_x, step_y) + knockback;
        knockback *= knockback_friction;

        // The hedge stops the push like it stops walking.
        position.set_x(bn::clamp(position.x(), min_x, max_x));
        position.set_y(bn::clamp(position.y(), min_y, max_y));
        hero.set_position(position);
        walk.update(hero, step_x, step_y);

        if(invincible > 0)
        {
            --invincible;
        }
        else
        {
            bn::fixed_rect hero_box(hero.position() + bn::fixed_point(0, 6), bn::fixed_size(12, 16));

            for(const enemy& log : enemies)
            {
                if(hero_box.intersects(bn::fixed_rect(log.sprite.position() + bn::fixed_point(0, 6), bn::fixed_size(20, 14))))
                {
                    health = health > 1 ? health - 1 : max_health;   // No game over here: back to full.
                    invincible = invincible_frames;

                    // Away from the log, always at the same speed: the difference divided by its length.
                    int dx = (hero.x() - log.sprite.x()).round_integer();
                    int dy = (hero.y() - log.sprite.y()).round_integer();
                    int length = bn::max(bn::sqrt(dx * dx + dy * dy), 1);
                    knockback = bn::fixed_point(dx, dy) * knockback_speed / length;
                    break;
                }
            }
        }

        // White for the first frames, then blinking: 4 frames shown, 4 hidden.
        // Worked out from the counter, so when it reaches 0 the hero is always visible.
        bool flashing = invincible > invincible_frames - flash_frames;
        hero_palette.set_fade(bn::colors::white, flashing ? 1 : 0);
        hero.set_visible(flashing || invincible % 8 < 4);

        for(int index = 0; index < max_health; ++index)
        {
            hearts[index].set_tiles(bn::sprite_items::heart.tiles_item(), index < health ? 0 : 1);
        }

        text.clear();

        if(invincible > 0)
        {
            text_generator.generate(-104, 72, bn::format<20>("Invincible {}", invincible), text);
        }

        bn::core::update();
    }
}
