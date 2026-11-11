#include "bn_core.h"
#include "bn_fixed_rect.h"
#include "bn_format.h"
#include "bn_keypad.h"
#include "bn_optional.h"
#include "bn_random.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_animate_actions.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_world.h"
#include "bn_sprite_items_box_10x10.h"
#include "bn_sprite_items_box_12x14.h"
#include "bn_sprite_items_box_16x16.h"
#include "bn_sprite_items_box_18x18.h"
#include "bn_sprite_items_box_32x32.h"
#include "bn_sprite_items_coin.h"
#include "bn_sprite_items_hero.h"
#include "bn_sprite_items_log.h"

#include "font.h"

// Where the box sits, from the sprite's center, how big it is, and the outline that draws it.
struct box_shape
{
    bn::fixed_point offset;
    bn::fixed_size size;
    bn::sprite_item outline;
};

// The tight boxes cover what you see. The hero's covers the body and leaves the top of the head out.
constexpr box_shape hero_tight = { bn::fixed_point(0, 4), bn::fixed_size(12, 14), bn::sprite_items::box_12x14 };
constexpr box_shape log_tight = { bn::fixed_point(-2, 2), bn::fixed_size(18, 18), bn::sprite_items::box_18x18 };
constexpr box_shape coin_tight = { bn::fixed_point(-1, -1), bn::fixed_size(10, 10), bn::sprite_items::box_10x10 };

// The whole sprite, transparent corners included.
constexpr box_shape big_whole = { bn::fixed_point(0, 0), bn::fixed_size(32, 32), bn::sprite_items::box_32x32 };
constexpr box_shape coin_whole = { bn::fixed_point(0, 0), bn::fixed_size(16, 16), bn::sprite_items::box_16x16 };

// A sprite with a hitbox. The box isn't stored: it's worked out from the sprite each time.
class body
{

public:
    body(const bn::sprite_item& item, const bn::fixed_point& position,
         const box_shape& tight, const box_shape& whole) :
        _sprite(item.create_sprite(position)),
        _tight(tight),
        _whole(whole)
    {
    }

    [[nodiscard]] bn::sprite_ptr& sprite()
    {
        return _sprite;
    }

    [[nodiscard]] bn::fixed_rect hitbox() const
    {
        const box_shape& shape = _shape();
        return bn::fixed_rect(_sprite.position() + shape.offset, shape.size);
    }

    void set_tight(bool tight)
    {
        _use_tight = tight;
        _outline.reset();
    }

    void update_outline(bool visible)
    {
        if(! visible)
        {
            _outline.reset();
            return;
        }

        if(! _outline)
        {
            _outline = _shape().outline.create_sprite(0, 0);
            _outline->set_z_order(-1);
        }

        _outline->set_position(hitbox().position());
    }

private:
    bn::sprite_ptr _sprite;
    box_shape _tight;
    box_shape _whole;
    bool _use_tight = true;
    bn::optional<bn::sprite_ptr> _outline;

    [[nodiscard]] const box_shape& _shape() const
    {
        return _use_tight ? _tight : _whole;
    }
};

int main()
{
    bn::core::init();

    bn::regular_bg_ptr world = bn::regular_bg_items::world.create_bg(0, 0);
    bn::sprite_text_generator text_generator(hud::font);
    text_generator.set_center_alignment();
    bn::random random;

    body hero(bn::sprite_items::hero, bn::fixed_point(0, 50), hero_tight, big_whole);

    bn::vector<body, 2> logs;
    logs.emplace_back(bn::sprite_items::log, bn::fixed_point(-80, -36), log_tight, big_whole);
    logs.emplace_back(bn::sprite_items::log, bn::fixed_point(80, 8), log_tight, big_whole);
    bn::fixed log_speeds[] = { 0.75, -1 };

    bn::vector<body, 3> coins;
    coins.emplace_back(bn::sprite_items::coin, bn::fixed_point(-60, 50), coin_tight, coin_whole);
    coins.emplace_back(bn::sprite_items::coin, bn::fixed_point(70, -10), coin_tight, coin_whole);
    coins.emplace_back(bn::sprite_items::coin, bn::fixed_point(-20, -60), coin_tight, coin_whole);

    bool tight = true;
    bool outlines = true;
    int coins_taken = 0;
    int hits = 0;
    int safe_frames = 0;

    bn::vector<bn::sprite_ptr, 8> score_sprites;
    bn::vector<bn::sprite_ptr, 12> hint_sprites;
    bool score_changed = true;
    bool hint_changed = true;

    while(true)
    {
        if(bn::keypad::a_pressed())
        {
            tight = ! tight;
            hint_changed = true;
            hero.set_tight(tight);

            for(body& log : logs)
            {
                log.set_tight(tight);
            }

            for(body& coin : coins)
            {
                coin.set_tight(tight);
            }
        }

        if(bn::keypad::b_pressed())
        {
            outlines = ! outlines;
        }

        bn::fixed step_x = bn::keypad::right_held() - bn::keypad::left_held();
        bn::fixed step_y = bn::keypad::down_held() - bn::keypad::up_held();
        hero.sprite().set_position(hero.sprite().position() + bn::fixed_point(step_x, step_y));

        for(int index = 0; index < logs.size(); ++index)
        {
            bn::sprite_ptr& log_sprite = logs[index].sprite();
            log_sprite.set_x(log_sprite.x() + log_speeds[index]);

            if(bn::abs(log_sprite.x()) > 100)
            {
                log_speeds[index] = -log_speeds[index];
            }

            log_sprite.set_horizontal_flip(log_speeds[index] < 0);
        }

        bn::fixed_rect hero_box = hero.hitbox();

        // After a hit, the hero is safe for a second. Otherwise one touch counts once per frame.
        if(safe_frames > 0)
        {
            --safe_frames;
        }
        else
        {
            for(const body& log : logs)
            {
                if(log.hitbox().intersects(hero_box))
                {
                    ++hits;
                    safe_frames = 60;
                    score_changed = true;
                    break;
                }
            }
        }

        hero.sprite().set_visible(safe_frames % 8 < 4);

        for(body& coin : coins)
        {
            if(coin.hitbox().intersects(hero_box))
            {
                ++coins_taken;
                score_changed = true;
                coin.sprite().set_position(random.get_int(-100, 101), random.get_int(-60, 61));
            }
        }

        hero.update_outline(outlines);

        for(body& log : logs)
        {
            log.update_outline(outlines);
        }

        for(body& coin : coins)
        {
            coin.update_outline(outlines);
        }

        if(score_changed)
        {
            score_changed = false;
            score_sprites.clear();
            text_generator.generate(0, -70, bn::format<24>("Coins: {}  Hits: {}", coins_taken, hits), score_sprites);
        }

        if(hint_changed)
        {
            hint_changed = false;
            hint_sprites.clear();
            text_generator.generate(0, 70, bn::format<40>("A: {} boxes  B: outlines",
                    tight ? "tight" : "whole-sprite"), hint_sprites);
        }

        bn::core::update();
    }
}
