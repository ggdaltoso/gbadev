#include "bn_core.h"
#include "bn_blending.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_format.h"
#include "bn_string_view.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_world.h"
#include "bn_sprite_items_coin.h"
#include "bn_sprite_items_hero.h"

#include "font.h"

int main()
{
    bn::core::init();

    bn::regular_bg_ptr world = bn::regular_bg_items::world.create_bg(0, 0);

    // Only what has blending enabled is affected: the coin next to the hero is the control.
    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(-16, 0);
    hero.set_blending_enabled(true);
    bn::sprite_ptr coin = bn::sprite_items::coin.create_sprite(24, 8);

    bn::sprite_text_generator text_generator(hud::font);
    text_generator.set_center_alignment();

    bn::vector<bn::sprite_ptr, 8> hint_sprites;
    text_generator.generate(0, 70, "A: mode  Up, Down: alpha", hint_sprites);

    constexpr bn::string_view mode_names[] = { "Transparency", "Intensity", "Fade" };
    int mode = 0;
    int alpha_steps = 5;
    bn::vector<bn::sprite_ptr, 8> status_sprites;
    bool changed = true;

    while(true)
    {
        if(bn::keypad::a_pressed())
        {
            mode = (mode + 1) % 3;
            changed = true;
        }

        int wanted_steps = alpha_steps + bn::keypad::up_pressed() - bn::keypad::down_pressed();
        int new_steps = bn::clamp(wanted_steps, 0, 10);

        if(new_steps != alpha_steps)
        {
            alpha_steps = new_steps;
            changed = true;
        }

        if(changed)
        {
            changed = false;
            bn::fixed alpha = bn::fixed(alpha_steps) / 10;

            // Fade can't be on with the other two, so turn all of them off first.
            bn::blending::set_transparency_alpha(1);
            bn::blending::set_intensity_alpha(0);
            bn::blending::set_fade_alpha(0);

            if(mode == 0)
            {
                bn::blending::set_transparency_alpha(alpha);
            }
            else if(mode == 1)
            {
                bn::blending::set_intensity_alpha(alpha);
            }
            else
            {
                bn::blending::set_fade_alpha(alpha);
            }

            status_sprites.clear();
            text_generator.generate(0, -70, bn::format<32>("{}  {}", mode_names[mode], alpha), status_sprites);
        }

        bn::core::update();
    }
}
