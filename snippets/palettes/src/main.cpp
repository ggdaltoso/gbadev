#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"
#include "bn_bg_palette_ptr.h"
#include "bn_bg_palette_actions.h"

#include "bn_regular_bg_items_river.h"

int main()
{
    bn::core::init();

    bn::regular_bg_ptr river = bn::regular_bg_items::river.create_bg(0, 0);

    // The water is painted with colors 1 to 9, in stripes: 1, 2, ..., 9, 1, 2...
    // Rotating those nine colors moves every stripe at once, without touching a tile.
    bn::bg_palette_ptr palette = river.palette();
    palette.set_rotate_range(1, 9);

    // Every 3 frames, shift the colors one place.
    int delta = 1;
    bn::bg_palette_rotate_by_action flow(palette, 3, delta);

    while(true)
    {
        if(bn::keypad::a_pressed())
        {
            // Upstream: a new action that rotates the other way, from where the colors are now.
            delta = -delta;
            flow = bn::bg_palette_rotate_by_action(palette, 3, delta);
        }

        flow.update();
        bn::core::update();
    }
}
