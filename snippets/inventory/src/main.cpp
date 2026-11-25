#include "bn_algorithm.h"
#include "bn_core.h"
#include "bn_format.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_random.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_sram.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_field.h"
#include "bn_sprite_items_cursor.h"
#include "bn_sprite_items_food.h"

#include "font.h"
#include "hero_walk.h"

// One row per kind of item, in the order of food.bmp's frames. The game keeps a row's index, never the row.
struct item_type
{
    const char* name;
    int stack;   // The most one slot holds.
    int heal;
};

constexpr item_type item_types[] = {
    { "Candy", 9, 1 },
    { "Donut", 5, 2 },
    { "Pizza", 3, 4 },
    { "Burger", 2, 6 },
};

constexpr int item_type_count = sizeof(item_types) / sizeof(item_types[0]);
constexpr int bag_size = 6;
constexpr int max_health = 10;

// Which item and how many: two ints, so a slot can go to SRAM as it is.
struct slot
{
    int item;
    int count;
};

struct save_data
{
    int format;
    int health;
    int slot_count;
    slot slots[bag_size];
};

constexpr int save_format = 0x494E5631;

struct food
{
    bn::sprite_ptr sprite;
    int item;
};

// One item into the bag: onto a stack of the same item with room, or else into a new slot.
// False when it doesn't fit, so the caller can leave it on the ground.
bool add(bn::vector<slot, bag_size>& bag, int item)
{
    for(slot& held : bag)
    {
        if(held.item == item && held.count < item_types[item].stack)
        {
            ++held.count;
            return true;
        }
    }

    if(bag.full())
    {
        return false;
    }

    bag.push_back({ item, 1 });
    return true;
}

int main()
{
    bn::core::init();

    save_data save;
    bn::sram::read(save);

    if(save.format != save_format)
    {
        save = { save_format, max_health, 0, {} };
    }

    int health = save.health;
    bn::vector<slot, bag_size> bag;

    for(int index = 0; index < save.slot_count; ++index)
    {
        bag.push_back(save.slots[index]);
    }

    bn::regular_bg_ptr field = bn::regular_bg_items::field.create_bg(0, 0);
    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(0, -8);
    hero_walk walk;
    bn::sprite_ptr cursor = bn::sprite_items::cursor.create_sprite(-60, 64);
    bn::vector<food, 3> ground;
    bn::random random;

    bn::sprite_text_generator text_generator(hud::font);
    bn::vector<bn::sprite_ptr, bag_size> icons;
    bn::vector<bn::sprite_ptr, 24> text;

    int selected = 0;      // The slot under the cursor, which may be empty.
    int spawn_timer = 0;
    int hunger = 0;
    int full_message = 0;  // Frames left of "Bag full".
    bool changed = true;

    while(true)
    {
        int step_x = bn::keypad::right_held() - bn::keypad::left_held();
        int step_y = bn::keypad::down_held() - bn::keypad::up_held();
        hero.set_position(bn::clamp(hero.x() + step_x, bn::fixed(-104), bn::fixed(104)),
                          bn::clamp(hero.y() + step_y, bn::fixed(-60), bn::fixed(28)));
        walk.update(hero, step_x, step_y);

        if(--spawn_timer <= 0 && ! ground.full())
        {
            spawn_timer = 90;
            int item = random.get_int(item_type_count);
            int x = random.get_int(-96, 97);
            int y = random.get_int(-52, 33);
            ground.push_back({ bn::sprite_items::food.create_sprite(x, y, item), item });
        }

        // Only what fits in the bag leaves the ground.
        bool bag_changed = false;
        bool was_full = full_message > 0;

        bn::erase_if(ground, [&](const food& lying)
        {
            if(bn::abs(lying.sprite.x() - hero.x()) < 12 && bn::abs(lying.sprite.y() - hero.y() - 6) < 12)
            {
                if(add(bag, lying.item))
                {
                    bag_changed = true;
                    return true;
                }

                full_message = 60;
            }

            return false;
        });

        if(bn::keypad::l_pressed() || bn::keypad::r_pressed())
        {
            selected = bn::clamp(selected + bn::keypad::r_pressed() - bn::keypad::l_pressed(), 0, bag_size - 1);
            changed = true;
        }

        // Eating: an empty slot has nothing to eat.
        if(bn::keypad::a_pressed() && selected < bag.size())
        {
            slot& held = bag[selected];
            health = bn::min(health + item_types[held.item].heal, max_health);

            if(--held.count == 0)
            {
                bag.erase(bag.begin() + selected);   // The slots after it move one to the left.
            }

            bag_changed = true;
        }

        if(++hunger == 180)
        {
            hunger = 0;
            health = bn::max(health - 1, 0);
            bag_changed = true;   // Not the bag, but health is saved with it.
        }

        // Saved on every change, never every frame: the save is always what's on screen.
        if(bag_changed)
        {
            save.health = health;
            save.slot_count = bag.size();

            for(int index = 0; index < bag.size(); ++index)
            {
                save.slots[index] = bag[index];
            }

            bn::sram::write(save);
            changed = true;
        }

        if(full_message > 0)
        {
            --full_message;
        }

        if(changed || (full_message > 0) != was_full)
        {
            changed = false;
            icons.clear();
            text.clear();
            text_generator.set_left_alignment();
            text_generator.generate(-112, -72, bn::format<16>("Health {}", health), text);

            for(int index = 0; index < bag.size(); ++index)
            {
                int x = -60 + index * 24;
                icons.push_back(bn::sprite_items::food.create_sprite(x, 63, bag[index].item));
                text_generator.generate(x + 5, 72, bn::format<4>("{}", bag[index].count), text);
            }

            text_generator.set_right_alignment();

            if(selected < bag.size())
            {
                const item_type& type = item_types[bag[selected].item];
                text_generator.generate(112, -72, bn::format<24>("{}: heals {}", type.name, type.heal), text);
            }

            if(full_message > 0)
            {
                text_generator.generate(112, -56, "Bag full", text);
            }
        }

        cursor.set_x(-60 + selected * 24);

        if(bn::keypad::start_pressed())
        {
            bn::core::reset();   // Back to the start of main(): the bag comes back from SRAM.
        }

        if(bn::keypad::select_pressed())
        {
            bn::sram::clear(sizeof(save_data));
            bn::core::reset();
        }

        bn::core::update();
    }
}
