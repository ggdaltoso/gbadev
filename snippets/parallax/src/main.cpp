#include "bn_core.h"
#include "bn_fixed.h"
#include "bn_format.h"
#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_ground.h"
#include "bn_regular_bg_items_hills.h"
#include "bn_regular_bg_items_sky.h"
#include "bn_sprite_items_player.h"

#include "font.h"

// Each layer moves this fraction of what the camera moves. The smaller, the farther away it looks.
constexpr bn::fixed ground_speed = 1;
constexpr bn::fixed hills_speed = 0.5;
constexpr bn::fixed sky_speed = 0.125;

constexpr int world_half_width = 256;   // The layers are 512 pixels wide.
constexpr int camera_limit = world_half_width - 120;   // So the camera never shows past the ground's edges.

int main()
{
    bn::core::init();

    // Priority 0 is drawn in front. The ground must cover the hills, and the hills the sky.
    bn::regular_bg_ptr sky = bn::regular_bg_items::sky.create_bg(0, 0);
    sky.set_priority(3);
    bn::regular_bg_ptr hills = bn::regular_bg_items::hills.create_bg(0, 0);
    hills.set_priority(2);
    bn::regular_bg_ptr ground = bn::regular_bg_items::ground.create_bg(0, 0);
    ground.set_priority(1);

    // Sprites are drawn behind backgrounds of a lower priority number: 3, the default, would hide the player.
    bn::sprite_ptr player = bn::sprite_items::player.create_sprite(0, 32);
    player.set_bg_priority(1);
    bn::fixed player_x = -200;   // In the world. The sprite's x is on the screen.
    int walk_frames = 0;

    bn::sprite_text_generator text_generator(hud::font);
    text_generator.set_center_alignment();
    bn::vector<bn::sprite_ptr, 8> text_sprites;
    bn::fixed shown_camera_x = 1000;

    while(true)
    {
        int direction = bn::keypad::right_held() - bn::keypad::left_held();
        player_x = bn::clamp(player_x + direction * 2, bn::fixed(-world_half_width + 12), bn::fixed(world_half_width - 12));

        // The camera follows the player until the ground's edge comes into view.
        bn::fixed camera_x = bn::clamp(player_x, bn::fixed(-camera_limit), bn::fixed(camera_limit));

        // A layer moved left by the camera times its speed. The same camera, three speeds.
        ground.set_x(-camera_x * ground_speed);
        hills.set_x(-camera_x * hills_speed);
        sky.set_x(-camera_x * sky_speed);
        player.set_x(player_x - camera_x);

        if(direction != 0)
        {
            player.set_horizontal_flip(direction > 0);   // The art faces left.
        }

        walk_frames = direction != 0 ? walk_frames + 1 : 0;
        player.set_tiles(bn::sprite_items::player.tiles_item(), walk_frames / 8 % 2);

        if(camera_x != shown_camera_x)
        {
            shown_camera_x = camera_x;
            text_sprites.clear();
            text_generator.generate(0, -70, bn::format<40>("Ground {}  Hills {}  Sky {}", ground.x().integer(),
                                                           hills.x().integer(), sky.x().integer()), text_sprites);
        }

        bn::core::update();
    }
}
