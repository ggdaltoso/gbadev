#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_sram.h"
#include "bn_format.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_world.h"
#include "bn_sprite_items_coin.h"
#include "bn_sprite_items_hero.h"

#include "font.h"

// Only plain values: SRAM stores these bytes, and a pointer wouldn't mean anything after a restart.
struct save_data
{
    int format;
    int x;
    int y;
    int times_saved;
};

// Change it when save_data changes, so that an old save isn't read with the new layout.
constexpr int save_format = 0x53415631;

int main()
{
    bn::core::init();

    bn::regular_bg_ptr world = bn::regular_bg_items::world.create_bg(0, 0);

    save_data save;
    bn::sram::read(save);

    // Without the right format this isn't our save: it's a new cart, or an old layout.
    if(save.format != save_format)
    {
        save = { save_format, 0, 0, 0 };
    }

    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(save.x, save.y);

    // The coin marks the saved spot, which is where the hero comes back to.
    bn::sprite_ptr checkpoint = bn::sprite_items::coin.create_sprite(save.x, save.y);
    checkpoint.set_visible(save.times_saved > 0);

    bn::sprite_text_generator text_generator(hud::font);
    text_generator.set_center_alignment();

    bn::vector<bn::sprite_ptr, 8> hint_sprites;
    text_generator.generate(0, 70, "A: save  B: erase  START: restart", hint_sprites);

    bn::vector<bn::sprite_ptr, 4> count_sprites;
    text_generator.generate(0, -70, bn::format<16>("Saves: {}", save.times_saved), count_sprites);

    while(true)
    {
        int wanted_x = hero.x().integer() + (bn::keypad::right_held() - bn::keypad::left_held()) * 2;
        int wanted_y = hero.y().integer() + (bn::keypad::down_held() - bn::keypad::up_held()) * 2;
        hero.set_position(bn::clamp(wanted_x, -104, 104), bn::clamp(wanted_y, -64, 64));

        if(bn::keypad::a_pressed())
        {
            save.x = hero.x().integer();
            save.y = hero.y().integer();
            ++save.times_saved;

            // Once per press, never every frame.
            bn::sram::write(save);

            checkpoint.set_position(hero.position());
            checkpoint.set_visible(true);
            count_sprites.clear();
            text_generator.generate(0, -70, bn::format<16>("Saves: {}", save.times_saved), count_sprites);
        }

        // Wiping the format is enough: the next start won't recognize the save.
        if(bn::keypad::b_pressed())
        {
            bn::sram::clear(sizeof(save_data));
            bn::core::reset();
        }

        // Back to the start of main(), as when turning it on. SRAM keeps its bytes.
        if(bn::keypad::start_pressed())
        {
            bn::core::reset();
        }

        bn::core::update();
    }
}
