#include "bn_algorithm.h"
#include "bn_core.h"
#include "bn_format.h"
#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_menu.h"
#include "bn_sprite_items_coin.h"
#include "bn_sprite_items_cursor.h"
#include "bn_sprite_items_lock.h"

#include "font.h"

constexpr int columns = 3;
constexpr int rows = 2;
constexpr int levels = columns * rows;
constexpr int coins_per_level = 3;

// The goal of each level: how many coins in all, from every level, open it.
constexpr int coins_needed[levels] = { 0, 1, 3, 5, 8, 11 };

// What the player has done: the best number of coins won in each level. The only state the menu needs.
int coins_won[levels] = {};

int total_coins()
{
    int total = 0;

    for(int coins : coins_won)
    {
        total += coins;
    }

    return total;
}

// Worked out every time from the coins, never kept: it can't fall behind them.
bool unlocked(int level)
{
    return total_coins() >= coins_needed[level];
}

// The center of a level's box on the screen.
int box_x(int level)
{
    return (level % columns - 1) * 72;
}

int box_y(int level)
{
    return level / columns * 52 - 20;
}

// Everything on the menu is drawn from coins_won and the cursor, each time one of them changes.
void draw(int selected, bn::sprite_text_generator& text_generator, bn::ivector<bn::sprite_ptr>& sprites)
{
    sprites.clear();
    text_generator.generate(0, -68, "Levels", sprites);

    for(int level = 0; level < levels; ++level)
    {
        int x = box_x(level);
        int y = box_y(level);
        text_generator.generate(x, y - 12, bn::format<4>("{}", level + 1), sprites);

        if(! unlocked(level))
        {
            sprites.push_back(bn::sprite_items::lock.create_sprite(x, y + 6));
            continue;
        }

        for(int coin = 0; coin < coins_per_level; ++coin)
        {
            int frame = coin < coins_won[level] ? 0 : 1;   // Won, or still to win.
            sprites.push_back(bn::sprite_items::coin.create_sprite(x + (coin - 1) * 18, y + 8, frame));
        }
    }

    // The goal still missing, when there is one.
    int missing = coins_needed[selected] - total_coins();

    if(missing > 0)
    {
        text_generator.generate(0, 66, bn::format<32>("Level {}: {} more coin{} to open", selected + 1, missing,
                                                      missing == 1 ? "" : "s"), sprites);
    }
    else
    {
        text_generator.generate(0, 66, bn::format<32>("Level {}: {} of 3 coins. A: play", selected + 1,
                                                      coins_won[selected]), sprites);
    }
}

int main()
{
    bn::core::init();

    bn::regular_bg_ptr menu = bn::regular_bg_items::menu.create_bg(0, 0);
    bn::sprite_text_generator text_generator(hud::font);
    text_generator.set_center_alignment();
    bn::vector<bn::sprite_ptr, 64> sprites;
    bn::sprite_ptr cursor = bn::sprite_items::cursor.create_sprite(0, 0);

    int column = 0;
    int row = 0;
    bool changed = true;

    while(true)
    {
        // Column and row, each kept inside the grid: the cursor can't leave it, and locked levels can be looked at.
        int new_column = bn::clamp(column + bn::keypad::right_pressed() - bn::keypad::left_pressed(), 0, columns - 1);
        int new_row = bn::clamp(row + bn::keypad::down_pressed() - bn::keypad::up_pressed(), 0, rows - 1);
        changed = changed || new_column != column || new_row != row;
        column = new_column;
        row = new_row;
        int selected = row * columns + column;

        // The lock is checked here, where the level would start, not only drawn. Playing wins one more coin.
        if(bn::keypad::a_pressed() && unlocked(selected) && coins_won[selected] < coins_per_level)
        {
            ++coins_won[selected];
            changed = true;
        }

        if(changed)
        {
            draw(selected, text_generator, sprites);
            cursor.set_position(box_x(selected), box_y(selected));
            changed = false;
        }

        bn::core::update();
    }
}
