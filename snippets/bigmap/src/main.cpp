#include "bn_camera_ptr.h"
#include "bn_core.h"
#include "bn_display.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"

#include "bn_regular_bg_items_world.h"
#include "bn_sprite_items_hero.h"

#include "hero_walk.h"

int main()
{
    bn::core::init();

    bn::camera_ptr camera = bn::camera_ptr::create(0, 0);

    // 1024x1024: too big for the hardware, so Butano keeps only the part around the camera in
    // video memory and copies in each new column or row of 8x8 cells as the camera reaches it.
    bn::regular_bg_ptr world = bn::regular_bg_items::world.create_bg(0, 0);
    world.set_camera(camera);

    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(0, 0);
    hero.set_camera(camera);
    hero_walk walk;

    // The map is centered on (0, 0). Its size comes from the background, in pixels.
    bn::fixed half_width = world.dimensions().width() / 2;
    bn::fixed half_height = world.dimensions().height() / 2;
    bn::fixed camera_x = half_width - bn::display::width() / 2;
    bn::fixed camera_y = half_height - bn::display::height() / 2;

    while(true)
    {
        // Hold B to run: 4 pixels a frame still crosses at most one cell per frame.
        int speed = bn::keypad::b_held() ? 4 : 1;
        int step_x = bn::keypad::right_held() - bn::keypad::left_held();
        int step_y = bn::keypad::down_held() - bn::keypad::up_held();
        hero.set_position(bn::clamp(hero.x() + step_x * speed, -half_width + 8, half_width - 8),
                          bn::clamp(hero.y() + step_y * speed, -half_height + 8, half_height - 16));

        walk.update(hero, step_x, step_y, speed);

        camera.set_position(bn::clamp(hero.x(), -camera_x, camera_x), bn::clamp(hero.y(), -camera_y, camera_y));
        bn::core::update();
    }
}
