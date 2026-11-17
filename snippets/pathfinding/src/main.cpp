#include "bn_array.h"
#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_point.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_animate_actions.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_maze.h"
#include "bn_sprite_items_coin.h"
#include "bn_sprite_items_dot.h"
#include "bn_sprite_items_hero.h"

#include "hero_walk.h"
#include "maze.h"

// Steps from each cell to the goal: distance[row][column]. -1 for crates.
using distance_map = bn::array<bn::array<int, maze::columns>, maze::rows>;

constexpr bn::array<bn::point, 4> steps = { bn::point(1, 0), bn::point(-1, 0), bn::point(0, 1), bn::point(0, -1) };
constexpr bn::array<bn::point, 4> goals = { bn::point(13, 8), bn::point(3, 3), bn::point(11, 3), bn::point(7, 8) };
constexpr int hero_feet = 10;   // From the sprite's center down to its feet: the hero stands where its feet are.

bool is_floor(bn::point cell)
{
    return maze::cells[cell.y()][cell.x()] == '.';
}

int& at(distance_map& distance, bn::point cell)
{
    return distance[cell.y()][cell.x()];
}

// Breadth-first search, out from the goal. Cells leave the queue in the order they joined it,
// so every cell at distance 1 comes before any at distance 2, and so on: the first time a cell
// is reached is by a shortest way.
void search_from(bn::point goal, distance_map& distance)
{
    for(bn::array<int, maze::columns>& row : distance)
    {
        row.fill(-1);
    }

    // Each cell joins the queue once, so a vector with room for every cell is enough:
    // reading moves forward through it and nothing is ever removed.
    bn::vector<bn::point, maze::columns * maze::rows> queue;
    at(distance, goal) = 0;
    queue.push_back(goal);

    for(int next = 0; next < queue.size(); ++next)
    {
        bn::point cell = queue[next];

        for(bn::point step : steps)
        {
            // No bounds check: the edge is all crates, so a floor cell's neighbors are inside the maze.
            bn::point neighbor = cell + step;

            if(is_floor(neighbor) && at(distance, neighbor) < 0)
            {
                at(distance, neighbor) = at(distance, cell) + 1;   // Set when queued, so it's queued only once.
                queue.push_back(neighbor);
            }
        }
    }
}

bn::point cell_at(bn::fixed x, bn::fixed y)
{
    return bn::point((x + 120).floor_integer() / maze::cell_size, (y + 80).floor_integer() / maze::cell_size);
}

bn::fixed_point center_of(bn::point cell)
{
    return bn::fixed_point(cell.x() * maze::cell_size - 120 + 8, cell.y() * maze::cell_size - 80 + 8);
}

// The hero's feet, an 8x6 box: blocked if any corner is on a crate.
bool feet_blocked(bn::fixed x, bn::fixed y)
{
    bn::fixed top = y + hero_feet - 3;
    bn::fixed bottom = y + hero_feet + 2;
    return ! is_floor(cell_at(x - 4, top)) || ! is_floor(cell_at(x + 3, top)) ||
            ! is_floor(cell_at(x - 4, bottom)) || ! is_floor(cell_at(x + 3, bottom));
}

int main()
{
    bn::core::init();

    bn::regular_bg_ptr maze_bg = bn::regular_bg_items::maze.create_bg(0, 0);

    int goal_index = 0;
    bn::sprite_ptr coin = bn::sprite_items::coin.create_sprite(center_of(goals[goal_index]));
    bn::sprite_animate_action<4> spin = bn::create_sprite_animate_action_forever(
                coin, 6, bn::sprite_items::coin.tiles_item(), 0, 1, 2, 3);

    bn::fixed_point start = center_of(bn::point(1, 1));
    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(start.x(), start.y() - hero_feet);
    hero.set_z_order(-1);   // In front of the trail's dots, which are made later and would be on top.
    hero_walk walk;

    // One search answers every start: the map only changes when the goal moves, not when the hero does.
    distance_map distance;
    search_from(goals[goal_index], distance);

    bn::vector<bn::sprite_ptr, 64> dots;
    bn::point trail_from(-1, -1);

    while(true)
    {
        // One axis at a time, like the map collision page: a crate on one axis doesn't stop the other.
        int step_x = bn::keypad::right_held() - bn::keypad::left_held();
        int step_y = bn::keypad::down_held() - bn::keypad::up_held();

        if(! feet_blocked(hero.x() + step_x, hero.y()))
        {
            hero.set_x(hero.x() + step_x);
        }

        if(! feet_blocked(hero.x(), hero.y() + step_y))
        {
            hero.set_y(hero.y() + step_y);
        }

        walk.update(hero, step_x, step_y);
        bn::point here = cell_at(hero.x(), hero.y() + hero_feet);

        if(here == goals[goal_index])
        {
            goal_index = (goal_index + 1) % goals.size();
            coin.set_position(center_of(goals[goal_index]));
            search_from(goals[goal_index], distance);
            trail_from = bn::point(-1, -1);   // Same cell, new map: the trail must be drawn again.
        }

        // The trail: from the hero's cell, keep stepping to a neighbor one step closer to the goal.
        // Drawn again only when the hero changes cell.
        if(here != trail_from)
        {
            trail_from = here;
            dots.clear();
            bn::point cell = here;

            while(at(distance, cell) > 1 && ! dots.full())
            {
                for(bn::point step : steps)
                {
                    if(at(distance, cell + step) == at(distance, cell) - 1)
                    {
                        cell += step;
                        break;
                    }
                }

                dots.push_back(bn::sprite_items::dot.create_sprite(center_of(cell)));
            }
        }

        spin.update();
        bn::core::update();
    }
}
