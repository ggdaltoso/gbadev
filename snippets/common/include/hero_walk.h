#ifndef HERO_WALK_H
#define HERO_WALK_H

#include "bn_sprite_ptr.h"

#include "bn_sprite_items_hero.h"

// Animates the guide's shared hero as it walks: it faces the way it moves and steps through the
// walk frames of hero.bmp. Moving the hero stays in each snippet; this only picks the frame.
class hero_walk
{

public:
    // sheet is where the frames come from: hero.bmp, or another sheet with the same frames in the same places.
    explicit hero_walk(const bn::sprite_item& sheet = bn::sprite_items::hero) :
        _sheet(sheet)
    {
    }

    // Call once per frame with the step the hero took: -1, 0 or 1 on each axis.
    // speed is how many pixels that step covers: legs move faster when running, so feet don't slide.
    void update(bn::sprite_ptr& hero, int step_x, int step_y, int speed = 1)
    {
        if(step_x != 0)
        {
            // Sideways wins on diagonals: the sheet has no diagonal frames.
            _first_frame = walk_right;
            _flipped = step_x < 0;   // Facing left is facing right, flipped.
        }
        else if(step_y != 0)
        {
            _first_frame = step_y > 0 ? walk_down : walk_up;
            _flipped = false;
        }

        // Standing still shows the first frame of the walk, facing the last way it went.
        bool moving = step_x != 0 || step_y != 0;
        _steps = moving ? _steps + speed : 0;
        int frame = _first_frame + (_steps / frames_per_step) % 4;

        if(frame != _frame)
        {
            _frame = frame;
            hero.set_tiles(_sheet.tiles_item(), frame);
        }

        // Always set, so a flip from walking left never leaks into walking up or down.
        hero.set_horizontal_flip(_flipped);
    }

private:
    // First frames of each walk cycle in hero.bmp (see snippets/tools/hero_sheet.py).
    static constexpr int walk_down = 5;
    static constexpr int walk_right = 9;
    static constexpr int walk_up = 13;
    static constexpr int frames_per_step = 8;   // At 1 pixel a frame, screen frames per walk frame.

    const bn::sprite_item& _sheet;
    int _first_frame = walk_right;
    int _frame = 0;   // Snippets create the hero with frame 0, standing and facing right.
    int _steps = 0;
    bool _flipped = false;
};

#endif
