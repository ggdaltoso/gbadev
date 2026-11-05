#include "bn_core.h"
#include "bn_bgs_mosaic.h"
#include "bn_fixed_point.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprites_mosaic.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_world.h"
#include "bn_sprite_items_hero.h"

#include "font.h"

int main()
{
    bn::core::init();

    constexpr int half_transition = 24;
    constexpr bn::fixed_point spots[] = { { 0, 0 }, { 72, -40 }, { -80, 32 } };

    // Only what has mosaic enabled is affected, so the text stays sharp.
    bn::regular_bg_ptr world = bn::regular_bg_items::world.create_bg(0, 0);
    world.set_mosaic_enabled(true);
    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(spots[0]);
    hero.set_mosaic_enabled(true);

    bn::sprite_text_generator text_generator(hud::font);
    text_generator.set_center_alignment();
    bn::vector<bn::sprite_ptr, 4> hint_sprites;
    text_generator.generate(0, 70, "A: teleport", hint_sprites);

    int spot = 0;
    int frames_left = 0;

    while(true)
    {
        if(frames_left == 0 && bn::keypad::a_pressed())
        {
            frames_left = half_transition * 2;
        }

        if(frames_left > 0)
        {
            --frames_left;

            // The hero jumps at the middle, when the picture is at its blurriest.
            if(frames_left == half_transition)
            {
                spot = (spot + 1) % 3;
                hero.set_position(spots[spot]);
            }

            int frames_from_middle = bn::abs(frames_left - half_transition);
            bn::fixed stretch = 1 - bn::fixed(frames_from_middle) / half_transition;

            // Backgrounds and sprites have separate settings.
            bn::bgs_mosaic::set_stretch(stretch);
            bn::sprites_mosaic::set_stretch(stretch);
        }

        bn::core::update();
    }
}
