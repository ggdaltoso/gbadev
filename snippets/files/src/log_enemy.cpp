#include "log_enemy.h"

// What only the definitions need stays here: main.cpp never sees these.
#include "bn_math.h"

#include "bn_sprite_items_log.h"

// log_enemy:: says which class these functions belong to.
log_enemy::log_enemy(const bn::fixed_point& position) :
    _sprite(bn::sprite_items::log.create_sprite(position)),
    _animation(bn::create_sprite_animate_action_forever(
            _sprite, 8, bn::sprite_items::log.tiles_item(), 0, 1, 2, 3))
{
}

void log_enemy::update(const bn::fixed_point& hero_position)
{
    if(_asleep)
    {
        --_sleep_frames;
    }
    else
    {
        bn::fixed_point distance = hero_position - _sprite.position();

        if(bn::abs(distance.x()) < 12 && bn::abs(distance.y()) < 12)
        {
            _asleep = true;
            _sleep_frames = 120;
            _animation = bn::create_sprite_animate_action_forever(
                    _sprite, 12, bn::sprite_items::log.tiles_item(), 4, 5, 6, 7);
        }
        else
        {
            bn::fixed step_x = bn::clamp(distance.x(), bn::fixed(-0.5), bn::fixed(0.5));
            bn::fixed step_y = bn::clamp(distance.y(), bn::fixed(-0.5), bn::fixed(0.5));
            _sprite.set_position(_sprite.position() + bn::fixed_point(step_x, step_y));
            _sprite.set_horizontal_flip(distance.x() < 0);
        }
    }

    _animation.update();
}
