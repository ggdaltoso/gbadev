// Include guard: if this header is included twice, the second time is skipped.
#ifndef LOG_ENEMY_H
#define LOG_ENEMY_H

// Only what the class's declaration needs: its members and parameters.
#include "bn_fixed_point.h"
#include "bn_sprite_animate_actions.h"
#include "bn_sprite_ptr.h"

// The declaration: what a log is and what it can do. Whoever includes this can use one.
class log_enemy
{

public:
    explicit log_enemy(const bn::fixed_point& position);

    void update(const bn::fixed_point& hero_position);

    [[nodiscard]] bool asleep() const
    {
        return _asleep;
    }

    [[nodiscard]] bool gone() const
    {
        return _asleep && _sleep_frames == 0;
    }

private:
    bn::sprite_ptr _sprite;
    bn::sprite_animate_action<4> _animation;
    bool _asleep = false;
    int _sleep_frames = 0;
};

#endif
