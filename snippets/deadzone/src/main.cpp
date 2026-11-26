#include "bn_algorithm.h"
#include "bn_camera_ptr.h"
#include "bn_core.h"
#include "bn_display.h"
#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_world.h"
#include "bn_sprite_items_corner.h"
#include "bn_sprite_items_focus.h"

#include "hero_walk.h"

constexpr int speed = 2;
constexpr bn::fixed zone_x = 32;   // Half the dead zone's width: the focus moves this far before the camera does.
constexpr bn::fixed zone_y = 20;
constexpr bn::fixed lead_x = 48;   // How far ahead of the hero the focus goes.
constexpr bn::fixed lead_y = 28;
constexpr int lead_steps = 16;     // The lead covers 1/16 of what's left each frame, so turning around is a slide.

int main()
{
    bn::core::init();

    bn::camera_ptr camera = bn::camera_ptr::create(0, 0);
    bn::regular_bg_ptr world = bn::regular_bg_items::world.create_bg(0, 0);
    world.set_camera(camera);
    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(0, 0);
    hero.set_camera(camera);
    hero_walk walk;

    // Markers so the dead zone can be seen: its corners stay on the screen, the focus lives in the world.
    bn::sprite_ptr focus_marker = bn::sprite_items::focus.create_sprite(0, 0);
    focus_marker.set_camera(camera);
    bn::vector<bn::sprite_ptr, 4> corners;

    for(int corner = 0; corner < 4; ++corner)
    {
        bool right = corner % 2;
        bool bottom = corner / 2;
        bn::sprite_ptr sprite = bn::sprite_items::corner.create_sprite(right ? zone_x - 4 : -zone_x + 4,
                                                                       bottom ? zone_y - 4 : -zone_y + 4);
        sprite.set_horizontal_flip(right);
        sprite.set_vertical_flip(bottom);
        corners.push_back(sprite);
    }

    bn::fixed half_width = world.dimensions().width() / 2;
    bn::fixed half_height = world.dimensions().height() / 2;
    bn::fixed camera_x = half_width - bn::display::width() / 2;
    bn::fixed camera_y = half_height - bn::display::height() / 2;

    bn::fixed_point wanted_lead;   // Where the lead is heading: ahead of the last way the hero walked.
    bn::fixed_point lead;

    while(true)
    {
        int step_x = bn::keypad::right_held() - bn::keypad::left_held();
        int step_y = bn::keypad::down_held() - bn::keypad::up_held();
        hero.set_position(bn::clamp(hero.x() + step_x * speed, -half_width + 8, half_width - 8),
                          bn::clamp(hero.y() + step_y * speed, -half_height + 8, half_height - 16));
        walk.update(hero, step_x, step_y, speed);

        // Look-ahead: the focus slides ahead of the hero. Standing still keeps the last lead.
        if(step_x != 0 || step_y != 0)
        {
            wanted_lead = bn::fixed_point(step_x * lead_x, step_y * lead_y);
        }

        lead += (wanted_lead - lead) / lead_steps;
        bn::fixed_point focus = hero.position() + lead;
        focus_marker.set_position(focus);

        // Dead zone: the camera moves only as far as it must to keep the focus inside the box around it.
        bn::fixed x = bn::clamp(camera.x(), focus.x() - zone_x, focus.x() + zone_x);
        bn::fixed y = bn::clamp(camera.y(), focus.y() - zone_y, focus.y() + zone_y);

        // The map's edges come last, so they always win.
        camera.set_position(bn::clamp(x, -camera_x, camera_x), bn::clamp(y, -camera_y, camera_y));
        bn::core::update();
    }
}
