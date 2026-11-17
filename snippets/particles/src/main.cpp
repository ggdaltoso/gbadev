#include "bn_core.h"
#include "bn_fixed_rect.h"
#include "bn_format.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_random.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_world.h"
#include "bn_sprite_items_coin.h"
#include "bn_sprite_items_hero.h"
#include "bn_sprite_items_spark.h"

#include "hero_walk.h"

#include "font.h"

// A spark flies from where it was born along its way, slowing down, for spark_frames frames.
struct spark
{
    bn::sprite_ptr sprite;
    bn::fixed_point from;
    bn::fixed_point way;
    int age = 0;
};

constexpr int frames_per_graphic = 8;
constexpr int spark_frames = frames_per_graphic * 4;

bn::fixed ease_out(bn::fixed t)
{
    bn::fixed left = 1 - t;
    return 1 - left.safe_multiplication(left);
}

// Adds count sparks at position, each in a random direction and distance. When there's no room,
// the oldest spark goes, so a new burst always shows. bn::ivector takes a vector of any capacity.
void burst(bn::ivector<spark>& sparks, bn::random& random, const bn::fixed_point& position, int count)
{
    for(int index = 0; index < count; ++index)
    {
        if(sparks.full())
        {
            sparks.erase(sparks.begin());
        }

        auto [sin, cos] = bn::degrees_lut_sin_and_cos(random.get_int(360));
        bn::fixed distance = random.get_fixed(12, 32);
        bn::fixed_point way(cos.safe_multiplication(distance), sin.safe_multiplication(distance));
        sparks.push_back(spark{ bn::sprite_items::spark.create_sprite(position), position, way });
    }
}

int main()
{
    bn::core::init();

    bn::regular_bg_ptr world = bn::regular_bg_items::world.create_bg(0, 0);
    bn::sprite_text_generator text_generator(hud::font);
    text_generator.set_center_alignment();

    // Two generators: the sparks' numbers never change where the coins go.
    bn::random random;
    bn::random spark_random;

    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(0, 30);
    hero_walk walk;

    bn::vector<bn::sprite_ptr, 3> coins;
    coins.push_back(bn::sprite_items::coin.create_sprite(-60, 32));
    coins.push_back(bn::sprite_items::coin.create_sprite(50, 32));
    coins.push_back(bn::sprite_items::coin.create_sprite(100, 32));

    bn::vector<spark, 48> sparks;
    int shown_count = -1;
    bn::vector<bn::sprite_ptr, 8> count_sprites;
    bn::vector<bn::sprite_ptr, 8> hint_sprites;
    text_generator.generate(0, 70, "A: burst", hint_sprites);

    while(true)
    {
        int step_x = bn::keypad::right_held() - bn::keypad::left_held();
        int step_y = bn::keypad::down_held() - bn::keypad::up_held();
        hero.set_position(hero.position() + bn::fixed_point(step_x, step_y));

        walk.update(hero, step_x, step_y);

        if(bn::keypad::a_pressed())
        {
            burst(sparks, spark_random, hero.position(), 24);
        }

        bn::fixed_rect hero_box(hero.position() + bn::fixed_point(0, 4), bn::fixed_size(12, 14));

        for(bn::sprite_ptr& coin : coins)
        {
            bn::fixed_rect coin_box(coin.position() + bn::fixed_point(-1, -1), bn::fixed_size(10, 10));

            if(coin_box.intersects(hero_box))
            {
                burst(sparks, spark_random, coin.position(), 12);
                coin.set_position(random.get_int(-100, 101), random.get_int(-40, 61));
            }
        }

        // The position comes from the age, like a tween. Every 8 frames the age also picks a smaller star.
        for(spark& particle : sparks)
        {
            ++particle.age;
            bn::fixed t = bn::fixed(particle.age) / spark_frames;
            bn::fixed progress = ease_out(t);
            bn::fixed_point offset(particle.way.x().safe_multiplication(progress),
                                   particle.way.y().safe_multiplication(progress));
            particle.sprite.set_position(particle.from + offset);
            // set_tiles costs even when nothing changes, so it only runs when the frame does.
            if(particle.age % frames_per_graphic == 0 && particle.age < spark_frames)
            {
                particle.sprite.set_tiles(bn::sprite_items::spark.tiles_item(), particle.age / frames_per_graphic);
            }
        }

        bn::erase_if(sparks, [](const spark& particle)
        {
            return particle.age == spark_frames;
        });

        if(sparks.size() != shown_count)
        {
            shown_count = sparks.size();
            count_sprites.clear();
            text_generator.generate(0, -70, bn::format<16>("Sparks: {}", shown_count), count_sprites);
        }

        bn::core::update();
    }
}
