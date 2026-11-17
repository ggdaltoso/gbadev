#include "bn_core.h"
#include "bn_fixed_rect.h"
#include "bn_format.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_optional.h"
#include "bn_regular_bg_ptr.h"
#include "bn_seed_random.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_world.h"
#include "bn_sprite_items_coin.h"
#include "bn_sprite_items_hero.h"

#include "hero_walk.h"

#include "font.h"

// The buttons the game reads in one frame, one bit each.
enum key_bits
{
    up_bit = 1, down_bit = 2, left_bit = 4, right_bit = 8, run_bit = 16
};

int read_keypad()
{
    return (bn::keypad::up_held() ? up_bit : 0) | (bn::keypad::down_held() ? down_bit : 0) |
            (bn::keypad::left_held() ? left_bit : 0) | (bn::keypad::right_held() ? right_bit : 0) |
            (bn::keypad::b_held() ? run_bit : 0);
}

// The same buttons held for some frames in a row. Buttons change a few times a second, not every frame.
struct run
{
    uint8_t keys;
    uint8_t frames;
};

enum class mode
{
    idle,
    recording,
    replaying
};

// Everything the game is: a replay starts from exactly this, with the same seed.
struct game
{
    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(0, 0);
    hero_walk walk;
    bn::vector<bn::sprite_ptr, 5> coins;
    bn::seed_random random;
    int score = 0;

    explicit game(unsigned seed) :
        random(seed)
    {
        while(! coins.full())
        {
            coins.push_back(bn::sprite_items::coin.create_sprite(random_spot()));
        }
    }

    bn::fixed_point random_spot()
    {
        return bn::fixed_point(random.get_int(-104, 105), random.get_int(-56, 57));
    }

    // One frame of the game. It only knows the buttons through keys.
    void update(int keys)
    {
        int speed = keys & run_bit ? 2 : 1;
        int step_x = (keys & right_bit ? 1 : 0) - (keys & left_bit ? 1 : 0);
        int step_y = (keys & down_bit ? 1 : 0) - (keys & up_bit ? 1 : 0);
        bn::fixed x = bn::clamp(hero.x() + step_x * speed, bn::fixed(-112), bn::fixed(112));
        bn::fixed y = bn::clamp(hero.y() + step_y * speed, bn::fixed(-64), bn::fixed(64));
        hero.set_position(x, y);
        walk.update(hero, step_x, step_y, speed);

        bn::fixed_rect hero_box(hero.position() + bn::fixed_point(0, 4), bn::fixed_size(12, 14));

        for(bn::sprite_ptr& coin : coins)
        {
            if(bn::fixed_rect(coin.position(), bn::fixed_size(10, 10)).intersects(hero_box))
            {
                coin.set_position(random_spot());
                ++score;
            }
        }
    }
};

int main()
{
    bn::core::init();

    bn::regular_bg_ptr world = bn::regular_bg_items::world.create_bg(0, 0);
    bn::sprite_text_generator text_generator(hud::font);
    text_generator.set_center_alignment();

    // The game lives in an optional: emplace destroys the old one and builds a new one at the start.
    unsigned seed = 0;
    bn::optional<game> current;
    current.emplace(seed);

    bn::vector<run, 256> recording;
    int replay_run = 0;
    int replay_frame = 0;
    mode now = mode::idle;
    unsigned frames = 0;

    bn::vector<bn::sprite_ptr, 16> top_sprites;
    bn::vector<bn::sprite_ptr, 16> bottom_sprites;
    int shown_score = -1;
    bool text_changed = true;

    while(true)
    {
        ++frames;

        // START drives the recorder. It isn't one of the game's buttons, so it's never recorded.
        // A full recording stops too, before this frame's buttons are read.
        if(bn::keypad::start_pressed() || (now == mode::recording && recording.full()))
        {
            if(now == mode::recording)
            {
                now = mode::replaying;
            }
            else
            {
                // A new seed for each recording: nobody presses START on the same frame twice.
                seed = frames * 2654435761u;
                recording.clear();
                now = mode::recording;
            }

            // Recording and replaying both start from a new game with the same seed.
            current.emplace(seed);
            replay_run = 0;
            replay_frame = 0;
            text_changed = true;
        }

        int keys = read_keypad();

        if(now == mode::replaying)
        {
            // The buttons come from the recording instead of the keypad.
            keys = recording[replay_run].keys;

            if(++replay_frame == recording[replay_run].frames)
            {
                ++replay_run;
                replay_frame = 0;
            }
        }
        else if(now == mode::recording)
        {
            // Same buttons as the last run: one more frame, while the count fits in a uint8_t.
            if(! recording.empty() && recording.back().keys == keys && recording.back().frames < 255)
            {
                ++recording.back().frames;
            }
            else
            {
                recording.push_back(run{ uint8_t(keys), 1 });
            }
        }

        current->update(keys);

        if(now == mode::replaying && replay_run == recording.size())
        {
            now = mode::idle;   // The replay is over: the game goes on from where it ended.
            text_changed = true;
        }

        if(text_changed)
        {
            text_changed = false;
            top_sprites.clear();

            if(now == mode::recording)
            {
                text_generator.generate(0, -70, "Recording   START: replay", top_sprites);
            }
            else if(now == mode::replaying)
            {
                text_generator.generate(0, -70, bn::format<32>("Replay of {} runs", recording.size()), top_sprites);
            }
            else
            {
                text_generator.generate(0, -70, "START: record", top_sprites);
            }
        }

        if(current->score != shown_score)
        {
            shown_score = current->score;
            bottom_sprites.clear();
            text_generator.generate(0, 70, bn::format<16>("Coins: {}", shown_score), bottom_sprites);
        }

        bn::core::update();
    }
}
