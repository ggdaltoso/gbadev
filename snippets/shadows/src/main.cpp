#include "bn_blending.h"
#include "bn_core.h"
#include "bn_fixed_rect.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"

#include "bn_regular_bg_items_ground.h"
#include "bn_regular_bg_items_water.h"
#include "bn_sprite_items_bare_hero.h"
#include "bn_sprite_items_shadow.h"

#include "hero_walk.h"

// The hero's boots end 10 pixels below the sprite's center: that's where it touches the ground,
// so it's where the shadow goes and the line the reflection mirrors on.
constexpr int feet = 10;

// The open water, in screen pixels: the hole in the ground layer. The hero's feet stay out of it.
constexpr bn::fixed_rect pond(bn::fixed_point(-1, 24), bn::fixed_size(88, 56));

bool feet_in_pond(bn::fixed x, bn::fixed y)
{
    return pond.intersects(bn::fixed_rect(bn::fixed_point(x, y + feet - 2), bn::fixed_size(8, 4)));
}

int main()
{
    bn::core::init();

    // Two layers: the ground in front, with a hole where the pond is, and the water behind it.
    // A lower priority number is drawn in front.
    bn::regular_bg_ptr ground = bn::regular_bg_items::ground.create_bg(0, 0);
    ground.set_priority(1);
    bn::regular_bg_ptr water = bn::regular_bg_items::water.create_bg(0, 0);
    water.set_priority(2);

    // The reflection goes between them: in front of the water and behind the ground,
    // so the ground hides all of it but what's over the hole.
    // bare_hero is the shared hero without the shadow drawn into its art: this snippet draws its own.
    bn::sprite_ptr reflection = bn::sprite_items::bare_hero.create_sprite(0, 0);
    reflection.set_bg_priority(2);
    reflection.set_vertical_flip(true);
    reflection.set_blending_enabled(true);
    hero_walk reflection_walk(bn::sprite_items::bare_hero);

    // The shadow and the hero are on the ground. The shadow is behind the hero: a higher z order.
    bn::sprite_ptr shadow = bn::sprite_items::shadow.create_sprite(0, 0);
    shadow.set_bg_priority(1);
    shadow.set_z_order(1);
    shadow.set_blending_enabled(true);

    bn::sprite_ptr hero = bn::sprite_items::bare_hero.create_sprite(-80, -40);
    hero.set_bg_priority(1);
    hero_walk walk(bn::sprite_items::bare_hero);

    // One alpha for everything blended: the shadow and the reflection are both half see-through.
    bn::blending::set_transparency_alpha(0.5);

    while(true)
    {
        int step_x = bn::keypad::right_held() - bn::keypad::left_held();
        int step_y = bn::keypad::down_held() - bn::keypad::up_held();
        bn::fixed x = bn::clamp(hero.x() + step_x, bn::fixed(-112), bn::fixed(112));
        bn::fixed y = bn::clamp(hero.y() + step_y, bn::fixed(-72), bn::fixed(64));

        if(! feet_in_pond(x, hero.y()))
        {
            hero.set_x(x);
        }

        if(! feet_in_pond(hero.x(), y))
        {
            hero.set_y(y);
        }

        // The same steps give the reflection the same frame and the same facing as the hero.
        walk.update(hero, step_x, step_y);
        reflection_walk.update(reflection, step_x, step_y);

        // Mirrored on the feet: as far below them as the sprite's center is above them.
        reflection.set_position(hero.x(), hero.y() + feet * 2);
        shadow.set_position(hero.x(), hero.y() + feet - 1);

        bn::core::update();
    }
}
