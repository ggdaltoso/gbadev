#include "bn_core.h"
#include "bn_bg_palettes.h"
#include "bn_color.h"

int main()
{
    bn::core::init();

    int frame = 0;

    while(true)
    {
        // Change the backdrop color once per second (every 60 frames).
        if(frame % 120 == 0)
        {
            bn::bg_palettes::set_transparent_color(bn::color(31, 0, 0));
        }
        else if(frame % 120 == 60)
        {
            bn::bg_palettes::set_transparent_color(bn::color(0, 0, 31));
        }

        ++frame;
        bn::core::update();
    }
}
