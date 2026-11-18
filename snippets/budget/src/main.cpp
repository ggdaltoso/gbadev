#include "bn_config_sprites.h"
#include "bn_core.h"
#include "bn_format.h"
#include "bn_keypad.h"
#include "bn_random.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_sprites.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_sky.h"
#include "bn_sprite_items_enemies.h"
#include "bn_sprite_items_ship.h"
#include "bn_sprite_items_shot.h"

#include "font.h"

constexpr int columns = 7;
constexpr int rows = 3;

// The budget: Butano has room for BN_CFG_SPRITES_MAX_ITEMS sprites (128), and everything shares it.
constexpr int enemy_sprites = columns * rows;
constexpr int ship_sprites = 1;
constexpr int hud_sprites = 4;   // Measured: "Sprites: 128 of 128" takes 4 sprites of text.
constexpr int max_shots = BN_CFG_SPRITES_MAX_ITEMS - enemy_sprites - ship_sprites - hud_sprites;

struct shot
{
    bn::sprite_ptr sprite;
    int speed;   // Pixels per frame: negative goes up.
};

int main()
{
    bn::core::init();

    bn::regular_bg_ptr sky = bn::regular_bg_items::sky.create_bg(0, 0);
    bn::sprite_ptr ship = bn::sprite_items::ship.create_sprite(0, 64);
    bn::vector<bn::sprite_ptr, enemy_sprites> enemies;

    for(int row = 0; row < rows; ++row)
    {
        for(int column = 0; column < columns; ++column)
        {
            enemies.push_back(bn::sprite_items::enemies.create_sprite((column - 3) * 26, -46 + row * 22, (2 - row) * 2));
        }
    }

    // Shots are made when fired and destroyed when they leave the screen. The vector's capacity is what
    // the budget leaves for them, so a shot that doesn't fit is never made.
    bn::vector<shot, max_shots> shots;
    bn::random random;
    int reload = 0;

    bn::sprite_text_generator text_generator(hud::font);
    text_generator.set_center_alignment();
    bn::vector<bn::sprite_ptr, hud_sprites> text_sprites;
    int shown_used = -1;

    while(true)
    {
        bn::fixed ship_x = ship.x() + (bn::keypad::right_held() - bn::keypad::left_held()) * 2;
        ship.set_x(bn::clamp(ship_x, bn::fixed(-108), bn::fixed(108)));

        if(reload > 0)
        {
            --reload;
        }

        if(bn::keypad::a_held() && reload == 0 && ! shots.full())
        {
            reload = 6;
            bn::sprite_ptr sprite = bn::sprite_items::shot.create_sprite(ship.x(), ship.y() - 12);
            shots.push_back(shot{ sprite, -4 });
        }

        // Each enemy fires now and then. Holding B makes it rain.
        int chance = bn::keypad::b_held() ? 100 : 2;

        for(const bn::sprite_ptr& enemy : enemies)
        {
            if(random.get_int(1000) < chance && ! shots.full())
            {
                bn::sprite_ptr sprite = bn::sprite_items::shot.create_sprite(enemy.x(), enemy.y() + 12);
                sprite.set_vertical_flip(true);
                shots.push_back(shot{ sprite, 2 });
            }
        }

        for(shot& flying : shots)
        {
            flying.sprite.set_y(flying.sprite.y() + flying.speed);
        }

        // Off the screen: destroyed, and its share of the budget is free again.
        bn::erase_if(shots, [](const shot& flying) { return bn::abs(flying.sprite.y()) > 88; });

        int used = bn::sprites::used_items_count();

        if(used != shown_used)
        {
            shown_used = used;
            text_sprites.clear();
            text_generator.generate(0, -70, bn::format<24>("Sprites: {} of {}", used, BN_CFG_SPRITES_MAX_ITEMS),
                                    text_sprites);
        }

        bn::core::update();
    }
}
