#include <cstddef>

#include "bn_algorithm.h"
#include "bn_backdrop.h"
#include "bn_color.h"
#include "bn_core.h"
#include "bn_format.h"
#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_sram.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_menu.h"
#include "bn_sprite_items_coin.h"

#include "font.h"
#include "games.h"

// The games in the ROM. Adding one is a new file and a line here: the hub calls it through the pointer.
struct game
{
    const char* name;
    int (*play)(bn::sprite_text_generator& text_generator);
    const char* unit;   // Written after the score.
};

constexpr game games[] = {
    { "Coin dash", coin_dash, " of 5" },
    { "Dodge", dodge, "s" },
};

constexpr int games_count = sizeof(games) / sizeof(games[0]);

// One save for the whole ROM, at the start of SRAM, with a record for each game. Only the hub uses it.
struct save_data
{
    int format;
    int best[games_count];
    unsigned checksum;   // Last, worked out from every byte before it.
};

constexpr int save_format = 0x41524331;

unsigned checksum_of(const save_data& save)
{
    const auto* bytes = reinterpret_cast<const unsigned char*>(&save);
    unsigned sum = 0;

    for(int index = 0; index < int(offsetof(save_data, checksum)); ++index)
    {
        sum = sum * 31 + bytes[index];
    }

    return sum;
}

save_data load()
{
    save_data save;
    bn::sram::read(save);

    if(save.format != save_format || save.checksum != checksum_of(save))
    {
        save = {};   // A new cart, or a damaged save: every record starts at 0.
    }

    return save;
}

void store(save_data save)
{
    save.format = save_format;
    save.checksum = checksum_of(save);
    bn::sram::write(save);
}

// The menu is a scene too: its background, text and cursor are gone when it returns the game picked.
int menu(const save_data& save, int selected, bn::sprite_text_generator& text_generator)
{
    bn::backdrop::set_color(bn::color(3, 4, 9));   // Whatever the last game left there.
    bn::regular_bg_ptr panel = bn::regular_bg_items::menu.create_bg(0, 0);
    bn::vector<bn::sprite_ptr, 24> text;
    text_generator.generate(0, -56, "Arcade", text);
    text_generator.generate(0, 56, "A: play  SELECT: reset", text);
    text_generator.set_left_alignment();

    for(int index = 0; index < games_count; ++index)
    {
        int y = index * 36 - 20;
        text_generator.generate(-72, y, games[index].name, text);
        text_generator.generate(20, y, bn::format<16>("Best: {}{}", save.best[index], games[index].unit), text);
    }

    text_generator.set_center_alignment();
    bn::sprite_ptr cursor = bn::sprite_items::coin.create_sprite(-86, 0);

    while(! bn::keypad::a_pressed())
    {
        selected = bn::clamp(selected + bn::keypad::down_pressed() - bn::keypad::up_pressed(), 0, games_count - 1);
        cursor.set_y(selected * 36 - 20);

        if(bn::keypad::select_pressed())
        {
            bn::core::reset();   // Off and on again: the records come back from SRAM.
        }

        bn::core::update();
    }

    return selected;
}

int main()
{
    bn::core::init();

    bn::sprite_text_generator text_generator(hud::font);
    text_generator.set_center_alignment();
    save_data save = load();
    int selected = 0;

    while(true)
    {
        selected = menu(save, selected, text_generator);
        int score = games[selected].play(text_generator);

        if(score > save.best[selected])
        {
            save.best[selected] = score;
            store(save);   // Only when a record is beaten.
        }
    }
}
