#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_regular_bg_map_cell_info.h"
#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_tiles_ptr.h"
#include "bn_sprite_ptr.h"

#include "bn_regular_bg_items_plaza.h"
#include "bn_regular_bg_tiles_items_fountain_frames.h"
#include "bn_regular_bg_tiles_items_wave_frames.h"
#include "bn_sprite_items_hero.h"

#include "hero_walk.h"

// One copy of an animation in the map, and its frames: each frame is width x height tiles,
// stacked top to bottom in a tiles item. Every other copy drawn the same way shares its tiles.
struct tile_animation
{
    int map_x;              // The copy's top-left cell, in 8x8 cells.
    int map_y;
    int width;              // Its size, in cells.
    int height;
    const bn::regular_bg_tiles_item& frames;
    int frame_count;
    int frames_per_step;    // Screen frames each animation frame stays.
    int first_frame;        // The frame the map was drawn with.
};

constexpr tile_animation animations[] = {
    { 2, 8, 6, 6, bn::regular_bg_tiles_items::fountain_frames, 3, 8, 0 },  // All three fountains.
    { 6, 18, 2, 2, bn::regular_bg_tiles_items::wave_frames, 8, 10, 0 },    // Half of the pond's waves...
    { 4, 18, 2, 2, bn::regular_bg_tiles_items::wave_frames, 8, 10, 4 },    // ...and the other half.
};

// A tile is 8 words of pixels.
bool same_tile(const bn::tile& a, const bn::tile& b)
{
    for(int index = 0; index < 8; ++index)
    {
        if(a.data[index] != b.data[index])
        {
            return false;
        }
    }

    return true;
}

// Copies a frame over the tiles the copy's cells point at, and every cell in the map that points
// at the same tiles changes with it. Butano copies at most 32 tiles per screen frame, so tiles
// that look the same as in the previous frame, like the fountain's stone rim, are skipped.
void show_frame(bn::regular_bg_tiles_ptr& tiles, const tile_animation& animation, int frame)
{
    bn::span<const bn::tile> frame_tiles = animation.frames.tiles_ref();
    int previous = (frame + animation.frame_count - 1) % animation.frame_count;

    for(int y = 0; y < animation.height; ++y)
    {
        for(int x = 0; x < animation.width; ++x)
        {
            const bn::tile& now = frame_tiles[(frame * animation.height + y) * animation.width + x];
            const bn::tile& before = frame_tiles[(previous * animation.height + y) * animation.width + x];

            if(! same_tile(now, before))
            {
                bn::regular_bg_map_cell cell = bn::regular_bg_items::plaza.map_item().cell(
                            animation.map_x + x, animation.map_y + y);
                bn::regular_bg_map_cell_info info(cell);

                // A flipped cell shares its tile with a mirrored one: see flipped_tiles_reduction.
                BN_ASSERT(! info.horizontal_flip() && ! info.vertical_flip(), "Flipped animated cell");
                tiles.overwrite_tile(info.tile_index(), now);
            }
        }
    }
}

int main()
{
    bn::core::init();

    bn::regular_bg_ptr plaza = bn::regular_bg_items::plaza.create_bg(0, 0);
    bn::regular_bg_tiles_ptr tiles = plaza.tiles();
    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(0, -12);
    hero_walk walk;
    int counter = 0;

    while(true)
    {
        int step_x = bn::keypad::right_held() - bn::keypad::left_held();
        int step_y = bn::keypad::down_held() - bn::keypad::up_held();
        hero.set_position(hero.position() + bn::fixed_point(step_x, step_y));

        walk.update(hero, step_x, step_y);

        // Tiles only get copied when a frame changes, not on every screen frame.
        for(const tile_animation& animation : animations)
        {
            if(counter % animation.frames_per_step == 0)
            {
                int step = counter / animation.frames_per_step;
                show_frame(tiles, animation, (animation.first_frame + step) % animation.frame_count);
            }
        }

        ++counter;
        bn::core::update();
    }
}
