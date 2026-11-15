#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_string_view.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_box.h"
#include "bn_regular_bg_items_world.h"
#include "bn_sprite_items_hero.h"
#include "bn_sprite_items_spark.h"
#include "bn_sprite_items_villager.h"

#include "font.h"

constexpr bn::string_view villager_text =
        "Hi there! Not many people walk this far from the village. The coins? They show up every "
        "morning, and nobody knows who leaves them. If you find out, come back and tell me. "
        "I'll be standing right here, like always.";

constexpr int max_line_width = 216;
constexpr int lines_per_page = 3;
constexpr int frames_per_character = 2;
constexpr int text_x = -110;
constexpr int first_line_y = 46;
constexpr int line_height = 12;

// Splits text into lines no wider than max_width, breaking at spaces.
// The lines are views into text, so text must outlive them.
void wrap(bn::string_view text, const bn::sprite_text_generator& generator, int max_width,
          bn::ivector<bn::string_view>& lines)
{
    lines.clear();

    while(! text.empty())
    {
        int end = text.size();

        if(generator.width(text) > max_width)
        {
            // The last space where everything before it still fits.
            end = 0;

            for(int index = 0; index < text.size(); ++index)
            {
                if(text[index] == ' ')
                {
                    if(generator.width(text.substr(0, index)) > max_width)
                    {
                        break;
                    }

                    end = index;
                }
            }

            BN_ASSERT(end > 0, "A word is wider than the box");
        }

        lines.push_back(text.substr(0, end));
        text.remove_prefix(bn::min(end + 1, text.size()));
    }
}

int main()
{
    bn::core::init();

    bn::regular_bg_ptr world = bn::regular_bg_items::world.create_bg(0, 0);
    bn::sprite_ptr villager = bn::sprite_items::villager.create_sprite(40, -10);
    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(-40, 20);

    bn::sprite_text_generator text_generator(hud::font);
    text_generator.set_bg_priority(0);

    // The box covers the sprites in front of it: background priority 0 beats sprites at the default 3.
    bn::regular_bg_ptr box = bn::regular_bg_items::box.create_bg(0, 0);
    box.set_priority(0);
    box.set_visible(false);

    bn::sprite_ptr more = bn::sprite_items::spark.create_sprite(106, 70, 1);
    more.set_bg_priority(0);
    more.set_visible(false);

    bn::vector<bn::string_view, 12> lines;
    bn::vector<bn::sprite_ptr, 8> line_sprites[lines_per_page];
    bn::vector<bn::sprite_ptr, 4> hint_sprites;

    bool talking = false;
    int first_line = 0;     // The line at the top of the box.
    int shown = 0;          // Characters shown on this page.
    int page_size = 0;      // Characters on this page.
    int timer = 0;
    int row_shown[lines_per_page] = {};  // Characters each row of the box shows now.

    while(true)
    {
        if(talking)
        {
            if(bn::keypad::a_pressed())
            {
                if(shown < page_size)
                {
                    shown = page_size;  // Show the rest of the page at once.
                }
                else if(first_line + lines_per_page < lines.size())
                {
                    first_line += lines_per_page;
                    shown = 0;
                }
                else
                {
                    talking = false;
                }

                timer = 0;
            }
            else if(shown < page_size && ++timer == frames_per_character)
            {
                ++shown;
                timer = 0;
            }

            if(! talking)
            {
                box.set_visible(false);
                more.set_visible(false);

                for(int row = 0; row < lines_per_page; ++row)
                {
                    line_sprites[row].clear();
                    row_shown[row] = 0;
                }
            }
            else
            {
                page_size = 0;
                int remaining = shown;

                // Each row shows as many characters as are left to show. Only rows that changed are rebuilt.
                for(int row = 0; row < lines_per_page; ++row)
                {
                    int line_index = first_line + row;
                    bn::string_view line = line_index < lines.size() ? lines[line_index] : bn::string_view();
                    page_size += line.size();

                    int visible = bn::min(remaining, line.size());
                    remaining -= visible;

                    if(visible != row_shown[row])
                    {
                        row_shown[row] = visible;
                        line_sprites[row].clear();
                        text_generator.generate(text_x, first_line_y + row * line_height,
                                                line.substr(0, visible), line_sprites[row]);
                    }
                }

                more.set_visible(shown == page_size && timer++ % 32 < 20);
            }
        }
        else
        {
            int step_x = bn::keypad::right_held() - bn::keypad::left_held();
            int step_y = bn::keypad::down_held() - bn::keypad::up_held();
            hero.set_position(hero.position() + bn::fixed_point(step_x, step_y));

            if(step_x != 0)
            {
                hero.set_horizontal_flip(step_x < 0);
            }

            bn::fixed_point gap = villager.position() - hero.position();
            bool near = bn::abs(gap.x()) < 24 && bn::abs(gap.y()) < 24;

            if(near && hint_sprites.empty())
            {
                text_generator.set_center_alignment();
                text_generator.generate(0, 70, "A: talk", hint_sprites);
                text_generator.set_left_alignment();
            }
            else if(! near)
            {
                hint_sprites.clear();
            }

            if(near && bn::keypad::a_pressed())
            {
                talking = true;
                hint_sprites.clear();
                wrap(villager_text, text_generator, max_line_width, lines);
                first_line = 0;
                shown = 0;
                timer = 0;
                box.set_visible(true);
            }
        }

        bn::core::update();
    }
}
