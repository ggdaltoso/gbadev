#include "bn_core.h"
#include "bn_format.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_profiler.h"
#include "bn_random.h"
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

class walker
{

public:
    explicit walker(const bn::fixed_point& position) :
        _sprite(bn::sprite_items::log.create_sprite(position)),
        _animation(bn::create_sprite_animate_action_forever(
                _sprite, 8, bn::sprite_items::log.tiles_item(), 0, 1, 2, 3))
    {
    }

    [[nodiscard]] bn::fixed_point position() const
    {
        return _sprite.position();
    }

    void move(const bn::fixed_point& step)
    {
        _sprite.set_position(_sprite.position() + step);
    }

    void chase(const bn::fixed_point& target)
    {
        bn::fixed_point distance = target - position();
        bn::fixed step_x = bn::clamp(distance.x(), bn::fixed(-0.5), bn::fixed(0.5));
        bn::fixed step_y = bn::clamp(distance.y(), bn::fixed(-0.5), bn::fixed(0.5));
        move(bn::fixed_point(step_x, step_y));
        _sprite.set_horizontal_flip(distance.x() < 0);
        _animation.update();
    }

private:
    bn::sprite_ptr _sprite;
    bn::sprite_animate_action<4> _animation;
};

int main()
{
    bn::core::init();

    bn::regular_bg_ptr world = bn::regular_bg_items::world.create_bg(0, 0);
    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(0, 0);
    bn::vector<walker, 64> walkers;
    bn::random random;

    bn::sprite_text_generator text_generator(hud::font);
    text_generator.set_center_alignment();
    bn::vector<bn::sprite_ptr, 8> hint_sprites;
    text_generator.generate(0, 70, "A, B: logs  SELECT: split  START: table", hint_sprites);
    bn::vector<bn::sprite_ptr, 8> stats_sprites;
    int frames_to_stats = 0;

    // Split: each frame checks only a quarter of the logs against the others, taking turns.
    bool split = false;
    int phase = 0;

    while(true)
    {
        int wanted_x = hero.x().integer() + (bn::keypad::right_held() - bn::keypad::left_held()) * 2;
        int wanted_y = hero.y().integer() + (bn::keypad::down_held() - bn::keypad::up_held()) * 2;
        hero.set_position(bn::clamp(wanted_x, -104, 104), bn::clamp(wanted_y, -64, 64));

        if(bn::keypad::a_pressed())
        {
            for(int added = 0; added < 8 && ! walkers.full(); ++added)
            {
                int x = random.get_int(-112, 113);
                int y = random.get_bool() ? -80 : 80;
                walkers.emplace_back(bn::fixed_point(x, y));
            }
        }

        if(bn::keypad::b_pressed())
        {
            int remaining = bn::max(walkers.size() - 8, 0);
            walkers.shrink(remaining);
        }

        if(bn::keypad::select_pressed())
        {
            split = ! split;
            frames_to_stats = 0;
        }

        BN_PROFILER_START("chase");

        for(walker& log : walkers)
        {
            log.chase(hero.position());
        }

        BN_PROFILER_STOP();

        BN_PROFILER_START("separate");

        // Every pair of logs, so that they don't pile up: 64 logs are 2016 pairs.
        int step = split ? 4 : 1;
        int first = split ? phase : 0;
        phase = (phase + 1) % 4;

        for(int i = first; i < walkers.size(); i += step)
        {
            for(int j = i + 1; j < walkers.size(); ++j)
            {
                bn::fixed_point distance = walkers[j].position() - walkers[i].position();

                if(bn::abs(distance.x()) < 16 && bn::abs(distance.y()) < 16)
                {
                    // Visited 4 times less often when split, so it pushes 4 times harder.
                    bn::fixed push = bn::fixed(0.25) * step;
                    bn::fixed push_x = distance.x() < 0 ? -push : push;
                    bn::fixed push_y = distance.y() < 0 ? -push : push;
                    walkers[i].move(bn::fixed_point(-push_x, -push_y));
                    walkers[j].move(bn::fixed_point(push_x, push_y));
                }
            }
        }

        BN_PROFILER_STOP();

        // The usage of the frame before this one: 100% is the whole frame.
        --frames_to_stats;

        if(frames_to_stats <= 0)
        {
            BN_PROFILER_START("stats text");

            int cpu_percent = (bn::core::last_cpu_usage() * 100).round_integer();
            stats_sprites.clear();
            bn::string_view mode = split ? "split" : "all";
            text_generator.generate(0, -70, bn::format<32>("Logs: {}  CPU: {}%  {}", walkers.size(), cpu_percent, mode), stats_sprites);
            frames_to_stats = 30;

            BN_PROFILER_STOP();
        }

        if(bn::keypad::start_pressed())
        {
            // Stops the game and shows a table with every block measured so far.
            bn::profiler::show();
        }

        bn::core::update();
    }
}
