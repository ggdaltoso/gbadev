#include "bn_algorithm.h"
#include "bn_bg_palettes.h"
#include "bn_color.h"
#include "bn_core.h"
#include "bn_format.h"
#include "bn_keypad.h"
#include "bn_random.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_utility.h"
#include "bn_vector.h"

#include "bn_sprite_items_food.h"

#include "font.h"

constexpr int kinds = 4;                   // One per frame of food.bmp.
constexpr int copies = 2;                  // Of each kind in the bag.
constexpr int bag_size = kinds * copies;
constexpr int shown = 14;                  // Draws kept on screen in each row.

// A bag of tokens taken out one by one, then refilled and shuffled: every kind comes out
// `copies` times per bag, so none can run away with a long streak.
class shuffle_bag
{

public:
    int draw(bn::random& random)
    {
        if(_left == 0)
        {
            refill(random);
        }

        --_left;
        _last = _tokens[_left];   // From the end: the bag is shuffled, so any end will do.
        return _last;
    }

private:
    int _tokens[bag_size] = {};
    int _left = 0;
    int _last = -1;

    void refill(bn::random& random)
    {
        for(int index = 0; index < bag_size; ++index)
        {
            _tokens[index] = index % kinds;
        }

        // Fisher-Yates: each place swaps with one at or before it, so every order is as likely.
        for(int index = bag_size - 1; index > 0; --index)
        {
            bn::swap(_tokens[index], _tokens[random.get_int(index + 1)]);
        }

        // The seam: if the new bag's first draw repeats the old bag's last, swap another kind in.
        for(int index = 0; _tokens[bag_size - 1] == _last; ++index)
        {
            bn::swap(_tokens[bag_size - 1], _tokens[index]);
        }

        _left = bag_size;
    }
};

// One row of draws on screen, the newest on the right, and its streaks.
struct row
{
    bn::vector<bn::sprite_ptr, shown> icons;
    int history[shown];
    int streak = 0;
    int longest = 0;

    explicit row(int y)
    {
        for(int index = 0; index < shown; ++index)
        {
            icons.push_back(bn::sprite_items::food.create_sprite(-104 + index * 16, y));
            icons.back().set_visible(false);
            history[index] = -1;   // Nothing drawn yet.
        }
    }

    void add(int kind)
    {
        streak = kind == history[shown - 1] ? streak + 1 : 1;
        longest = bn::max(longest, streak);

        for(int index = 0; index < shown; ++index)
        {
            history[index] = index < shown - 1 ? history[index + 1] : kind;

            if(history[index] >= 0)
            {
                icons[index].set_tiles(bn::sprite_items::food.tiles_item(), history[index]);
                icons[index].set_visible(true);
            }
        }
    }
};

int main()
{
    bn::core::init();
    bn::bg_palettes::set_transparent_color(bn::color(4, 6, 10));

    bn::random random;
    shuffle_bag bag;
    row random_row(-28);
    row bag_row(36);

    bn::sprite_text_generator text_generator(hud::font);
    bn::vector<bn::sprite_ptr, 32> text;
    text_generator.generate(-112, 70, "Hold A: faster", text);
    int fixed_text = text.size();   // The hint stays; the rest is rewritten on every draw.

    int draws = 0;
    int timer = 0;

    while(true)
    {
        // A draw every 15 frames, or every frame while A is held. Both rows draw at the same time.
        if(bn::keypad::a_held() || ++timer == 15)
        {
            timer = 0;
            ++draws;
            random_row.add(random.get_int(kinds));
            bag_row.add(bag.draw(random));

            text.shrink(fixed_text);
            text_generator.generate(-112, -72, bn::format<16>("Draws {}", draws), text);
            text_generator.generate(-112, -48, bn::format<48>("Random: streak {}, longest {}",
                                                              random_row.streak, random_row.longest), text);
            text_generator.generate(-112, 16, bn::format<48>("Shuffle bag: streak {}, longest {}",
                                                             bag_row.streak, bag_row.longest), text);
        }

        bn::core::update();
    }
}
