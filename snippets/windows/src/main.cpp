#include "bn_core.h"
#include "bn_bg_palettes.h"
#include "bn_color.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_rect_window.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_window.h"

#include "bn_regular_bg_items_world.h"
#include "bn_sprite_items_coin.h"
#include "bn_sprite_items_hero.h"

int main()
{
    bn::core::init();

    // What's left where nothing is drawn.
    bn::bg_palettes::set_transparent_color(bn::color(0, 0, 0));

    bn::regular_bg_ptr world = bn::regular_bg_items::world.create_bg(0, 0);
    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(0, 0);

    bn::sprite_ptr coins[] = {
        bn::sprite_items::coin.create_sprite(-88, -48),
        bn::sprite_items::coin.create_sprite(72, -32),
        bn::sprite_items::coin.create_sprite(-40, 56),
    };

    // Outside every window, show nothing. Inside, everything stays on by default.
    bn::window outside = bn::window::outside();
    outside.set_show_bg(world, false);
    outside.set_show_sprites(false);

    bn::rect_window light = bn::rect_window::internal();
    int light_radius = 32;

    while(true)
    {
        int wanted_x = hero.x().integer() + (bn::keypad::right_held() - bn::keypad::left_held()) * 2;
        int wanted_y = hero.y().integer() + (bn::keypad::down_held() - bn::keypad::up_held()) * 2;
        int x = bn::clamp(wanted_x, -104, 104);
        int y = bn::clamp(wanted_y, -64, 64);
        hero.set_position(x, y);

        if(bn::keypad::a_pressed())
        {
            light_radius = light_radius == 32 ? 56 : 32;
        }

        // A window is set in screen coordinates, so it follows the hero by hand.
        light.set_boundaries(y - light_radius, x - light_radius, y + light_radius, x + light_radius);

        bn::core::update();
    }
}
