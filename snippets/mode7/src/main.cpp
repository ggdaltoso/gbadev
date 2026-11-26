#include "bn_affine_bg_dx_register_hbe_ptr.h"
#include "bn_affine_bg_dy_register_hbe_ptr.h"
#include "bn_affine_bg_pa_register_hbe_ptr.h"
#include "bn_affine_bg_pc_register_hbe_ptr.h"
#include "bn_affine_bg_ptr.h"
#include "bn_bg_palettes.h"
#include "bn_color.h"
#include "bn_core.h"
#include "bn_display.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_sprite_ptr.h"

#include "bn_affine_bg_items_track.h"
#include "bn_sprite_items_car.h"

constexpr int horizon = 48;               // The screen line where the floor starts.
constexpr bn::fixed camera_height = 32;   // In map pixels: lower looks faster and flatter.
constexpr int focal = 128;                // Distance from the eye to the screen: bigger zooms in.
constexpr int lines = bn::display::height();
constexpr int half_width = bn::display::width() / 2;

// One value per screen line, in the formats the hardware wants: 8 bits after the point.
struct floor_lines
{
    int16_t pa[lines];   // How far across the map one pixel to the right goes.
    int16_t pc[lines];
    int dx[lines];       // Where on the map the line's leftmost pixel is.
    int dy[lines];
};

void update_floor(bn::fixed_point camera, int angle, floor_lines& floor)
{
    bn::fixed forward_x = bn::degrees_lut_cos(angle);
    bn::fixed forward_y = -bn::degrees_lut_sin(angle);   // The map's y grows down.
    bn::fixed right_x = -forward_y;
    bn::fixed right_y = forward_x;

    for(int line = 0; line < lines; ++line)
    {
        int below = line - horizon;

        if(below <= 0)
        {
            // Above the horizon: far off the map, where it's transparent, so the sky shows.
            floor.pa[line] = 0;
            floor.pc[line] = 0;
            floor.dx[line] = -4096 << 8;
            floor.dy[line] = -4096 << 8;
            continue;
        }

        // Lines near the horizon look far away, lines at the bottom look at the ground nearby.
        bn::fixed distance = camera_height * focal / below;
        bn::fixed scale = distance / focal;   // Map pixels per screen pixel on this line.
        bn::fixed center_x = camera.x() + forward_x * distance;
        bn::fixed center_y = camera.y() + forward_y * distance;

        floor.pa[line] = int16_t((right_x * scale).data() >> 4);
        floor.pc[line] = int16_t((right_y * scale).data() >> 4);
        floor.dx[line] = (center_x - right_x * scale * half_width).data() >> 4;
        floor.dy[line] = (center_y - right_y * scale * half_width).data() >> 4;
    }
}

int main()
{
    bn::core::init();
    bn::bg_palettes::set_transparent_color(bn::color(14, 22, 31));

    bn::affine_bg_ptr track = bn::affine_bg_items::track.create_bg(0, 0);
    track.set_wrapping_enabled(false);
    bn::sprite_ptr car = bn::sprite_items::car.create_sprite(0, 52);

    // The four registers that place an affine background, rewritten on every line by H-Blank effects.
    floor_lines floor;
    bn::fixed_point camera(560, 860);
    int angle = 0;
    update_floor(camera, angle, floor);
    bn::affine_bg_pa_register_hbe_ptr pa_hbe = bn::affine_bg_pa_register_hbe_ptr::create(track, floor.pa);
    bn::affine_bg_pc_register_hbe_ptr pc_hbe = bn::affine_bg_pc_register_hbe_ptr::create(track, floor.pc);
    bn::affine_bg_dx_register_hbe_ptr dx_hbe = bn::affine_bg_dx_register_hbe_ptr::create(track, floor.dx);
    bn::affine_bg_dy_register_hbe_ptr dy_hbe = bn::affine_bg_dy_register_hbe_ptr::create(track, floor.dy);

    while(true)
    {
        int turn = bn::keypad::left_held() - bn::keypad::right_held();
        angle = (angle + turn + 360) % 360;
        car.set_tiles(bn::sprite_items::car.tiles_item(), turn != 0);
        car.set_horizontal_flip(turn > 0);   // The turning frame leans right; flipped, it leans left.

        int drive = bn::keypad::up_held() - bn::keypad::down_held();
        camera += bn::fixed_point(bn::degrees_lut_cos(angle), -bn::degrees_lut_sin(angle)) * (drive * 2);

        update_floor(camera, angle, floor);
        pa_hbe.reload_values_ref();
        pc_hbe.reload_values_ref();
        dx_hbe.reload_values_ref();
        dy_hbe.reload_values_ref();
        bn::core::update();
    }
}
