#include "bn_camera_ptr.h"
#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_optional.h"
#include "bn_random.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_animate_actions.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_world.h"
#include "bn_sprite_items_hero.h"
#include "bn_sprite_items_stump.h"

#include "font.h"

// Each hit adds trauma, and trauma drains a little every frame. The shake is trauma squared:
// a single hit is a nudge, a few hits in a row are a jolt, and the end fades out softly.
constexpr bn::fixed trauma_per_hit = 0.5;
constexpr bn::fixed trauma_drain = bn::fixed(1) / 60;
constexpr int max_offset = 8;

// The camera is the center of the screen, so it stops half a screen before the map's edge.
constexpr bn::fixed max_x = 256 - 120;
constexpr bn::fixed max_y = 256 - 80;

int main()
{
    bn::core::init();

    bn::camera_ptr camera = bn::camera_ptr::create(0, 0);
    bn::regular_bg_ptr world = bn::regular_bg_items::world.create_bg(0, 0);
    world.set_camera(camera);

    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(0, 0);
    hero.set_camera(camera);
    bn::optional<bn::sprite_animate_action<4>> swing;

    bn::sprite_ptr stump = bn::sprite_items::stump.create_sprite(40, 0);
    stump.set_camera(camera);
    bn::sprite_animate_action<4> snore = bn::create_sprite_animate_action_forever(
                stump, 16, bn::sprite_items::stump.tiles_item(), 0, 1, 2, 3);

    // The hint isn't on the camera, so it stays still while the world shakes.
    bn::sprite_text_generator text_generator(hud::font);
    text_generator.set_center_alignment();
    bn::vector<bn::sprite_ptr, 4> hint_sprites;
    text_generator.generate(0, 70, "A: swing", hint_sprites);

    bn::random shake_random;
    bn::fixed trauma = 0;
    bn::fixed_point focus = hero.position();  // Where the camera looks, without the shake.

    while(true)
    {
        if(swing)
        {
            swing->update();

            if(swing->done())
            {
                swing.reset();
                hero.set_tiles(bn::sprite_items::hero.tiles_item(), 0);
            }
        }
        else
        {
            int step_x = bn::keypad::right_held() - bn::keypad::left_held();
            int step_y = bn::keypad::down_held() - bn::keypad::up_held();
            hero.set_position(hero.position() + bn::fixed_point(step_x, step_y));

            if(step_x != 0)
            {
                hero.set_horizontal_flip(step_x < 0);
            }

            if(bn::keypad::a_pressed())
            {
                swing = bn::create_sprite_animate_action_once(
                            hero, 4, bn::sprite_items::hero.tiles_item(), 1, 2, 3, 4);

                // The sword reaches about 28 pixels in front of the hero.
                bn::fixed_point gap = stump.position() - hero.position();
                bn::fixed reach = hero.horizontal_flip() ? -gap.x() : gap.x();

                if(reach > 0 && reach < 28 && bn::abs(gap.y()) < 16)
                {
                    trauma = bn::min(trauma + trauma_per_hit, bn::fixed(1));
                }
            }
        }

        // Follow the hero smoothly and keep the view inside the map...
        focus += (hero.position() - focus) / 8;
        bn::fixed_point view(bn::clamp(focus.x(), -max_x, max_x), bn::clamp(focus.y(), -max_y, max_y));

        // ...then add the shake on top, without storing it anywhere.
        bn::fixed strength = trauma.safe_multiplication(trauma) * max_offset;
        bn::fixed_point offset(shake_random.get_fixed(-1, 1).safe_multiplication(strength),
                               shake_random.get_fixed(-1, 1).safe_multiplication(strength));
        camera.set_position(view + offset);

        trauma = bn::max(trauma - trauma_drain, bn::fixed(0));
        snore.update();
        bn::core::update();
    }
}
