#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_bg_palettes.h"
#include "bn_color.h"

int main()
{
    bn::core::init();

    constexpr bn::color shot_colors[] = { bn::color(31, 0, 0), bn::color(0, 31, 0), bn::color(31, 31, 31) };
    int shot_color = 0;
    int charge = 0;
    int flash_frames = 0;

    while(true)
    {
        // pressed: true on one frame only, so each press picks the next color once.
        if(bn::keypad::b_pressed())
        {
            shot_color = (shot_color + 1) % 3;
        }

        // held: true on every frame while A is down, so the charge grows each frame.
        if(bn::keypad::a_held() && charge < 31)
        {
            ++charge;
        }

        // released: true on the frame A goes up. Fire, longer after a longer charge.
        if(bn::keypad::a_released())
        {
            flash_frames = charge;
            charge = 0;
        }

        if(flash_frames > 0)
        {
            bn::bg_palettes::set_transparent_color(shot_colors[shot_color]);
            --flash_frames;
        }
        else
        {
            bn::bg_palettes::set_transparent_color(bn::color(0, 0, charge));
        }

        bn::core::update();
    }
}
