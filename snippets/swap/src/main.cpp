#include "bn_core.h"
#include "bn_format.h"
#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_bag.h"
#include "bn_regular_bg_items_world.h"
#include "bn_sprite_items_cursor.h"
#include "bn_sprite_items_hero.h"
#include "bn_sprite_items_items.h"

#include "hero_walk.h"

#include "font.h"

// Hides or shows every sprite of a text.
void set_visible(bn::ivector<bn::sprite_ptr>& sprites, bool visible)
{
    for(bn::sprite_ptr& sprite : sprites)
    {
        sprite.set_visible(visible);
    }
}

// The field: the hero walks, and a clock counts the time spent in it.
struct field_screen
{
    bn::regular_bg_ptr world = bn::regular_bg_items::world.create_bg(0, 0);
    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(0, 0);
    hero_walk walk;
    bn::vector<bn::sprite_ptr, 8> clock;
    int frames = 0;
    int shown_seconds = -1;

    void set_visible(bool visible)
    {
        world.set_visible(visible);
        hero.set_visible(visible);
        ::set_visible(clock, visible);
    }

    // Called on every frame, shown or not: time goes on in the field, but the hero only moves when it's shown.
    void update(bool shown, bn::sprite_text_generator& text_generator)
    {
        ++frames;

        if(! shown)
        {
            return;
        }

        int step_x = bn::keypad::right_held() - bn::keypad::left_held();
        int step_y = bn::keypad::down_held() - bn::keypad::up_held();
        hero.set_position(hero.position() + bn::fixed_point(step_x, step_y));
        walk.update(hero, step_x, step_y);

        int seconds = frames / 60;

        if(seconds != shown_seconds)
        {
            shown_seconds = seconds;
            clock.clear();
            text_generator.generate(-108, -68, bn::format<16>("Time {}:{}{}", seconds / 60, seconds % 60 / 10,
                                                               seconds % 10), clock);
        }
    }
};

// The bag: three items and a cursor on one of them.
struct bag_screen
{
    bn::regular_bg_ptr panel = bn::regular_bg_items::bag.create_bg(0, 0);
    bn::vector<bn::sprite_ptr, 3> icons;
    bn::vector<bn::sprite_ptr, 12> text;
    bn::sprite_ptr cursor = bn::sprite_items::cursor.create_sprite(0, 0);
    int selected = 0;

    explicit bag_screen(bn::sprite_text_generator& text_generator)
    {
        constexpr const char* names[] = { "Key", "Axe", "Bucket" };
        text_generator.generate(-80, -52, "Bag", text);

        for(int item = 0; item < 3; ++item)
        {
            icons.push_back(bn::sprite_items::items.create_sprite(-72, item_y(item), item));
            text_generator.generate(-56, item_y(item), names[item], text);
        }

        cursor.set_position(-72, item_y(selected));
    }

    static int item_y(int item)
    {
        return -24 + item * 24;
    }

    void set_visible(bool visible)
    {
        panel.set_visible(visible);
        ::set_visible(icons, visible);
        ::set_visible(text, visible);
        cursor.set_visible(visible);
    }

    // Only called while it's shown: nothing changes in the bag while the player is in the field.
    void update()
    {
        selected = (selected + 3 + bn::keypad::down_pressed() - bn::keypad::up_pressed()) % 3;
        cursor.set_y(item_y(selected));
    }
};

int main()
{
    bn::core::init();

    bn::sprite_text_generator text_generator(hud::font);

    // Both screens are made once and stay alive: swapping only hides one and shows the other.
    field_screen field;
    bag_screen bag(text_generator);
    bag.set_visible(false);
    bool in_bag = false;

    while(true)
    {
        if(bn::keypad::select_pressed())
        {
            in_bag = ! in_bag;
            field.set_visible(! in_bag);
            bag.set_visible(in_bag);
        }

        field.update(! in_bag, text_generator);

        if(in_bag)
        {
            bag.update();
        }

        bn::core::update();
    }
}
