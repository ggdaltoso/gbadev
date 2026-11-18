#include "bn_blending.h"
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
#include "bn_sprite_items_bare_hero.h"
#include "bn_sprite_items_obstacles.h"
#include "bn_sprite_items_shadow.h"

#include "hero_walk.h"

#include "font.h"

constexpr bn::fixed run_speed = 2;
constexpr int hero_y = -16;          // Where the hero stands on the screen: its place on the ground.
constexpr int feet = 10;             // The boots end 10 pixels below the sprite's center.
constexpr int lane_width = 32;
constexpr int spacing = 72;
constexpr int spawn_ahead = 140;
constexpr int lap = 256;

constexpr bn::fixed jump_speed = 4;  // Height gained in the first frame of a jump.
constexpr bn::fixed gravity = 0.25;  // Height speed lost every frame.

// How tall each obstacle is, by frame of obstacles.bmp: a bush, a log and a beehive.
constexpr int heights[] = { 12, 8, 14 };

struct obstacle
{
    bn::sprite_ptr sprite;
    int lane;
    bn::fixed at;
    int height;
};

int main()
{
    bn::core::init();

    bn::regular_bg_ptr track = bn::regular_bg_items::track.create_bg(0, 0);

    // The shadow stays on the ground. It's behind the obstacles: a sprite only blends with the
    // background, so over an obstacle it would hide it instead of darkening it.
    bn::sprite_ptr shadow = bn::sprite_items::shadow.create_sprite(0, hero_y + feet - 1);
    shadow.set_z_order(1);
    shadow.set_blending_enabled(true);
    bn::blending::set_transparency_alpha(0.5);

    // In front of the obstacles, which are made after it and would win the tie.
    bn::sprite_ptr hero = bn::sprite_items::bare_hero.create_sprite(0, hero_y);
    hero.set_z_order(-1);
    hero_walk walk(bn::sprite_items::bare_hero);

    bn::vector<obstacle, 8> obstacles;
    bn::random random;
    bn::fixed distance = 0;
    int laps = 0;
    bn::fixed next_obstacle = 200;
    int lane = 1;
    int bumps = 0;
    int safe_frames = 0;

    // The third axis: how high above the ground the hero is, and how fast that changes.
    bn::fixed z = 0;
    bn::fixed z_speed = 0;

    bn::sprite_text_generator text_generator(hud::font);
    text_generator.set_center_alignment();
    bn::vector<bn::sprite_ptr, 8> text_sprites;
    int shown_total = -1;

    while(true)
    {
        lane = bn::clamp(lane + bn::keypad::right_pressed() - bn::keypad::left_pressed(), 0, 2);
        bn::fixed lane_x = (lane - 1) * lane_width;
        hero.set_x(hero.x() + bn::clamp(lane_x - hero.x(), bn::fixed(-4), bn::fixed(4)));

        // Only from the ground. Gravity works on z alone: the place on the ground keeps running.
        if(z == 0 && bn::keypad::a_pressed())
        {
            z_speed = jump_speed;
        }

        z += z_speed;

        if(z > 0)
        {
            z_speed -= gravity;
        }
        else
        {
            z = 0;
            z_speed = 0;
        }

        // In the air the legs stop: step 0 shows the first frame of the walk.
        walk.update(hero, 0, z == 0 ? 1 : 0, run_speed.integer());

        // Drawn z pixels above its place on the ground. The shadow marks that place, smaller when higher.
        hero.set_y(hero_y - z);
        shadow.set_x(hero.x());
        shadow.set_tiles(bn::sprite_items::shadow.tiles_item(), z < 10 ? 0 : z < 22 ? 1 : 2);

        distance += run_speed;

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

        while(next_obstacle <= distance + spawn_ahead)
        {
            int kind = random.get_int(3);
            bn::sprite_ptr sprite = bn::sprite_items::obstacles.create_sprite(0, 0, kind);
            obstacles.push_back(obstacle{ sprite, random.get_int(3), next_obstacle, heights[kind] });
            next_obstacle += spacing;
        }

        track.set_y(-distance);

        if(safe_frames > 0)
        {
            --safe_frames;
        }

        for(obstacle& ahead : obstacles)
        {
            bn::fixed y = hero_y + ahead.at - distance;
            ahead.sprite.set_position((ahead.lane - 1) * lane_width, y);

            // A bump needs both: overlapping on the ground (hero_y, not where the hero is drawn),
            // and lower than the obstacle is tall.
            bool on_ground = bn::abs(y - hero_y) < 12 && bn::abs(ahead.sprite.x() - hero.x()) < 14;

            if(safe_frames == 0 && on_ground && z < ahead.height)
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
