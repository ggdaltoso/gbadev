#include "bn_core.h"
#include "bn_array.h"
#include "bn_display.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_regular_bg_position_hbe_ptr.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_world.h"
#include "bn_sprite_items_hero.h"

#include "font.h"

int main()
{
    bn::core::init();

    bn::regular_bg_ptr world = bn::regular_bg_items::world.create_bg(0, 0);
    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(0, 0);

    bn::sprite_text_generator text_generator(hud::font);
    text_generator.set_center_alignment();
    bn::vector<bn::sprite_ptr, 4> hint_sprites;
    text_generator.generate(0, 70, "A: more waves", hint_sprites);

    // One value per screen line. The effect keeps a reference to it, so it must outlive the effect.
    bn::array<bn::fixed, bn::display::height()> line_offsets;
    bn::regular_bg_position_hbe_ptr waves =
            bn::regular_bg_position_hbe_ptr::create_horizontal(world, line_offsets);

    int wave_size = 2;
    int first_line_angle = 0;

    while(true)
    {
        if(bn::keypad::a_pressed())
        {
            wave_size = wave_size == 2 ? 8 : 2;
        }

        first_line_angle = (first_line_angle + 4) % 360;

        for(int line = 0; line < bn::display::height(); ++line)
        {
            int line_angle = (first_line_angle + line * 6) % 360;
            line_offsets[line] = bn::degrees_lut_sin(line_angle) * wave_size;
        }

        // The array changed, so the effect has to read it again.
        waves.reload_deltas_ref();

        bn::core::update();
    }
}
