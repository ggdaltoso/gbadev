#include "bn_core.h"
#include "bn_array.h"
#include "bn_color.h"
#include "bn_display.h"
#include "bn_hdma.h"
#include "bn_keypad.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "bn_sprite_items_hero.h"

#include "font.h"

int main()
{
    bn::core::init();

    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(0, 24);

    bn::sprite_text_generator text_generator(hud::font);
    text_generator.set_center_alignment();
    bn::vector<bn::sprite_ptr, 4> hint_sprites;
    text_generator.generate(0, 70, "A: gradient on, off", hint_sprites);

    // One color per screen line: a sky from deep blue to orange.
    // HDMA reads this array while it runs, so it must stay alive that long.
    bn::array<uint16_t, bn::display::height()> sky;

    for(int line = 0; line < bn::display::height(); ++line)
    {
        // HDMA copies after a line is drawn, so a value shows up on the next line.
        int shown_line = (line + 1) % bn::display::height();
        int red = 2 + shown_line * 29 / 159;
        int green = 6 + shown_line * 12 / 159;
        int blue = 24 - shown_line * 18 / 159;
        sky[line] = uint16_t(bn::color(red, green, blue).data());
    }

    // The first color of palette memory is the backdrop, what shows where nothing is drawn.
    uint16_t& backdrop = *reinterpret_cast<uint16_t*>(0x05000000);
    bn::hdma::start(sky, backdrop);

    while(true)
    {
        if(bn::keypad::a_pressed())
        {
            if(bn::hdma::running())
            {
                bn::hdma::stop();
            }
            else
            {
                bn::hdma::start(sky, backdrop);
            }
        }

        bn::core::update();
    }
}
