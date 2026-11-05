#include "bn_core.h"
#include "bn_affine_bg_ptr.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "bn_affine_bg_items_world.h"
#include "bn_sprite_items_hero.h"

#include "font.h"

int main()
{
    bn::core::init();

    bn::affine_bg_ptr world = bn::affine_bg_items::world.create_bg(0, 0);
    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(0, 0);

    bn::sprite_text_generator text_generator(hud::font);
    text_generator.set_center_alignment();
    bn::vector<bn::sprite_ptr, 8> hint_sprites;
    text_generator.generate(0, 70, "D-pad: turn, zoom  A: spin", hint_sprites);

    while(true)
    {
        int turn = bn::keypad::right_held() - bn::keypad::left_held();
        int zoom = bn::keypad::up_held() - bn::keypad::down_held();

        // "safe" brings any angle into the range [0, 360).
        world.set_rotation_angle_safe(world.rotation_angle() + turn);

        bn::fixed wanted_scale = world.horizontal_scale() + bn::fixed(zoom) / 64;
        world.set_scale(bn::clamp(wanted_scale, bn::fixed(0.5), bn::fixed(2)));

        if(bn::keypad::a_held())
        {
            hero.set_rotation_angle_safe(hero.rotation_angle() + 8);
        }

        bn::core::update();
    }
}
