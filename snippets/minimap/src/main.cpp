#include "bn_camera_ptr.h"
#include "bn_core.h"
#include "bn_display.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"

#include "bn_regular_bg_items_world.h"
#include "bn_sprite_items_dot.h"
#include "bn_sprite_items_hero.h"
#include "bn_sprite_items_minimap.h"
#include "bn_sprite_items_view.h"

// The minimap is the 1024x1024 world drawn at 64x64: one minimap pixel for every 16 world pixels.
constexpr int minimap_scale = 16;

int main()
{
    bn::core::init();

    bn::camera_ptr camera = bn::camera_ptr::create(0, 0);
    bn::regular_bg_ptr world = bn::regular_bg_items::world.create_bg(0, 0);
    world.set_camera(camera);

    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(0, 0);
    hero.set_camera(camera);

    // No camera: the minimap and its markers stay on the screen, in the top right corner.
    // The markers are created after the minimap, so they're drawn on top of it.
    constexpr bn::fixed_point minimap_center(bn::display::width() / 2 - 36, -bn::display::height() / 2 + 36);
    bn::sprite_ptr minimap = bn::sprite_items::minimap.create_sprite(minimap_center);
    bn::sprite_ptr view = bn::sprite_items::view.create_sprite(minimap_center);
    bn::sprite_ptr dot = bn::sprite_items::dot.create_sprite(minimap_center);

    bn::fixed half_width = world.dimensions().width() / 2;
    bn::fixed half_height = world.dimensions().height() / 2;
    bn::fixed camera_x = half_width - bn::display::width() / 2;
    bn::fixed camera_y = half_height - bn::display::height() / 2;
    int frame = 0;

    while(true)
    {
        int speed = bn::keypad::b_held() ? 4 : 1;
        int step_x = bn::keypad::right_held() - bn::keypad::left_held();
        int step_y = bn::keypad::down_held() - bn::keypad::up_held();
        hero.set_position(bn::clamp(hero.x() + step_x * speed, -half_width + 8, half_width - 8),
                          bn::clamp(hero.y() + step_y * speed, -half_height + 8, half_height - 16));

        if(step_x != 0)
        {
            hero.set_horizontal_flip(step_x < 0);
        }

        camera.set_position(bn::clamp(hero.x(), -camera_x, camera_x), bn::clamp(hero.y(), -camera_y, camera_y));

        // The world and the minimap are both centered on (0, 0), so a world position divided by
        // the scale is the same spot on the minimap, measured from its center.
        dot.set_position(minimap_center + hero.position() / minimap_scale);
        view.set_position(minimap_center + camera.position() / minimap_scale);

        // The dot blinks, so it can't be mistaken for a flower.
        ++frame;
        dot.set_visible(frame % 32 < 24);

        bn::core::update();
    }
}
