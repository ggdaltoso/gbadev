#include "bn_algorithm.h"
#include "bn_core.h"
#include "bn_fixed.h"
#include "bn_format.h"
#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_level.h"
#include "bn_sprite_items_player.h"

#include "font.h"

// The two kinds of forgiveness, in frames and pixels. Small numbers: the player should never notice them.
constexpr int coyote_frames = 6;   // Still allowed to jump this long after walking off a ledge.
constexpr int corner_nudge = 4;    // A head this close to a block's corner slides past it instead of bumping.

constexpr bn::fixed gravity = 0.25;
constexpr bn::fixed jump_speed = -5.5;
constexpr bn::fixed max_fall_speed = 4;
constexpr bn::fixed walk_speed = 1.5;

// The level, one character per 18x18 cell: '#' is solid. snippets/tools/forgiving_assets.py draws it from here.
constexpr int cell = 18;
constexpr int columns = 14;
constexpr int rows = 9;
constexpr int origin_x = -126;   // Screen position of the level's top-left corner.
constexpr int origin_y = -81;

constexpr const char* level[rows] = {
    "..............",
    "..............",
    "..............",
    "...........##.",
    "..............",
    "..............",
    "#####...######",
    "..............",
    "##############",
};

bool solid(int x, int y)
{
    int column = (x - origin_x) / cell;
    int row = (y - origin_y) / cell;

    if(x < origin_x || column >= columns)
    {
        return true;   // Walls at both sides.
    }

    return y >= origin_y && row < rows && level[row][column] == '#';
}

// The player's box, from the sprite's center: 7 pixels to each side, 10 up to the head, 12 down to the feet.
bool hits(bn::fixed x, bn::fixed y)
{
    int left = (x - 7).floor_integer();
    int right = (x + 6).floor_integer();
    int top = (y - 10).floor_integer();
    int bottom = (y + 11).floor_integer();
    return solid(left, top) || solid(right, top) || solid(left, bottom) || solid(right, bottom);
}

int main()
{
    bn::core::init();

    bn::regular_bg_ptr background = bn::regular_bg_items::level.create_bg(0, 0);
    bn::sprite_ptr player = bn::sprite_items::player.create_sprite(-90, 27 - 12);
    bn::sprite_text_generator text_generator(hud::font);
    bn::vector<bn::sprite_ptr, 8> text;

    bn::fixed velocity_y = 0;
    int coyote = 0;          // Frames left in which a jump still works, counting down once off the ground.
    int last_nudge = 0;      // How far the last corner pushed the player aside, to show it.
    bn::string<32> shown;

    while(true)
    {
        bn::fixed x = player.x();
        bn::fixed y = player.y();

        // Jump while on the ground, or a few frames after leaving it. Jumping spends it: no second jump in the air.
        if(bn::keypad::a_pressed() && coyote > 0)
        {
            velocity_y = jump_speed;
            coyote = 0;
        }

        int direction = bn::keypad::right_held() - bn::keypad::left_held();
        bn::fixed new_x = x + direction * walk_speed;

        if(! hits(new_x, y))
        {
            x = new_x;
        }

        velocity_y = bn::min(velocity_y + gravity, max_fall_speed);
        bn::fixed new_y = y + velocity_y;

        if(hits(x, new_y))
        {
            bool slid = false;

            // Rising into a block: if a step of a few pixels to either side clears it, take the step and keep rising.
            for(int nudge = 1; velocity_y < 0 && nudge <= corner_nudge && ! slid; ++nudge)
            {
                for(int side : { -1, 1 })
                {
                    if(! slid && ! hits(x + side * nudge, new_y))
                    {
                        x += side * nudge;
                        last_nudge = nudge;
                        slid = true;
                    }
                }
            }

            if(! slid)
            {
                if(velocity_y < 0)
                {
                    int row = ((new_y - 10).floor_integer() - origin_y) / cell;
                    new_y = origin_y + (row + 1) * cell + 10;   // Head against the block's bottom.
                }
                else
                {
                    int row = ((new_y + 11).floor_integer() - origin_y) / cell;
                    new_y = origin_y + row * cell - 12;         // Feet on the block's top.
                }

                velocity_y = 0;
            }
        }

        player.set_position(x, new_y);

        // On the ground: the pixel under the feet is solid. Off it, the coyote frames count down.
        if(hits(x, new_y + 1))
        {
            coyote = coyote_frames;
        }
        else if(coyote > 0)
        {
            --coyote;
        }

        if(direction != 0)
        {
            player.set_horizontal_flip(direction > 0);   // The art faces left.
        }

        bn::string<32> wanted = bn::format<32>("Coyote {}   Corner nudge {}", coyote, last_nudge);

        if(wanted != shown)
        {
            shown = wanted;
            text.clear();
            text_generator.generate(-110, -71, shown, text);
        }

        bn::core::update();
    }
}
