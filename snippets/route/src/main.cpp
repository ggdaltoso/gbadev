#include "bn_core.h"
#include "bn_fixed_point.h"
#include "bn_math.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"

#include "bn_regular_bg_items_village.h"
#include "bn_sprite_items_hero.h"

#include "hero_walk.h"

// One stop on the route: a 16x16 cell of the village, and how many frames to stay there.
struct waypoint
{
    int x;
    int y;
    int wait;
};

// From the door to the notice board and back. snippets/tools/route_assets.py reads these lines
// and lays the dirt path under them.
constexpr waypoint route[] = {
    { 2, 3, 60 },    // The door.
    { 2, 6, 0 },
    { 7, 8, 0 },     // Across the square, on a slant.
    { 12, 8, 0 },
    { 12, 5, 90 },   // In front of the board.
    { 12, 8, 0 },
    { 7, 8, 0 },
    { 2, 6, 0 },
};

constexpr int stops = sizeof(route) / sizeof(route[0]);
constexpr bn::fixed speed = 1.5;   // Pixels per frame, on every leg, straight or slanted.
constexpr int feet_offset = -6;

bn::fixed_point position(const waypoint& stop)
{
    return bn::fixed_point(stop.x * 16 - 112, stop.y * 16 - 72 + feet_offset);
}

int main()
{
    bn::core::init();

    bn::regular_bg_ptr village = bn::regular_bg_items::village.create_bg(0, 0);
    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(position(route[0]));
    hero_walk walk;

    int leg = 0;                   // Walking from route[leg] to the next stop.
    bn::fixed walked = 0;          // How far along this leg, in pixels.
    int waiting = route[0].wait;

    while(true)
    {
        bn::fixed_point before = hero.position();

        if(waiting > 0)
        {
            --waiting;
        }
        else
        {
            const waypoint& from = route[leg];
            const waypoint& to = route[(leg + 1) % stops];   // After the last stop comes the first.
            int dx = (to.x - from.x) * 16;
            int dy = (to.y - from.y) * 16;
            int length = bn::sqrt(dx * dx + dy * dy);      // An int square root: bn::sqrt of a fixed loses bits.
            walked += speed;

            if(walked >= length)
            {
                // Arrived: put it exactly on the stop, never past it.
                hero.set_position(position(to));
                leg = (leg + 1) % stops;
                walked = 0;
                waiting = to.wait;
            }
            else
            {
                // The same fraction of the way on both axes: a straight line at the same speed on every leg.
                hero.set_position(position(from) + bn::fixed_point(dx, dy) * walked / length);
            }
        }

        bn::fixed_point step = hero.position() - before;
        walk.update(hero, (step.x() > 0) - (step.x() < 0), (step.y() > 0) - (step.y() < 0), 2);
        bn::core::update();
    }
}
