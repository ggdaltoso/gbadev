#include "bn_core.h"
#include "bn_regular_bg_ptr.h"

#include "bn_regular_bg_items_land.h"
#include "bn_regular_bg_items_water.h"

int main()
{
    bn::core::init();

    bn::regular_bg_ptr water = bn::regular_bg_items::water.create_bg(0, 0);
    bn::regular_bg_ptr land = bn::regular_bg_items::land.create_bg(0, 0);

    // Lower numbers are drawn in front. Without this, ties go to the newest background.
    water.set_priority(3);
    land.set_priority(2);

    while(true)
    {
        // The water flows down half a pixel per frame. The map wraps around every
        // 256 pixels, so y = 256 looks like y = 0: wrap y too, instead of letting it grow.
        bn::fixed y = water.y() + bn::fixed(0.5);
        water.set_y(y >= 256 ? y - 256 : y);

        bn::core::update();
    }
}
