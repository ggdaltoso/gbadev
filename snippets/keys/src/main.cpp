#include "bn_backdrop.h"
#include "bn_color.h"
#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_menu.h"
#include "bn_sprite_items_coin.h"
#include "bn_sprite_items_font.h"
#include "bn_sprite_items_font_lit.h"

#include "font.h"

using key = bn::keypad::key_type;

// Every key the GBA has, as data: a value, the name to show and the line of the screen it goes on.
struct key_info
{
    key value;
    const char* name;
    int line;
};

constexpr key_info keys[] = {
    { key::LEFT, "Left", 0 }, { key::RIGHT, "Right", 0 }, { key::UP, "Up", 0 }, { key::DOWN, "Down", 0 },
    { key::A, "A", 1 }, { key::B, "B", 1 }, { key::L, "L", 1 }, { key::R, "R", 1 },
    { key::START, "Start", 1 }, { key::SELECT, "Select", 1 },
};

// The game asks for actions, never for keys: this table is all that changes when the player remaps.
constexpr const char* action_names[] = { "Jump", "Shoot", "Dash" };
constexpr int actions_count = 3;
key bindings[actions_count] = { key::A, key::B, key::R };

const char* name_of(key value)
{
    for(const key_info& info : keys)
    {
        if(info.value == value)
        {
            return info.name;
        }
    }

    return "?";
}

// START and SELECT run this screen, so they can't be given to an action.
bool reserved(key value)
{
    return value == key::START || value == key::SELECT;
}

// The action that had this key gets the old key of the one that takes it: no key ends up doing two things.
void assign(int action, key value)
{
    for(key& binding : bindings)
    {
        if(binding == value)
        {
            binding = bindings[action];
        }
    }

    bindings[action] = value;
}

void draw(int selected, int waiting, unsigned held, bn::sprite_text_generator& text_generator,
          bn::ivector<bn::sprite_ptr>& sprites)
{
    sprites.clear();
    text_generator.set_palette_item(bn::sprite_items::font.palette_item());
    text_generator.generate(-text_generator.width("Controls") / 2, -64, "Controls", sprites);

    for(int action = 0; action < actions_count; ++action)
    {
        int y = action * 26 - 40;
        bool active = held & unsigned(bindings[action]);   // Lit while the action's key is held.
        text_generator.set_palette_item(active ? bn::sprite_items::font_lit.palette_item() :
                                                 bn::sprite_items::font.palette_item());
        text_generator.generate(-64, y, action_names[action], sprites);
        text_generator.generate(24, y, action == waiting ? "Press a key" : name_of(bindings[action]), sprites);
    }

    sprites.push_back(bn::sprite_items::coin.create_sprite(-84, selected * 26 - 40));

    // Every key, lit while held, laid out from the names' widths.
    int x[2] = { -100, -100 };

    for(const key_info& info : keys)
    {
        bool lit = held & unsigned(info.value);
        text_generator.set_palette_item(lit ? bn::sprite_items::font_lit.palette_item() :
                                              bn::sprite_items::font.palette_item());
        text_generator.generate(x[info.line], 38 + info.line * 14, info.name, sprites);
        x[info.line] += text_generator.width(info.name) + 10;
    }

    text_generator.set_palette_item(bn::sprite_items::font.palette_item());
    text_generator.generate(-100, 70, "SELECT: next  START: change", sprites);
}

int main()
{
    bn::core::init();

    bn::backdrop::set_color(bn::color(3, 4, 9));
    bn::regular_bg_ptr menu = bn::regular_bg_items::menu.create_bg(0, 0);
    bn::sprite_text_generator text_generator(hud::font);
    bn::vector<bn::sprite_ptr, 48> sprites;

    int selected = 0;
    int waiting = -1;      // The action waiting for a new key, or -1.
    unsigned shown = 0xffff;
    bool changed = true;

    while(true)
    {
        if(waiting >= 0)
        {
            // Checked from the frame after START, so the press that opened the wait isn't taken as the answer.
            for(const key_info& info : keys)
            {
                if(bn::keypad::pressed(info.value) && ! reserved(info.value))
                {
                    assign(waiting, info.value);
                    waiting = -1;
                    changed = true;
                    break;
                }
            }
        }
        else if(bn::keypad::select_pressed())
        {
            selected = (selected + 1) % actions_count;
            changed = true;
        }
        else if(bn::keypad::start_pressed())
        {
            waiting = selected;
            changed = true;
        }

        // Each key_type is one bit, so a set of keys fits in one number.
        unsigned held = 0;

        for(const key_info& info : keys)
        {
            if(bn::keypad::held(info.value))
            {
                held |= unsigned(info.value);
            }
        }

        if(changed || held != shown)
        {
            draw(selected, waiting, held, text_generator, sprites);
            shown = held;
            changed = false;
        }

        bn::core::update();
    }
}
