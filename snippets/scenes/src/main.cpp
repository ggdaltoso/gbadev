#include "bn_core.h"
#include "bn_algorithm.h"
#include "bn_assert.h"
#include "bn_bg_palettes.h"
#include "bn_bg_palettes_actions.h"
#include "bn_colors.h"
#include "bn_format.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_palettes.h"
#include "bn_sprite_palettes_actions.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_world.h"
#include "bn_sprite_items_coin.h"
#include "bn_sprite_items_hero.h"

#include "font.h"

enum class scene
{
    TITLE,
    PLAY,
    RESULT
};

constexpr int total_coins = 5;

// A sprite_ptr always points to a real sprite, so it has to be created in the initializer list.
class hero
{

public:
    hero() :
        _sprite(bn::sprite_items::hero.create_sprite(0, 0))
    {
    }

    [[nodiscard]] bn::fixed_point position() const
    {
        return _sprite.position();
    }

    void update()
    {
        int wanted_x = _sprite.x().integer() + (bn::keypad::right_held() - bn::keypad::left_held()) * 2;
        int wanted_y = _sprite.y().integer() + (bn::keypad::down_held() - bn::keypad::up_held()) * 2;
        _sprite.set_position(bn::clamp(wanted_x, -104, 104), bn::clamp(wanted_y, -64, 64));
    }

private:
    bn::sprite_ptr _sprite;
};

void fade_to(bn::fixed intensity)
{
    constexpr int duration_frames = 20;
    bn::bg_palettes_fade_to_action bgs_fade(duration_frames, intensity);
    bn::sprite_palettes_fade_to_action sprites_fade(duration_frames, intensity);

    while(! bgs_fade.done())
    {
        bgs_fade.update();
        sprites_fade.update();
        bn::core::update();
    }
}

// Each scene owns what it creates. When it returns, all of it goes away.
scene title_scene(bn::sprite_text_generator& text_generator)
{
    bn::regular_bg_ptr world = bn::regular_bg_items::world.create_bg(0, 0);
    bn::sprite_ptr player = bn::sprite_items::hero.create_sprite(0, 8);

    bn::vector<bn::sprite_ptr, 8> text_sprites;
    text_generator.generate(0, -48, "Coin dash", text_sprites);
    text_generator.generate(0, 40, "START: play", text_sprites);

    fade_to(0);

    while(! bn::keypad::start_pressed())
    {
        bn::core::update();
    }

    fade_to(1);
    return scene::PLAY;
}

scene play_scene(bn::sprite_text_generator& text_generator, int& coins_taken)
{
    bn::regular_bg_ptr world = bn::regular_bg_items::world.create_bg(0, 0);
    hero player;

    constexpr bn::fixed_point coin_spots[total_coins] = { {-88, -48}, {88, -48}, {-72, 48}, {72, 48}, {0, -56} };
    bn::vector<bn::sprite_ptr, total_coins> coins;

    for(const bn::fixed_point& spot : coin_spots)
    {
        coins.push_back(bn::sprite_items::coin.create_sprite(spot));
    }

    bn::vector<bn::sprite_ptr, 8> text_sprites;
    int frames_left = 10 * 60;
    int shown_seconds = -1;
    coins_taken = 0;

    fade_to(0);

    while(frames_left > 0 && coins_taken < total_coins)
    {
        player.update();
        --frames_left;

        bn::fixed_point hero_position = player.position();
        bn::erase_if(coins, [hero_position](const bn::sprite_ptr& coin)
        {
            bn::fixed_point distance = coin.position() - hero_position;
            return bn::abs(distance.x()) < 14 && bn::abs(distance.y()) < 14;
        });

        int coins_now = total_coins - coins.size();
        int seconds = (frames_left + 59) / 60;

        if(coins_now != coins_taken || seconds != shown_seconds)
        {
            coins_taken = coins_now;
            shown_seconds = seconds;
            text_sprites.clear();
            text_generator.generate(0, -70, bn::format<24>("Coins: {}  Time: {}", coins_taken, seconds), text_sprites);
        }

        bn::core::update();
    }

    fade_to(1);
    return scene::RESULT;
}

scene result_scene(bn::sprite_text_generator& text_generator, int coins_taken)
{
    bn::vector<bn::sprite_ptr, 8> text_sprites;
    text_generator.generate(0, -16, bn::format<24>("Coins: {} of {}", coins_taken, total_coins), text_sprites);
    text_generator.generate(0, 16, "A: again  B: title", text_sprites);

    fade_to(0);
    scene next = scene::RESULT;

    while(next == scene::RESULT)
    {
        if(bn::keypad::a_pressed())
        {
            next = scene::PLAY;
        }
        else if(bn::keypad::b_pressed())
        {
            next = scene::TITLE;
        }

        bn::core::update();
    }

    fade_to(1);
    return next;
}

int main()
{
    bn::core::init();

    // Everything starts black, and each scene fades itself in.
    bn::bg_palettes::set_fade(bn::colors::black, 1);
    bn::sprite_palettes::set_fade(bn::colors::black, 1);

    bn::sprite_text_generator text_generator(hud::font);
    text_generator.set_center_alignment();

    scene next = scene::TITLE;
    int coins_taken = 0;

    while(true)
    {
        switch(next)
        {
            case scene::TITLE:
                next = title_scene(text_generator);
                break;

            case scene::PLAY:
                next = play_scene(text_generator, coins_taken);
                break;

            case scene::RESULT:
                next = result_scene(text_generator, coins_taken);
                break;

            default:
                BN_ERROR("Unknown scene");
                break;
        }
    }
}
