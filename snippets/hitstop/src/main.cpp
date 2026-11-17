#include "bn_core.h"
#include "bn_format.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_optional.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_animate_actions.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_ground.h"
#include "bn_sprite_items_hero.h"
#include "bn_sprite_items_log.h"
#include "bn_sprite_items_spark.h"

#include "font.h"

constexpr int hitstop_frames = 6;    // 0.1 seconds: long enough to feel, short enough not to look like lag.
constexpr int frames_per_pose = 4;   // The swing is poses 1 to 4 of the hero, 4 frames each.
constexpr int swing_frames = frames_per_pose * 4;
constexpr int hit_frame = frames_per_pose;   // The first frame of pose 2: the sword straight out.

void show_text(bn::sprite_text_generator& generator, bn::vector<bn::sprite_ptr, 16>& sprites,
               int hits, bool hitstop_on)
{
    sprites.clear();
    generator.generate(-112, -72, bn::format<24>("Hits {}", hits), sprites);
    generator.generate(-112, 70, hitstop_on ? "A: swing   B: hitstop on" : "A: swing   B: hitstop off", sprites);
}

int main()
{
    bn::core::init();

    bn::regular_bg_ptr ground = bn::regular_bg_items::ground.create_bg(0, 0);
    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(-24, 0);
    bn::sprite_ptr dummy = bn::sprite_items::log.create_sprite(0, 0);
    dummy.set_horizontal_flip(true);
    bn::sprite_animate_action<4> dummy_steps = bn::create_sprite_animate_action_forever(
                dummy, 8, bn::sprite_items::log.tiles_item(), 0, 1, 2, 3);

    bn::sprite_ptr spark = bn::sprite_items::spark.create_sprite(0, 0);
    spark.set_visible(false);
    bn::optional<bn::sprite_animate_action<4>> spark_flash;

    bn::sprite_text_generator text_generator(hud::font);
    bn::vector<bn::sprite_ptr, 16> text_sprites;

    int hits = 0;
    bool hitstop_on = true;
    show_text(text_generator, text_sprites, hits, hitstop_on);

    int freeze = 0;          // Frames of hitstop left.
    int swing = -1;          // Frames into the swing, or -1 when not swinging.
    bool wants_swing = false;

    while(true)
    {
        // Input is read every frame, frozen or not, so a press during hitstop isn't lost.
        if(bn::keypad::a_pressed())
        {
            wants_swing = true;
        }

        if(bn::keypad::b_pressed())
        {
            hitstop_on = ! hitstop_on;
            show_text(text_generator, text_sprites, hits, hitstop_on);
        }

        // The spark is the hit itself, so it plays through the hitstop.
        if(spark_flash)
        {
            spark_flash->update();

            if(spark_flash->done())
            {
                spark_flash.reset();
                spark.set_visible(false);
            }
        }

        if(freeze > 0)
        {
            // Hitstop: nothing in the world moves, not even its animations.
            --freeze;
        }
        else
        {
            dummy_steps.update();

            if(swing >= 0)
            {
                hero.set_tiles(bn::sprite_items::hero.tiles_item(), 1 + swing / frames_per_pose);

                // One check per swing, on the frame the sword is out.
                if(swing == hit_frame)
                {
                    bn::fixed_point gap = dummy.position() - hero.position();
                    bn::fixed reach = hero.horizontal_flip() ? -gap.x() : gap.x();

                    if(reach > 0 && reach < 28 && bn::abs(gap.y()) < 16)
                    {
                        ++hits;
                        show_text(text_generator, text_sprites, hits, hitstop_on);

                        spark.set_position(hero.x() + (hero.horizontal_flip() ? -14 : 14), hero.y() + 2);
                        spark.set_visible(true);
                        spark_flash = bn::create_sprite_animate_action_once(
                                    spark, 2, bn::sprite_items::spark.tiles_item(), 0, 1, 2, 3);

                        if(hitstop_on)
                        {
                            freeze = hitstop_frames;
                        }
                    }
                }

                if(++swing == swing_frames)
                {
                    swing = -1;
                    hero.set_tiles(bn::sprite_items::hero.tiles_item(), 0);
                }
            }
            else
            {
                int step_x = bn::keypad::right_held() - bn::keypad::left_held();
                int step_y = bn::keypad::down_held() - bn::keypad::up_held();
                hero.set_position(hero.position() + bn::fixed_point(step_x, step_y));

                if(step_x != 0)
                {
                    hero.set_horizontal_flip(step_x < 0);
                }

                if(wants_swing)
                {
                    wants_swing = false;
                    swing = 0;
                }
            }
        }

        bn::core::update();
    }
}
