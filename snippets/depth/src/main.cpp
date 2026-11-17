#include "bn_core.h"
#include "bn_fixed_rect.h"
#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_ground.h"
#include "bn_sprite_items_flag.h"
#include "bn_sprite_items_hero.h"
#include "bn_sprite_items_stall.h"
#include "bn_sprite_items_statue.h"
#include "bn_sprite_items_villager.h"

// How far below each sprite's center its art touches the ground, measured from the art.
constexpr int hero_feet = 12;
constexpr int villager_feet = 10;
constexpr int tall_feet = 32;        // The statue and the flags: 64 pixels high, art at the bottom.
constexpr int stall_feet = 16;       // Each 32x32 piece; the stall's ground is under its bottom row.

// Higher z orders are drawn first, so they end up behind. Lower on the screen is closer,
// so the further down a thing stands, the lower its z order.
int z_order_at(bn::fixed ground_y)
{
    return -ground_y.round_integer();
}

void stand(bn::sprite_ptr& sprite, int feet)
{
    sprite.set_z_order(z_order_at(sprite.y() + feet));
}

// What the hero's feet can't walk into: the bottom of each thing, where it meets the ground.
constexpr bn::fixed_rect solids[] = {
    bn::fixed_rect(62, 17, 74, 34),     // The stall's counter, front to back.
    bn::fixed_rect(-58, 0, 26, 8),      // The statue's base.
    bn::fixed_rect(-112, 0, 4, 4),      // The flag poles.
    bn::fixed_rect(-32, 0, 4, 4),
    bn::fixed_rect(-40, 47, 10, 6),     // The villagers.
    bn::fixed_rect(20, 59, 10, 6),
};

bool blocked(bn::fixed_point feet)
{
    if(feet.x() < -116 || feet.x() > 116 || feet.y() < -78 || feet.y() > 78)
    {
        return true;
    }

    for(const bn::fixed_rect& solid : solids)
    {
        if(solid.contains(feet))
        {
            return true;
        }
    }

    return false;
}

int main()
{
    bn::core::init();

    bn::regular_bg_ptr ground = bn::regular_bg_items::ground.create_bg(0, 0);

    // Things that don't move get their z order once.
    bn::sprite_ptr statue = bn::sprite_items::statue.create_sprite(-60, -28);
    stand(statue, tall_feet);

    bn::sprite_ptr left_flag = bn::sprite_items::flag.create_sprite(-100, -30);
    bn::sprite_ptr right_flag = bn::sprite_items::flag.create_sprite(-20, -30);
    stand(left_flag, tall_feet);
    stand(right_flag, tall_feet);

    bn::sprite_ptr villager = bn::sprite_items::villager.create_sprite(-40, 40);
    bn::sprite_ptr other_villager = bn::sprite_items::villager.create_sprite(20, 52);
    other_villager.set_horizontal_flip(true);
    stand(villager, villager_feet);
    stand(other_villager, villager_feet);

    // The stall is 9 sprites, but one thing: every piece sorts by the stall's ground, not its own.
    bn::vector<bn::sprite_ptr, 9> stall;

    for(int index = 0; index < 9; ++index)
    {
        stall.push_back(bn::sprite_items::stall.create_sprite(28 + index % 3 * 32, -46 + index / 3 * 32, index));
    }

    for(bn::sprite_ptr& piece : stall)
    {
        piece.set_z_order(z_order_at(stall.back().y() + stall_feet));
    }

    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(0, 56);

    while(true)
    {
        int step_x = bn::keypad::right_held() - bn::keypad::left_held();
        int step_y = bn::keypad::down_held() - bn::keypad::up_held();
        bn::fixed_point feet = hero.position() + bn::fixed_point(0, hero_feet);

        if(! blocked(feet + bn::fixed_point(step_x, 0)))
        {
            hero.set_x(hero.x() + step_x);
        }

        if(! blocked(feet + bn::fixed_point(hero.x() - feet.x(), step_y)))
        {
            hero.set_y(hero.y() + step_y);
        }

        if(step_x != 0)
        {
            hero.set_horizontal_flip(step_x < 0);
        }

        // The hero moves, so the hero sorts again every frame.
        stand(hero, hero_feet);
        bn::core::update();
    }
}
