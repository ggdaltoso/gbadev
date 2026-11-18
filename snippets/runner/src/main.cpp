#include "bn_core.h"
#include "bn_fixed.h"
#include "bn_format.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_random.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_track.h"
#include "bn_sprite_items_hero.h"
#include "bn_sprite_items_obstacles.h"

#include "hero_walk.h"

#include "font.h"

constexpr bn::fixed run_speed = 2;   // Pixels of track per frame.
constexpr int hero_y = -40;          // The hero stays here on the screen; the track moves.
constexpr int lane_width = 32;
constexpr int spacing = 72;          // Pixels of track between two obstacles.
constexpr int spawn_ahead = 140;     // Obstacles are made this far ahead: below the bottom of the screen.
constexpr int lap = 256;             // The track's height: it repeats every 256 pixels.

// An obstacle only knows its lane and how far along the track it is.
struct obstacle
{
    bn::sprite_ptr sprite;
    int lane;
    bn::fixed at;   // The distance at which the hero reaches it.
};

int main()
{
    bn::core::init();

    bn::regular_bg_ptr track = bn::regular_bg_items::track.create_bg(0, 0);
    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(0, hero_y);
    hero_walk walk;
    bn::vector<obstacle, 8> obstacles;
    bn::random random;

    bn::fixed distance = 0;   // How far the hero has run in this lap.
    int laps = 0;
    bn::fixed next_obstacle = 200;
    int lane = 1;
    int bumps = 0;
    int safe_frames = 0;

    bn::sprite_text_generator text_generator(hud::font);
    text_generator.set_center_alignment();
    bn::vector<bn::sprite_ptr, 8> text_sprites;
    int shown_total = -1;

    while(true)
    {
        lane = bn::clamp(lane + bn::keypad::right_pressed() - bn::keypad::left_pressed(), 0, 2);
        bn::fixed lane_x = (lane - 1) * lane_width;
        hero.set_x(hero.x() + bn::clamp(lane_x - hero.x(), bn::fixed(-4), bn::fixed(4)));
        walk.update(hero, 0, 1, run_speed.integer());   // Always running down, legs at the track's pace.

        distance += run_speed;

        // A lap: every 256 pixels, everything measured in distance goes back by 256 together. The track
        // looks the same, and the numbers stay small instead of growing until bn::fixed runs out.
        if(distance >= lap)
        {
            distance -= lap;
            next_obstacle -= lap;
            ++laps;

            for(obstacle& ahead : obstacles)
            {
                ahead.at -= lap;
            }
        }

        // Obstacles come by distance: one every `spacing` pixels of track, whatever the speed.
        while(next_obstacle <= distance + spawn_ahead)
        {
            int obstacle_lane = random.get_int(3);
            bn::sprite_ptr sprite = bn::sprite_items::obstacles.create_sprite(0, 0, random.get_int(3));
            obstacles.push_back(obstacle{ sprite, obstacle_lane, next_obstacle });
            next_obstacle += spacing;
        }

        // The track and the obstacles come from the same number, so the obstacles stay on the ground.
        track.set_y(-distance);

        if(safe_frames > 0)
        {
            --safe_frames;
        }

        for(obstacle& ahead : obstacles)
        {
            bn::fixed y = hero_y + ahead.at - distance;
            ahead.sprite.set_position((ahead.lane - 1) * lane_width, y);

            if(safe_frames == 0 && bn::abs(y - hero_y) < 12 && bn::abs(ahead.sprite.x() - hero.x()) < 14)
            {
                ++bumps;
                safe_frames = 60;
            }
        }

        bn::erase_if(obstacles, [distance](const obstacle& passed) { return passed.at - distance < -60; });
        hero.set_visible(safe_frames / 4 % 2 == 0);

        int total = laps * lap + distance.integer();

        if(total / 16 != shown_total)
        {
            shown_total = total / 16;
            text_sprites.clear();
            text_generator.generate(0, -70, bn::format<32>("Distance: {}  Bumps: {}", total, bumps), text_sprites);
        }

        bn::core::update();
    }
}
