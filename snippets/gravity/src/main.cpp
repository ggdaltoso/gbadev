#include "bn_core.h"
#include "bn_fixed.h"
#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_sstream.h"
#include "bn_string.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_level.h"
#include "bn_sprite_items_player.h"

#include "font.h"

// Pixels per frame, and pixels per frame per frame for the gravity.
constexpr bn::fixed gravity = 0.25;
constexpr bn::fixed jump_speed = -5;
constexpr bn::fixed cut_speed = -1.5;   // Letting go of A while rising slows the rise to this.
constexpr bn::fixed max_fall_speed = 4;
constexpr bn::fixed walk_speed = 1.5;
constexpr int feet = 12;   // From the sprite's center down to its feet.

// Platforms you land on from above. They must match PLATFORMS in snippets/tools/platformer_assets.py.
struct platform
{
    int left;
    int right;
    int top;
};

constexpr platform platforms[] = {
    { -120, 120, 44 },   // The ground.
    { -110, -56, 16 },
    { -74, -20, -10 },
    { -38, 16, -36 },
};

int main()
{
    bn::core::init();

    bn::regular_bg_ptr level = bn::regular_bg_items::level.create_bg(0, 0);
    bn::sprite_ptr player = bn::sprite_items::player.create_sprite(-100, 44 - feet);
    bn::fixed velocity_y = 0;
    bool on_ground = true;
    int walk_frames = 0;

    // The top of the screen shows how high the last jump went: from the feet at takeoff to the highest point.
    bool jumping = false;
    bn::fixed takeoff_feet = 0;
    bn::fixed highest_feet = 0;

    bn::sprite_text_generator text_generator(hud::font);
    text_generator.set_center_alignment();
    bn::vector<bn::sprite_ptr, 4> text_sprites;
    text_generator.generate(0, -70, "A: jump", text_sprites);

    while(true)
    {
        int direction = bn::keypad::right_held() - bn::keypad::left_held();
        bn::fixed x = bn::clamp(player.x() + direction * walk_speed, bn::fixed(-110), bn::fixed(110));

        // Jumping is only allowed from the ground.
        if(bn::keypad::a_pressed() && on_ground)
        {
            velocity_y = jump_speed;
            jumping = true;
            takeoff_feet = player.y() + feet;
            highest_feet = takeoff_feet;
        }

        // A short press makes a short jump: once A is up, the rise can't be faster than cut_speed.
        if(! bn::keypad::a_held() && velocity_y < cut_speed)
        {
            velocity_y = cut_speed;
        }

        // Gravity changes the velocity, never the position directly. The fall has a top speed.
        velocity_y = bn::min(velocity_y + gravity, max_fall_speed);

        bn::fixed old_feet = player.y() + feet;
        bn::fixed new_feet = old_feet + velocity_y;
        on_ground = false;

        // Land when the feet go from above a platform's top to on or below it in this frame.
        if(velocity_y >= 0)
        {
            for(const platform& under : platforms)
            {
                if(x >= under.left && x <= under.right && old_feet <= under.top && new_feet >= under.top)
                {
                    new_feet = under.top;
                    velocity_y = 0;
                    on_ground = true;
                    break;
                }
            }
        }

        player.set_position(x, new_feet - feet);
        highest_feet = bn::min(highest_feet, new_feet);

        // The art faces left, so facing right is the flipped one.
        if(direction != 0)
        {
            player.set_horizontal_flip(direction > 0);
        }

        walk_frames = on_ground && direction != 0 ? walk_frames + 1 : 0;
        int frame = on_ground ? walk_frames / 8 % 2 : 1;
        player.set_tiles(bn::sprite_items::player.tiles_item(), frame);

        if(jumping && on_ground)
        {
            jumping = false;
            text_sprites.clear();
            bn::string<24> text;
            bn::ostringstream stream(text);
            stream.set_precision(4);   // Digits before and after the point together: "21.25", not "21.25000".
            stream << "Last jump: " << takeoff_feet - highest_feet << " px";
            text_generator.generate(0, -70, text, text_sprites);
        }

        bn::core::update();
    }
}
