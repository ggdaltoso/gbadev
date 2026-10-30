#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_camera_ptr.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_animate_actions.h"

#include "bn_regular_bg_items_world.h"
#include "bn_sprite_items_stump.h"

int main()
{
    bn::core::init();

    bn::camera_ptr camera = bn::camera_ptr::create(0, 0);

    // The map is 512x512 and centered on (0, 0), so it goes from -256 to 256.
    bn::regular_bg_ptr world = bn::regular_bg_items::world.create_bg(0, 0);
    world.set_camera(camera);

    // A world position: the stump stays at this spot of the map wherever the camera goes.
    bn::sprite_ptr stump = bn::sprite_items::stump.create_sprite(-136, 168);
    stump.set_camera(camera);
    bn::sprite_animate_action<4> snore = bn::create_sprite_animate_action_forever(
                stump, 16, bn::sprite_items::stump.tiles_item(), 0, 1, 2, 3);

    // The camera is the center of the screen, so it must stop half a screen
    // (120x80) before the map's edge, or the screen shows past it.
    constexpr bn::fixed max_x = 256 - 120;
    constexpr bn::fixed max_y = 256 - 80;
    bool gliding = false;

    while(true)
    {
        bn::fixed_point position = camera.position();
        bn::fixed_point move(bn::keypad::right_held() - bn::keypad::left_held(),
                             bn::keypad::down_held() - bn::keypad::up_held());

        if(bn::keypad::a_pressed())
        {
            gliding = true;
        }

        if(move != bn::fixed_point())
        {
            gliding = false;
            position += move * 2;
        }
        else if(gliding)
        {
            // Smooth follow: cover a tenth of the remaining distance each frame.
            position += (stump.position() - position) / 10;
        }

        camera.set_position(bn::clamp(position.x(), -max_x, max_x), bn::clamp(position.y(), -max_y, max_y));
        snore.update();
        bn::core::update();
    }
}
