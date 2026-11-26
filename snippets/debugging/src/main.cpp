#include "bn_assert.h"
#include "bn_bg_palettes.h"
#include "bn_color.h"
#include "bn_core.h"
#include "bn_format.h"
#include "bn_keypad.h"
#include "bn_log.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "font.h"
#include "hero_walk.h"

constexpr int speed = 1;

// Checks its caller: a hit bigger than the health left is a bug somewhere else, so stop right here.
[[gnu::noinline]] int damage(int health, int amount)
{
    BN_ASSERT(amount <= health, "Hit for ", amount, " with ", health, " health left");
    return health - amount;
}

int main()
{
    bn::core::init();
    bn::bg_palettes::set_transparent_color(bn::color(4, 10, 6));

    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(0, 16);
    hero_walk walk;
    bool was_walking = false;
    int health = 3;
    int frame = 0;

    bn::sprite_text_generator text_generator(hud::font);
    bn::vector<bn::sprite_ptr, 16> text;
    text_generator.generate(-112, -72, "A: log   B: hurt", text);
    int fixed_text = text.size();
    text_generator.generate(-112, -56, "Health 3", text);

    BN_LOG("Started. Health: ", health);

    while(true)
    {
        ++frame;
        int step_x = bn::keypad::right_held() - bn::keypad::left_held();
        int step_y = bn::keypad::down_held() - bn::keypad::up_held();
        hero.set_position(hero.x() + step_x * speed, hero.y() + step_y * speed);
        walk.update(hero, step_x, step_y, speed);

        // Log what changed, not every frame: one line when the hero stops, not sixty a second.
        bool walking = step_x != 0 || step_y != 0;

        if(was_walking && ! walking)
        {
            BN_LOG("Stopped at ", hero.x(), ", ", hero.y());
        }

        was_walking = walking;

        if(bn::keypad::a_pressed())
        {
            BN_LOG("Frame ", frame, ": health ", health);
        }

        // The fourth hit asks for more health than is left: the error screen, on purpose.
        if(bn::keypad::b_pressed())
        {
            health = damage(health, 1);
            BN_LOG("Hurt. Health: ", health);
            text.shrink(fixed_text);
            text_generator.generate(-112, -56, bn::format<16>("Health {}", health), text);
        }

        bn::core::update();
    }
}
