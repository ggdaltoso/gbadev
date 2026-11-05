#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_music.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_animate_actions.h"
#include "bn_sprite_text_generator.h"
#include "bn_format.h"
#include "bn_vector.h"

#include "bn_music_items.h"
#include "bn_regular_bg_items_world.h"
#include "bn_sound_items.h"
#include "bn_sprite_items_coin.h"

#include "font.h"

int main()
{
    bn::core::init();

    bn::regular_bg_ptr world = bn::regular_bg_items::world.create_bg(0, 0);
    bn::sprite_ptr coin = bn::sprite_items::coin.create_sprite(0, 0);
    bn::sprite_animate_action<4> spin = bn::create_sprite_animate_action_forever(
                coin, 6, bn::sprite_items::coin.tiles_item(), 0, 1, 2, 3);

    bn::sprite_text_generator text_generator(hud::font);
    text_generator.set_center_alignment();

    bn::vector<bn::sprite_ptr, 8> hint_sprites;
    text_generator.generate(0, 70, "A: coin  B: stop  Up, Down: volume", hint_sprites);

    // In steps of 1 to 10, so that the screen shows a round number.
    int volume_steps = 7;
    bn::music_items::song.play(bn::fixed(volume_steps) / 10);

    bn::vector<bn::sprite_ptr, 8> status_sprites;
    bool status_changed = true;

    while(true)
    {
        // Plays over the music, on a channel of its own.
        if(bn::keypad::a_pressed())
        {
            bn::sound_items::coin.play();
        }

        if(bn::keypad::b_pressed())
        {
            // Stops and starts over: a recording in a module stays silent for a while after resuming.
            if(bn::music::playing())
            {
                bn::music::stop();
            }
            else
            {
                bn::music_items::song.play(bn::fixed(volume_steps) / 10);
            }

            status_changed = true;
        }

        int wanted_steps = volume_steps + bn::keypad::up_pressed() - bn::keypad::down_pressed();
        int new_steps = bn::clamp(wanted_steps, 0, 10);

        if(new_steps != volume_steps)
        {
            volume_steps = new_steps;

            if(bn::music::playing())
            {
                bn::music::set_volume(bn::fixed(volume_steps) / 10);
            }

            status_changed = true;
        }

        if(status_changed)
        {
            status_changed = false;
            status_sprites.clear();
            text_generator.generate(0, -70, bn::format<32>("{}   Volume {}", bn::music::playing() ? "Playing" : "Stopped",
                                                           volume_steps), status_sprites);
        }

        spin.update();
        bn::core::update();
    }
}
