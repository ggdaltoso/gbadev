#include "bn_array.h"
#include "bn_camera_ptr.h"
#include "bn_core.h"
#include "bn_display.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_music.h"
#include "bn_music_item.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "bn_music_items.h"
#include "bn_regular_bg_items_world.h"
#include "bn_sprite_items_hero.h"

#include "font.h"
#include "hero_walk.h"

// A rectangle of the world and the song that plays in it. World pixels: the map goes from -512 to 512.
struct region
{
    int left;
    int top;
    int right;
    int bottom;
    const char* name;
    bn::music_item song;
};

// The first region that holds the hero wins; the last one holds everything.
constexpr bn::array<region, 3> regions = {
    region{ -480, -480, -16, -96, "Forest", bn::music_items::forest },
    region{ -448, 112, 48, 488, "Village", bn::music_items::village },
    region{ -512, -512, 512, 512, "Fields", bn::music_items::field }
};

constexpr bn::fixed fade_step = bn::fixed(1) / 30;   // From full volume to silence in half a second.

const region& region_at(bn::fixed_point point)
{
    for(const region& candidate : regions)
    {
        if(point.x() >= candidate.left && point.x() < candidate.right &&
           point.y() >= candidate.top && point.y() < candidate.bottom)
        {
            return candidate;
        }
    }

    return regions.back();
}

int main()
{
    bn::core::init();

    bn::camera_ptr camera = bn::camera_ptr::create(0, 0);
    bn::regular_bg_ptr world = bn::regular_bg_items::world.create_bg(0, 0);
    world.set_camera(camera);

    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(0, 0);
    hero.set_camera(camera);
    hero_walk walk;

    bn::sprite_text_generator text_generator(hud::font);
    bn::vector<bn::sprite_ptr, 4> name_sprites;
    const char* shown_name = nullptr;

    bn::fixed camera_x = 512 - bn::display::width() / 2;
    bn::fixed camera_y = 512 - bn::display::height() / 2;

    // The song that's playing, which isn't always the one the region wants: during a fade it's the old one.
    bn::music_item playing = region_at(hero.position()).song;
    bn::fixed volume = 1;
    playing.play(volume);

    while(true)
    {
        int speed = bn::keypad::b_held() ? 4 : 1;
        int step_x = bn::keypad::right_held() - bn::keypad::left_held();
        int step_y = bn::keypad::down_held() - bn::keypad::up_held();
        hero.set_position(bn::clamp(hero.x() + step_x * speed, bn::fixed(-504), bn::fixed(504)),
                          bn::clamp(hero.y() + step_y * speed, bn::fixed(-504), bn::fixed(496)));
        walk.update(hero, step_x, step_y, speed);
        camera.set_position(bn::clamp(hero.x(), -camera_x, camera_x), bn::clamp(hero.y(), -camera_y, camera_y));

        const region& here = region_at(hero.position());

        if(here.song != playing)
        {
            // Fade the old song out. Only when it's silent does the new one start, from its beginning.
            volume -= fade_step;

            if(volume <= 0)
            {
                volume = 0;
                playing = here.song;
                playing.play(volume);
            }
        }
        else if(volume < 1)
        {
            // Fade in. Walking back during a fade out lands here too: the old song comes back without restarting.
            volume = bn::min(volume + fade_step, bn::fixed(1));
        }

        bn::music::set_volume(volume);

        if(here.name != shown_name)
        {
            shown_name = here.name;
            name_sprites.clear();
            text_generator.generate(-112, -72, shown_name, name_sprites);
        }

        bn::core::update();
    }
}
