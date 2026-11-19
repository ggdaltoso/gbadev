#include "bn_algorithm.h"
#include "bn_core.h"
#include "bn_fixed.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_level.h"
#include "bn_sprite_items_mover.h"
#include "bn_sprite_items_player.h"

#include "font.h"

constexpr bn::fixed gravity = 0.25;
constexpr bn::fixed jump_speed = -5.5;
constexpr bn::fixed max_fall_speed = 4;
constexpr bn::fixed walk_speed = 1.5;
constexpr int feet = 12;   // From the sprite's center down to its feet.

// The level, one character per 18x18 cell: '#' is solid, '-' is a plank you can jump up through.
// snippets/tools/platforms_assets.py draws it from here.
constexpr int cell = 18;
constexpr int columns = 14;
constexpr int rows = 9;
constexpr int origin_x = -126;   // Screen position of the level's top-left corner.
constexpr int origin_y = -81;

constexpr const char* level[rows] = {
    "..............",
    "..............",
    "..............",
    "..........##..",
    "..............",
    "---...........",
    "..............",
    "..............",
    "####......##..",
};

char cell_at(int x, int y)
{
    int column = (x - origin_x) / cell;
    int row = (y - origin_y) / cell;

    if(x < origin_x || column >= columns)
    {
        return '#';   // Walls at both sides.
    }

    return y >= origin_y && row < rows ? level[row][column] : '.';
}

// The player's box, from the sprite's center: 7 pixels to each side, 10 up to the head, 12 down to the feet.
bool hits(bn::fixed x, bn::fixed y)
{
    int left = (x - 7).floor_integer();
    int right = (x + 6).floor_integer();
    int top = (y - 10).floor_integer();
    int bottom = (y + 11).floor_integer();
    return cell_at(left, top) == '#' || cell_at(right, top) == '#' ||
           cell_at(left, bottom) == '#' || cell_at(right, bottom) == '#';
}

// A platform that goes back and forth on one axis, one pixel per frame, resting a second at each end.
// 36 pixels wide, landed on from above.
constexpr int end_wait = 60;

struct mover
{
    bn::sprite_ptr sprite;
    int direction_x;   // Its axis and which way it's going: one of the two is 0.
    int direction_y;
    int low;           // The ends of its trip, on its axis.
    int high;
    int wait = 0;      // Frames left resting at an end.
    int moved_x = 0;   // This frame's move: what anyone standing on it is carried by.
    int moved_y = 0;

    [[nodiscard]] int top() const
    {
        return sprite.y().integer() - 9;
    }

    void update()
    {
        moved_x = 0;
        moved_y = 0;

        if(wait > 0)
        {
            --wait;
            return;
        }

        int at = direction_x != 0 ? sprite.x().integer() : sprite.y().integer();
        int direction = direction_x + direction_y;

        if(at + direction < low || at + direction > high)
        {
            direction_x = -direction_x;   // Turn around, after a rest.
            direction_y = -direction_y;
            wait = end_wait;
            return;
        }

        moved_x = direction_x;
        moved_y = direction_y;
        sprite.set_position(sprite.x() + moved_x, sprite.y() + moved_y);
    }
};

// A one-way surface is only solid for feet that were above it last frame and are on or below it now:
// rising through it does nothing, and only a fall can land on it.
bool lands(int top, bn::fixed old_feet, bn::fixed new_feet)
{
    return old_feet <= top && new_feet >= top;
}

int main()
{
    bn::core::init();

    bn::regular_bg_ptr background = bn::regular_bg_items::level.create_bg(0, 0);
    bn::sprite_ptr player = bn::sprite_items::player.create_sprite(-100, 63 - feet);

    // Across the pit, and up and down the shaft on the right. Their y is the sprite's center: top + 9.
    bn::vector<mover, 2> movers;
    movers.push_back({ bn::sprite_items::mover.create_sprite(-36, 63 + 9), 1, 0, -36, 36 });
    movers.push_back({ bn::sprite_items::mover.create_sprite(108, 63 + 9), 0, -1, -27 + 9, 63 + 9 });

    bn::sprite_text_generator text_generator(hud::font);
    bn::vector<bn::sprite_ptr, 8> text;
    const char* shown = "";

    bn::fixed velocity_y = 0;
    int riding = -1;           // The mover the player stood on last frame, or -1.

    while(true)
    {
        // Platforms move first. Whoever stands on one moves with it, by exactly as much, before anything else.
        for(mover& platform : movers)
        {
            platform.update();
        }

        bn::fixed x = player.x();
        bn::fixed y = player.y();

        if(riding >= 0)
        {
            x += movers[riding].moved_x;
            y += movers[riding].moved_y;
        }

        int direction = bn::keypad::right_held() - bn::keypad::left_held();
        bn::fixed new_x = x + direction * walk_speed;

        if(! hits(new_x, y))
        {
            x = new_x;
        }

        bool standing = riding >= 0 || hits(x, y + 1) ||
                        cell_at((x - 7).floor_integer(), (y + feet).integer()) == '-' ||
                        cell_at((x + 6).floor_integer(), (y + feet).integer()) == '-';

        if(bn::keypad::a_pressed() && standing)
        {
            velocity_y = jump_speed;
        }

        velocity_y = bn::min(velocity_y + gravity, max_fall_speed);
        bn::fixed old_feet = y + feet;
        bn::fixed new_y = y + velocity_y;
        riding = -1;
        bool on_plank = false;

        if(hits(x, new_y))
        {
            int row = velocity_y < 0 ? ((new_y - 10).floor_integer() - origin_y) / cell + 1 :
                                       ((new_y + 11).floor_integer() - origin_y) / cell;
            new_y = velocity_y < 0 ? origin_y + row * cell + 10 : origin_y + row * cell - feet;
            velocity_y = 0;
        }
        else if(velocity_y >= 0)
        {
            // Planks: the top of the cell the feet are going into.
            for(int side_x : { (x - 7).floor_integer(), (x + 6).floor_integer() })
            {
                int row = ((new_y + feet).floor_integer() - origin_y) / cell;
                int top = origin_y + row * cell;

                if(cell_at(side_x, top) == '-' && lands(top, old_feet, new_y + feet))
                {
                    new_y = top - feet;
                    velocity_y = 0;
                    on_plank = true;
                }
            }

            for(int index = 0; index < movers.size(); ++index)
            {
                const mover& platform = movers[index];
                bool over = bn::abs(x - platform.sprite.x()) < 18 + 7;

                if(over && lands(platform.top(), old_feet, new_y + feet))
                {
                    new_y = platform.top() - feet;
                    velocity_y = 0;
                    riding = index;
                }
            }
        }

        // Fell into the pit: back to the start.
        if(new_y > 100)
        {
            x = -100;
            new_y = 63 - feet;
            velocity_y = 0;
        }

        player.set_position(x, new_y);

        if(hits(x, new_y + 1))
        {
            velocity_y = 0;   // Standing on a block: no speed builds up while standing.
        }

        const char* on = riding >= 0 ? "Moving platform" : on_plank ? "Plank" : hits(x, new_y + 1) ? "Ground" : "Air";

        if(direction != 0)
        {
            player.set_horizontal_flip(direction > 0);   // The art faces left.
        }

        if(on != shown)
        {
            shown = on;
            text.clear();
            text_generator.generate(-110, -71, on, text);
        }

        bn::core::update();
    }
}
