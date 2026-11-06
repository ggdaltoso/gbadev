#include "bn_core.h"
#include "bn_algorithm.h"
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
#include "log_enemy.h"

int main()
{
    bn::core::init();

    bn::regular_bg_ptr world = bn::regular_bg_items::world.create_bg(0, 0);
    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(0, 0);

    // Room for 8 logs, reserved up front: no allocation while the game runs.
    bn::vector<log_enemy, 8> logs;

    constexpr bn::fixed_point spawn_points[] = { {-104, -56}, {104, 56}, {104, -56}, {-104, 56} };
    int next_spawn = 0;
    int frames_to_spawn = 0;

    bn::sprite_text_generator text_generator(hud::font);
    text_generator.set_center_alignment();
    bn::vector<bn::sprite_ptr, 4> count_sprites;
    int shown_count = -1;

    while(true)
    {
        int wanted_x = hero.x().integer() + (bn::keypad::right_held() - bn::keypad::left_held()) * 2;
        int wanted_y = hero.y().integer() + (bn::keypad::down_held() - bn::keypad::up_held()) * 2;
        hero.set_position(bn::clamp(wanted_x, -104, 104), bn::clamp(wanted_y, -64, 64));

        --frames_to_spawn;

        if(frames_to_spawn <= 0 && ! logs.full())
        {
            // Builds the log right inside the vector, with the arguments of its constructor.
            logs.emplace_back(spawn_points[next_spawn]);
            next_spawn = (next_spawn + 1) % 4;
            frames_to_spawn = 60;
        }

        // A reference: without the &, each log would update a copy of itself.
        for(log_enemy& log : logs)
        {
            log.update(hero.position());
        }

        bn::erase_if(logs, [](const log_enemy& log)
        {
            return log.gone();
        });

        int awake_count = 0;

        for(const log_enemy& log : logs)
        {
            awake_count += ! log.asleep();
        }

        if(awake_count != shown_count)
        {
            shown_count = awake_count;
            count_sprites.clear();
            text_generator.generate(0, -70, bn::format<16>("Awake: {}", awake_count), count_sprites);
        }

        bn::core::update();
    }
}
