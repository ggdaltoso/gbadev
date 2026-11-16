#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_regular_bg_ptr.h"
#include "bn_span.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_room.h"
#include "bn_sprite_items_dot.h"
#include "bn_sprite_items_hero.h"
#include "bn_sprite_items_log.h"

#include "font.h"
#include "room.h"

constexpr bn::fixed view_range = 96;
constexpr int half_cone = 25;                    // degrees on each side: the cone is 50 degrees wide.
constexpr bn::fixed half_cone_cos = 0.9063;      // cos(25 degrees)
constexpr bn::fixed turn_speed = 0.5;            // degrees per frame
constexpr int dot_spacing = 12;
constexpr int dots_per_ray = 8;                  // 8 dots, 12 pixels apart, cover the 96 pixels.

// Outside the room counts as solid.
bool is_solid_cell(int column, int row)
{
    if(column < 0 || row < 0 || column >= room::columns || row >= room::rows)
    {
        return true;
    }

    return room::cells[row][column] == '#';
}

// Is this screen position inside a crate? As on the map collision page.
bool is_solid(bn::fixed x, bn::fixed y)
{
    int pixel_x = (x + 120).floor_integer();
    int pixel_y = (y + 80).floor_integer();

    if(pixel_x < 0 || pixel_y < 0)
    {
        return true;
    }

    return is_solid_cell(pixel_x / room::cell_size, pixel_y / room::cell_size);
}

bool hits_crate(bn::fixed_point center)
{
    constexpr bn::fixed half = 5;
    return is_solid(center.x() - half, center.y() - half) || is_solid(center.x() + half - 1, center.y() - half) ||
            is_solid(center.x() - half, center.y() + half - 1) || is_solid(center.x() + half - 1, center.y() + half - 1);
}

// How far a ray goes from `from` along `direction` (length 1) before it enters a crate, at most `max`.
// It visits every cell the ray crosses, in order: each step goes to whichever grid line is nearer,
// the next vertical one or the next horizontal one, and enters the cell behind it.
bn::fixed cast_ray(bn::fixed_point from, bn::fixed_point direction, bn::fixed max)
{
    bn::fixed x = from.x() + 120;   // In pixels from the room's top-left corner.
    bn::fixed y = from.y() + 80;
    int column = x.floor_integer() / room::cell_size;
    int row = y.floor_integer() / room::cell_size;
    int step_column = direction.x() > 0 ? 1 : -1;
    int step_row = direction.y() > 0 ? 1 : -1;

    // Distance along the ray to the next line, and from one line to the next. A ray that doesn't
    // move on an axis never meets that axis's lines, and dividing by its 0 would stop the game.
    // safe_division, not /: / drops half the divisor's bits, so anything under 1/64 counts as 0.
    bn::fixed next_x = max + 1;
    bn::fixed every_x;

    if(direction.x() != 0)
    {
        bn::fixed line_x = (step_column > 0 ? column + 1 : column) * room::cell_size;
        next_x = bn::abs(line_x - x).safe_division(bn::abs(direction.x()));
        every_x = bn::fixed(room::cell_size).safe_division(bn::abs(direction.x()));
    }

    bn::fixed next_y = max + 1;
    bn::fixed every_y;

    if(direction.y() != 0)
    {
        bn::fixed line_y = (step_row > 0 ? row + 1 : row) * room::cell_size;
        next_y = bn::abs(line_y - y).safe_division(bn::abs(direction.y()));
        every_y = bn::fixed(room::cell_size).safe_division(bn::abs(direction.y()));
    }

    while(true)
    {
        bn::fixed travelled;

        if(next_x < next_y)
        {
            travelled = next_x;
            next_x += every_x;
            column += step_column;
        }
        else
        {
            travelled = next_y;
            next_y += every_y;
            row += step_row;
        }

        if(travelled >= max)
        {
            return max;
        }

        if(is_solid_cell(column, row))
        {
            return travelled;
        }
    }
}

