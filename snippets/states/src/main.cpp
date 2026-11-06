#include "bn_core.h"
#include "bn_assert.h"
#include "bn_format.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_animate_actions.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_string_view.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_world.h"
#include "bn_sprite_items_hero.h"
#include "bn_sprite_items_log.h"

#include "font.h"

// The log is always in exactly one of these. A new behavior is a new value here.
enum class log_state
{
    SLEEP,
    ALERT,
    CHASE,
    RETURN
};

// Same order as log_state, to show its name.
constexpr bn::string_view state_names[] = { "sleep", "alert", "chase", "return" };

[[nodiscard]] bool near(const bn::fixed_point& a, const bn::fixed_point& b, int range)
{
    bn::fixed_point distance = b - a;
    return bn::abs(distance.x()) < range && bn::abs(distance.y()) < range;
}

class guard_log
{

public:
    explicit guard_log(const bn::fixed_point& home) :
        _home(home),
        _sprite(bn::sprite_items::log.create_sprite(home)),
        _animation(_sleep_animation())
    {
    }

    [[nodiscard]] log_state state() const
    {
        return _state;
    }

    [[nodiscard]] bn::fixed_point position() const
    {
        return _sprite.position();
    }

    void update(const bn::fixed_point& hero_position)
    {
        ++_state_frames;

        // One case per state. Each one decides only whether to stay or where to go next.
        switch(_state)
        {
            case log_state::SLEEP:
                if(near(position(), hero_position, 48))
                {
                    _enter(log_state::ALERT);
                }
                break;

            case log_state::ALERT:
                if(_state_frames == 30)
                {
                    _enter(log_state::CHASE);
                }
                break;

            case log_state::CHASE:
                _move_toward(hero_position, bn::fixed(0.75));

                if(! near(position(), hero_position, 96))
                {
                    _enter(log_state::RETURN);
                }
                break;

            case log_state::RETURN:
                _move_toward(_home, bn::fixed(0.5));

                if(near(position(), hero_position, 48))
                {
                    _enter(log_state::ALERT);
                }
                else if(position() == _home)
                {
                    _enter(log_state::SLEEP);
                }
                break;

            default:
                BN_ERROR("Unknown state: ", static_cast<int>(_state));
                break;
        }

        _animation.update();
    }

private:
    bn::fixed_point _home;
    bn::sprite_ptr _sprite;
    bn::sprite_animate_action<4> _animation;
    log_state _state = log_state::SLEEP;
    int _state_frames = 0;

    [[nodiscard]] bn::sprite_animate_action<4> _sleep_animation() const
    {
        return bn::create_sprite_animate_action_forever(_sprite, 12, bn::sprite_items::log.tiles_item(), 4, 5, 6, 7);
    }

    // The only place that changes state: what happens once, on the way in, lives here.
    void _enter(log_state state)
    {
        _state = state;
        _state_frames = 0;

        if(state == log_state::SLEEP)
        {
            _animation = _sleep_animation();
        }
        else if(state == log_state::ALERT)
        {
            _animation = bn::create_sprite_animate_action_forever(
                    _sprite, 12, bn::sprite_items::log.tiles_item(), 8, 8, 8, 8);
        }
        else
        {
            _animation = bn::create_sprite_animate_action_forever(
                    _sprite, 8, bn::sprite_items::log.tiles_item(), 0, 1, 2, 3);
        }
    }

    void _move_toward(const bn::fixed_point& target, bn::fixed speed)
    {
        bn::fixed_point distance = target - position();
        bn::fixed step_x = bn::clamp(distance.x(), -speed, speed);
        bn::fixed step_y = bn::clamp(distance.y(), -speed, speed);
        _sprite.set_position(position() + bn::fixed_point(step_x, step_y));

        if(step_x != 0)
        {
            _sprite.set_horizontal_flip(step_x < 0);
        }
    }
};

int main()
{
    bn::core::init();

    bn::regular_bg_ptr world = bn::regular_bg_items::world.create_bg(0, 0);
    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(64, 24);
    guard_log log(bn::fixed_point(-64, 0));

    bn::sprite_text_generator text_generator(hud::font);
    text_generator.set_center_alignment();
    bn::vector<bn::sprite_ptr, 4> state_sprites;
    bn::vector<bn::sprite_ptr, 1> alert_sprites;
    bool first_frame = true;
    log_state shown_state = log_state::SLEEP;

    while(true)
    {
        int wanted_x = hero.x().integer() + (bn::keypad::right_held() - bn::keypad::left_held()) * 2;
        int wanted_y = hero.y().integer() + (bn::keypad::down_held() - bn::keypad::up_held()) * 2;
        hero.set_position(bn::clamp(wanted_x, -104, 104), bn::clamp(wanted_y, -64, 64));

        log.update(hero.position());

        if(first_frame || log.state() != shown_state)
        {
            first_frame = false;
            shown_state = log.state();

            // enum class doesn't turn into a number by itself: the cast says it's on purpose.
            int state_index = static_cast<int>(shown_state);
            state_sprites.clear();
            text_generator.generate(0, -70, bn::format<16>("Log: {}", state_names[state_index]), state_sprites);

            alert_sprites.clear();

            if(shown_state == log_state::ALERT)
            {
                text_generator.generate(0, 0, "!", alert_sprites);
            }
        }

        // The "!" follows the log while it's shown.
        for(bn::sprite_ptr& sprite : alert_sprites)
        {
            sprite.set_position(log.position() + bn::fixed_point(0, -24));
        }

        bn::core::update();
    }
}
