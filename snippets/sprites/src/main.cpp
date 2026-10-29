#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_bg_palettes.h"
#include "bn_color.h"
#include "bn_math.h"
#include "bn_optional.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_animate_actions.h"

#include "bn_sprite_items_hero.h"

int main()
{
    bn::core::init();

    // Same color the sheet's shadow was blended onto (see snippets/tools/hero_sheet.py).
    bn::bg_palettes::set_transparent_color(bn::color(12, 20, 10));

    // Frame 0 is the hero standing; frames 1-4 are the sword swing.
    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(0, 0);
    bn::optional<bn::sprite_animate_action<4>> swing;

    while(true)
    {
        if(! swing)
        {
            if(bn::keypad::left_pressed())
            {
                hero.set_horizontal_flip(true);
            }
            else if(bn::keypad::right_pressed())
            {
                hero.set_horizontal_flip(false);
            }

            if(bn::keypad::a_pressed())
            {
                // Each frame is shown for 4 screen frames, then the action is done.
                swing = bn::create_sprite_animate_action_once(
                            hero, 4, bn::sprite_items::hero.tiles_item(), 1, 2, 3, 4);
            }
        }
        else
        {
            // Lunge half a pixel per frame: positions are bn::fixed, so the halves add up.
            bn::fixed step = hero.horizontal_flip() ? -0.5 : 0.5;
            hero.set_x(bn::clamp(hero.x() + step, bn::fixed(-104), bn::fixed(104)));
            swing->update();

            if(swing->done())
            {
                swing.reset();
                hero.set_tiles(bn::sprite_items::hero.tiles_item(), 0);
            }
        }

        bn::core::update();
    }
}
