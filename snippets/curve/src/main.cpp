#include "bn_core.h"
#include "bn_fixed.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_random.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_string.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_track.h"
#include "bn_sprite_items_hero.h"
#include "bn_sprite_items_obstacles.h"

#include "hero_walk.h"

#include "font.h"

constexpr int hero_y = -40;
constexpr int lane_width = 32;
constexpr int spawn_ahead = 140;
constexpr int lap = 256;

// One row of the curve: from this total distance on, this speed and this gap between obstacles.
struct stage
{
    int from;
    bn::fixed speed;   // Pixels of track per frame.
    int spacing;       // Pixels of track between two obstacles.
};

// The whole difficulty of the game, as data. Sorted by distance. Past the last row, it stays there.
constexpr stage curve[] = {
    { 0, 1.5, 104 },
    { 256, 2.5, 80 },
    { 512, 3, 64 },
    { 768, 3.5, 56 },
};

constexpr int rows = sizeof(curve) / sizeof(curve[0]);

// The difficulty at a total distance: the row it's in, blended toward the next row.
stage difficulty(int total)
{
    int row = 0;

    while(row + 1 < rows && curve[row + 1].from <= total)
    {
        ++row;
    }

    if(row + 1 == rows)
    {
        return curve[row];
    }

    const stage& now = curve[row];
    const stage& next = curve[row + 1];
    int done = total - now.from;
    int length = next.from - now.from;
    return stage{ total, now.speed + (next.speed - now.speed) * done / length,
                  now.spacing + (next.spacing - now.spacing) * done / length };
}

struct obstacle
{
    bn::sprite_ptr sprite;
    int lane;
    bn::fixed at;
};

int main()
{
    bn::core::init();

    bn::regular_bg_ptr track = bn::regular_bg_items::track.create_bg(0, 0);
    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(0, hero_y);
    hero_walk walk;
    bn::vector<obstacle, 8> obstacles;
    bn::random random;

    bn::fixed distance = 0;
    int laps = 0;
    bn::fixed next_obstacle = 200;
    int lane = 1;
    int safe_frames = 0;

    bn::sprite_text_generator text_generator(hud::font);
    text_generator.set_center_alignment();
    bn::vector<bn::sprite_ptr, 12> text_sprites;
    int shown_total = -1;

    while(true)
    {
        // The total distance, not the distance in this lap: that one starts again every 256 pixels.
        int total = laps * lap + distance.integer();
        stage now = difficulty(total);

        lane = bn::clamp(lane + bn::keypad::right_pressed() - bn::keypad::left_pressed(), 0, 2);
        bn::fixed lane_x = (lane - 1) * lane_width;
        hero.set_x(hero.x() + bn::clamp(lane_x - hero.x(), bn::fixed(-4), bn::fixed(4)));
        walk.update(hero, 0, 1, now.speed.round_integer());

        distance += now.speed;

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

        // Each gap is the curve's gap when its obstacle is made: the gaps shrink as the run goes on.
        while(next_obstacle <= distance + spawn_ahead)
        {
            bn::sprite_ptr sprite = bn::sprite_items::obstacles.create_sprite(0, 0, random.get_int(3));
            obstacles.push_back(obstacle{ sprite, random.get_int(3), next_obstacle });
            next_obstacle += now.spacing;
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

            if(safe_frames == 0 && bn::abs(y - hero_y) < 12 && bn::abs(ahead.sprite.x() - hero.x()) < 14)
            {
                safe_frames = 60;
            }
        }

        bn::erase_if(obstacles, [distance](const obstacle& passed) { return passed.at - distance < -60; });
        hero.set_visible(safe_frames / 4 % 2 == 0);

        if(total / 16 != shown_total)
        {
            shown_total = total / 16;
            bn::string<48> text;
            bn::ostringstream stream(text);
            stream.set_precision(3);   // Digits before and after the point together: "2.75".
            stream << "Distance: " << total << "  Speed: " << now.speed << "  Gap: " << now.spacing;
            text_sprites.clear();
            text_generator.generate(0, -70, text, text_sprites);
        }

        bn::core::update();
    }
}
