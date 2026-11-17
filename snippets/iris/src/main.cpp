#include "bn_array.h"
#include "bn_bg_palettes.h"
#include "bn_color.h"
#include "bn_core.h"
#include "bn_display.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_rect_window.h"
#include "bn_rect_window_boundaries_hbe_ptr.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_window.h"

#include "bn_regular_bg_items_ground.h"
#include "bn_regular_bg_items_inside.h"
#include "bn_sprite_items_hero.h"
#include "bn_sprite_items_hole.h"

#include "hero_walk.h"

constexpr int iris_frames = 24;
constexpr int hero_feet = 12;

// Where the hole is in each place, and where the hero comes out: beside it, not on it.
constexpr bn::fixed_point outside_hole(56, -24);
constexpr bn::fixed_point inside_hole(-64, 32);
constexpr bn::fixed_point arrival_offset(0, -20);

enum class iris_phase
{
    open,
    closing,
    opening
};

using window_lines = bn::array<bn::pair<bn::fixed, bn::fixed>, bn::display::height()>;

// One pair per screen line: where the window starts and ends on it. Together they make a circle.
void shape_iris(window_lines& lines, bn::fixed_point center, int radius)
{
    for(int line = 0; line < bn::display::height(); ++line)
    {
        int dy = line - bn::display::height() / 2 - center.y().round_integer();
        int squared = radius * radius - dy * dy;

        if(squared <= 0)
        {
            lines[line] = bn::pair<bn::fixed, bn::fixed>(0, 0);   // Nothing on this line.
        }
        else
        {
            // Past the screen's sides is fine: Butano cuts the values to the screen.
            int half = bn::sqrt(squared);
            lines[line] = bn::pair<bn::fixed, bn::fixed>(center.x() - half, center.x() + half);
        }
    }
}

// Just enough to cover the corner of the screen furthest from the center: a bigger circle
// would spend the first frames of the transition shrinking where nobody sees it.
int covering_radius(bn::fixed_point center)
{
    int dx = bn::display::width() / 2 + bn::abs(center.x()).ceil_integer();
    int dy = bn::display::height() / 2 + bn::abs(center.y()).ceil_integer();
    return bn::sqrt(dx * dx + dy * dy) + 1;
}

int main()
{
    bn::core::init();
    bn::bg_palettes::set_transparent_color(bn::color(0, 0, 0));

    bn::regular_bg_ptr grass = bn::regular_bg_items::ground.create_bg(0, 0);
    bn::regular_bg_ptr cave = bn::regular_bg_items::inside.create_bg(0, 0);
    cave.set_visible(false);
    bool inside = false;

    bn::sprite_ptr hole = bn::sprite_items::hole.create_sprite(outside_hole);
    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(-60, 20);
    hero_walk walk;

    // Outside the window, nothing: the black backdrop. Inside, everything.
    bn::window outside_window = bn::window::outside();
    outside_window.set_show_bg(grass, false);
    outside_window.set_show_bg(cave, false);
    outside_window.set_show_sprites(false);

    bn::rect_window iris = bn::rect_window::internal();
    iris.set_top(-bn::display::height() / 2);
    iris.set_bottom(bn::display::height() / 2);

    window_lines lines;
    shape_iris(lines, hero.position(), covering_radius(hero.position()));
    bn::rect_window_boundaries_hbe_ptr iris_lines = bn::rect_window_boundaries_hbe_ptr::create_horizontal(iris, lines);

    iris_phase phase = iris_phase::open;
    int frame = 0;

    while(true)
    {
        if(phase == iris_phase::open)
        {
            // Only while the iris is open does the hero walk. During the transition, input waits.
            int step_x = bn::keypad::right_held() - bn::keypad::left_held();
            int step_y = bn::keypad::down_held() - bn::keypad::up_held();
            hero.set_position(bn::clamp(hero.x() + step_x, bn::fixed(-100), bn::fixed(100)),
                              bn::clamp(hero.y() + step_y, bn::fixed(-70), bn::fixed(52)));

            walk.update(hero, step_x, step_y);

            bn::fixed_point to_hole = hole.position() - (hero.position() + bn::fixed_point(0, hero_feet));

            if(bn::abs(to_hole.x()) < 6 && bn::abs(to_hole.y()) < 6)
            {
                phase = iris_phase::closing;
                frame = 0;
            }
        }
        else
        {
            walk.update(hero, 0, 0);   // Input waits, so the hero stands still through the transition.
            ++frame;
            int full = covering_radius(hero.position());
            int radius = phase == iris_phase::closing ? full * (iris_frames - frame) / iris_frames :
                                                         full * frame / iris_frames;
            shape_iris(lines, hero.position(), radius);
            iris_lines.reload_deltas_ref();

            if(frame == iris_frames)
            {
                if(phase == iris_phase::closing)
                {
                    // The screen is black: change places now, where nobody sees it.
                    inside = ! inside;
                    grass.set_visible(! inside);
                    cave.set_visible(inside);
                    hole.set_position(inside ? inside_hole : outside_hole);
                    hero.set_position(hole.position() + arrival_offset);
                    phase = iris_phase::opening;
                }
                else
                {
                    phase = iris_phase::open;
                }

                frame = 0;
            }
        }

        bn::core::update();
    }
}
