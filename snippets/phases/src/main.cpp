#include "bn_algorithm.h"
#include "bn_core.h"
#include "bn_fixed_rect.h"
#include "bn_format.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_regular_bg_ptr.h"
#include "bn_span.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_sky.h"
#include "bn_sprite_items_boss.h"
#include "bn_sprite_items_bullet.h"
#include "bn_sprite_items_ship.h"
#include "bn_sprite_items_shot.h"

#include "font.h"

// One attack: a bullet pattern (see the Bullet patterns page) played for a while.
struct attack
{
    int frames;         // How long it lasts before the next attack of the phase.
    int every;          // Frames between volleys.
    int shots;          // Bullets per volley.
    bn::fixed spread;   // Degrees between them.
    bn::fixed turn;     // Degrees the volley turns after each one.
    bn::fixed speed;
    bool aimed;
};

// A phase is a list of attacks played in a loop, for as long as the boss's health is in its range.
struct phase
{
    int from_health;   // The phase starts when health drops to this.
    bn::span<const attack> attacks;
};

constexpr attack calm[] = {
    { 120, 40, 12, 30, 0, 1, false },      // Rings.
    { 120, 30, 3, 15, 0, 1.25, true },     // Small fans at the player.
};

constexpr attack angry[] = {
    { 150, 5, 2, 180, 11, 1.25, false },   // A spiral.
    { 90, 20, 5, 12, 0, 1.5, true },       // Wider fans, more often.
};

constexpr attack desperate[] = {
    { 180, 16, 8, 45, 9, 1.25, false },    // Turning rings...
    { 60, 10, 7, 10, 0, 1.75, true },      // ...then a fast burst of fans.
};

constexpr int max_health = 200;

// From the top: the first row whose from_health the health is at or below, read from the last one up.
constexpr phase phases[] = {
    { max_health, calm },
    { 133, angry },
    { 66, desperate },
};

constexpr int phase_count = sizeof(phases) / sizeof(phases[0]);
constexpr int shot_damage = 3;

int phase_for(int health)
{
    for(int index = phase_count - 1; index > 0; --index)
    {
        if(health <= phases[index].from_health)
        {
            return index;   // The lowest phase the health has reached.
        }
    }

    return 0;
}

struct bullet
{
    bn::sprite_ptr sprite;
    bn::fixed_point velocity;
};

bn::fixed wrap(bn::fixed degrees)
{
    while(degrees >= 360) { degrees -= 360; }
    while(degrees < 0) { degrees += 360; }
    return degrees;
}

int main()
{
    bn::core::init();

    bn::regular_bg_ptr sky = bn::regular_bg_items::sky.create_bg(0, 0);
    bn::sprite_ptr boss = bn::sprite_items::boss.create_sprite(0, -44);
    bn::sprite_ptr ship = bn::sprite_items::ship.create_sprite(0, 56);
    bn::vector<bullet, 80> bullets;
    bn::vector<bn::sprite_ptr, 8> shots;

    bn::sprite_text_generator text_generator(hud::font);
    bn::vector<bn::sprite_ptr, 16> text;
    bool text_changed = true;

    int health = max_health;
    int current_phase = 0;
    int attack_index = 0;
    int attack_frames = 0;   // Frames spent in the current attack.
    int volley_timer = 0;
    int reload = 0;
    bn::fixed angle = 90;

    while(true)
    {
        int step_x = bn::keypad::right_held() - bn::keypad::left_held();
        ship.set_x(bn::clamp(ship.x() + step_x * 2, bn::fixed(-108), bn::fixed(108)));

        // The player's shots.
        if(reload > 0)
        {
            --reload;
        }
        else if(bn::keypad::a_held() && ! shots.full())
        {
            reload = 12;
            shots.push_back(bn::sprite_items::shot.create_sprite(ship.x(), ship.y() - 16));
        }

        bn::fixed_rect boss_box(boss.position(), bn::fixed_size(32, 20));

        bn::erase_if(shots, [&](bn::sprite_ptr& shot)
        {
            shot.set_y(shot.y() - 4);

            if(health > 0 && boss_box.intersects(bn::fixed_rect(shot.position(), bn::fixed_size(4, 12))))
            {
                health = bn::max(health - shot_damage, 0);
                text_changed = true;
                return true;
            }

            return shot.y() < -88;
        });

        // Health picks the phase, every frame. A new phase starts from its first attack, with a clean screen.
        int new_phase = phase_for(health);

        if(new_phase != current_phase)
        {
            current_phase = new_phase;
            attack_index = 0;
            attack_frames = 0;
            volley_timer = 60;   // A breath before the new phase starts firing.
            bullets.clear();
            text_changed = true;
        }

        const bn::span<const attack>& attacks = phases[current_phase].attacks;

        if(health > 0)
        {
            const attack& now = attacks[attack_index];

            if(--volley_timer <= 0)
            {
                volley_timer = now.every;
                bn::fixed first = angle;

                if(now.aimed)
                {
                    bn::fixed_point to_ship = ship.position() - boss.position();
                    first = bn::degrees_atan2(to_ship.y().round_integer(), to_ship.x().round_integer()) -
                            now.spread * (now.shots - 1) / 2;
                }

                for(int index = 0; index < now.shots && ! bullets.full(); ++index)
                {
                    bn::fixed degrees = wrap(first + now.spread * index);
                    bn::fixed_point velocity(bn::degrees_lut_cos(degrees) * now.speed,
                                             bn::degrees_lut_sin(degrees) * now.speed);
                    bullets.push_back({ bn::sprite_items::bullet.create_sprite(boss.position()), velocity });
                }

                angle = wrap(angle + now.turn);
            }

            if(++attack_frames == now.frames)
            {
                attack_frames = 0;
                attack_index = (attack_index + 1) % attacks.size();
            }
        }

        boss.set_visible(health > 0);

        for(bullet& flying : bullets)
        {
            flying.sprite.set_position(flying.sprite.position() + flying.velocity);
        }

        bn::erase_if(bullets, [](const bullet& flying)
        {
            return bn::abs(flying.sprite.x()) > 124 || bn::abs(flying.sprite.y()) > 84;
        });

        if(text_changed)
        {
            text_changed = false;
            text.clear();
            text_generator.generate(-112, -72, bn::format<16>("Boss {}", health), text);
            text_generator.generate(48, -72, health > 0 ? bn::format<16>("Phase {}", current_phase + 1) :
                                                          bn::format<16>("Defeated"), text);
        }

        bn::core::update();
    }
}
