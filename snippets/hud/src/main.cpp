#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_camera_ptr.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_format.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_world.h"

#include "font.h"

int main()
{
    bn::core::init();

    // The world moves with the camera; the text has no camera, so it stays on screen.
    bn::camera_ptr camera = bn::camera_ptr::create(0, 0);
    bn::regular_bg_ptr world = bn::regular_bg_items::world.create_bg(0, 0);
    world.set_camera(camera);

    bn::sprite_text_generator text_generator(hud::font);

    // Fixed text: generated once, kept alive by its vector.
    bn::vector<bn::sprite_ptr, 8> hint_sprites;
    text_generator.set_center_alignment();
    text_generator.generate(0, 70, "Arrows: look around", hint_sprites);

    // Changing text: the right edge stays at x = 112, so the numbers don't jump around.
    bn::vector<bn::sprite_ptr, 4> position_sprites;
    text_generator.set_right_alignment();
    bn::point shown(1, 1);  // anything but the first position, so the first frame draws it

    while(true)
    {
        bn::point move(bn::keypad::right_held() - bn::keypad::left_held(),
                       bn::keypad::down_held() - bn::keypad::up_held());
        int wanted_x = camera.x().integer() + move.x() * 2;
        int wanted_y = camera.y().integer() + move.y() * 2;

        // Half a screen from the 512x512 map's edges, as on the camera page.
        bn::point position(bn::clamp(wanted_x, -136, 136), bn::clamp(wanted_y, -176, 176));
        camera.set_position(position.x(), position.y());

        // Only when it changed: generating text every frame wastes CPU and video memory.
        if(position != shown)
        {
            shown = position;
            position_sprites.clear();
            text_generator.generate(112, -70, bn::format<12>("X {}", position.x()), position_sprites);
            text_generator.generate(112, -56, bn::format<12>("Y {}", position.y()), position_sprites);
        }

        bn::core::update();
    }
}
