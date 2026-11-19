#include "games.h"

#include "bn_algorithm.h"
#include "bn_backdrop.h"
#include "bn_color.h"
#include "bn_core.h"
#include "bn_format.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_random.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

#include "bn_sprite_items_hero.h"
#include "bn_sprite_items_mine.h"

#include "hero_walk.h"

// Mines fall from the night sky. The score is the seconds lasted before one hits.
int dodge(bn::sprite_text_generator& text_generator)
{
    // The color behind everything is global, and it stays until someone changes it: every scene that shows it
    // sets it when it starts.
    bn::backdrop::set_color(bn::color(6, 2, 10));

    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(0, 56);
    hero_walk walk;
    bn::vector<bn::sprite_ptr, 12> mines;
    bn::random random;
    bn::vector<bn::sprite_ptr, 8> text;
    int frames = 0;
    int shown_seconds = -1;
    bool hit = false;

    while(! hit)
    {
        int step_x = bn::keypad::right_held() - bn::keypad::left_held();
        hero.set_x(bn::clamp(hero.x() + step_x * 2, bn::fixed(-108), bn::fixed(108)));
        walk.update(hero, step_x, 0, 2);
        ++frames;

        if(frames % 16 == 0 && ! mines.full())
        {
            mines.push_back(bn::sprite_items::mine.create_sprite(random.get_int(-108, 109), -88));
        }

        for(bn::sprite_ptr& mine : mines)
        {
            mine.set_y(mine.y() + 2);
            bn::fixed_point gap = mine.position() - hero.position();
            hit = hit || (bn::abs(gap.x()) < 11 && bn::abs(gap.y()) < 12);
        }

        bn::erase_if(mines, [](const bn::sprite_ptr& mine) { return mine.y() > 88; });

        if(frames / 60 != shown_seconds)
        {
            shown_seconds = frames / 60;
            text.clear();
            text_generator.generate(0, -70, bn::format<16>("Time {}s", shown_seconds), text);
        }

        bn::core::update();
    }

    for(int frame = 0; frame < 60; ++frame)   // A second to see how it ended.
    {
        bn::core::update();
    }

    return frames / 60;
}