bn::fixed_point direction_of(bn::fixed angle)
{
    // The sine table only takes 0 to 360.
    if(angle < 0)
    {
        angle += 360;
    }
    else if(angle >= 360)
    {
        angle -= 360;
    }

    bn::pair<bn::fixed, bn::fixed> sin_and_cos = bn::degrees_lut_sin_and_cos(angle);
    return bn::fixed_point(sin_and_cos.second, sin_and_cos.first);
}

// Dots along a ray, every dot_spacing pixels, as far as `length`.
void show_ray(bn::span<bn::sprite_ptr> dots, bn::fixed_point from, bn::fixed_point direction, bn::fixed length)
{
    for(int index = 0; index < dots.size(); ++index)
    {
        int along = (index + 1) * dot_spacing;
        dots[index].set_position(from + direction * along);
        dots[index].set_visible(along <= length);
    }
}

int main()
{
    bn::core::init();

    bn::regular_bg_ptr room_bg = bn::regular_bg_items::room.create_bg(0, 0);
    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(-80, -40);
    bn::sprite_ptr sentinel = bn::sprite_items::log.create_sprite(0, 8, 8);

    // Three rays of dots: the cone's two edges, then the line to the hero.
    bn::vector<bn::sprite_ptr, dots_per_ray * 3> dots;

    for(int index = 0; index < dots.max_size(); ++index)
    {
        int frame = index < dots_per_ray * 2 ? 0 : 1;
        dots.push_back(bn::sprite_items::dot.create_sprite(0, 0, frame));
    }

    bn::span<bn::sprite_ptr> all_dots(dots.data(), dots.size());
    bn::span<bn::sprite_ptr> left_edge = all_dots.subspan(0, dots_per_ray);
    bn::span<bn::sprite_ptr> right_edge = all_dots.subspan(dots_per_ray, dots_per_ray);
    bn::span<bn::sprite_ptr> sight_line = all_dots.subspan(dots_per_ray * 2, dots_per_ray);

    bn::sprite_text_generator text_generator(hud::font);
    text_generator.set_center_alignment();
    bn::vector<bn::sprite_ptr, 1> alert;
    text_generator.generate(0, -12, "!", alert);

    bn::fixed facing_angle = 0;

    while(true)
    {
        // Walk one axis at a time, so the hero slides along crates.
        int step_x = bn::keypad::right_held() - bn::keypad::left_held();
        int step_y = bn::keypad::down_held() - bn::keypad::up_held();

        if(! hits_crate(hero.position() + bn::fixed_point(step_x, 0)))
        {
            hero.set_x(hero.x() + step_x);
        }

        if(! hits_crate(hero.position() + bn::fixed_point(0, step_y)))
        {
            hero.set_y(hero.y() + step_y);
        }

        // The sentinel turns around slowly, like a lighthouse.
        facing_angle += turn_speed;

        if(facing_angle >= 360)
        {
            facing_angle -= 360;
        }

        bn::fixed_point facing = direction_of(facing_angle);
        bn::fixed_point eye = sentinel.position();
        sentinel.set_horizontal_flip(facing.x() < 0);

        bn::fixed_point left = direction_of(facing_angle - half_cone);
        bn::fixed_point right = direction_of(facing_angle + half_cone);
        show_ray(left_edge, eye, left, cast_ray(eye, left, view_range));
        show_ray(right_edge, eye, right, cast_ray(eye, right, view_range));

        // Seen means three things: close enough, inside the cone, and no crate in between.
        bn::fixed_point to_hero = hero.position() - eye;
        bn::fixed distance = bn::sqrt(to_hero.x() * to_hero.x() + to_hero.y() * to_hero.y());
        bool seen = false;

        if(distance > 0 && distance < view_range)
        {
            bn::fixed_point toward = to_hero / distance;

            // The dot product of two directions of length 1 is the cosine of the angle between them.
            bn::fixed cosine = toward.x().safe_multiplication(facing.x()) +
                    toward.y().safe_multiplication(facing.y());
            seen = cosine >= half_cone_cos && cast_ray(eye, toward, distance) >= distance;

            show_ray(sight_line, eye, toward, seen ? distance : 0);
        }
        else
        {
            show_ray(sight_line, eye, facing, 0);
        }

        alert.front().set_visible(seen);
        bn::core::update();
    }
}
