#include "bn_array.h"
#include "bn_camera_ptr.h"
#include "bn_core.h"
#include "bn_fixed_rect.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_animate_actions.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_string_view.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_box.h"
#include "bn_regular_bg_items_world.h"
#include "bn_sprite_items_hero.h"
#include "bn_sprite_items_log.h"

#include "hero_walk.h"

#include "font.h"

// What a step of the cutscene does. Each one knows how to start, when it's done, and where it ends.
enum class action
{
    say,            // Show text in the box until A.
    pan_to_log,     // Move the camera to the log.
    pan_back,       // Move the camera back to the hero.
    wait,           // Do nothing for some frames.
    wake_log        // The log sits up.
};

struct step
{
    action what;
    bn::string_view text = "";
    int frames = 0;
};

// The cutscene is data: a list read top to bottom. Changing it doesn't change the code that plays it.
constexpr auto script = bn::to_array<step>({
    { action::say, "Hero: Is someone snoring?" },
    { action::pan_to_log },
    { action::wait, "", 60 },
    { action::wake_log },
    { action::wait, "", 30 },
    { action::say, "Log: Who woke me up?" },
    { action::say, "Log: Go away. Let me sleep." },
    { action::pan_back },
    { action::say, "Hero: Okay, okay..." },
});

constexpr bn::fixed_point log_position(196, 96);
constexpr int pan_speed = 2;

// The camera follows the hero, but stays inside the 512x512 world.
bn::fixed_point camera_target(const bn::fixed_point& hero_position)
{
    return bn::fixed_point(bn::clamp(hero_position.x(), bn::fixed(-136), bn::fixed(136)),
                           bn::clamp(hero_position.y(), bn::fixed(-176), bn::fixed(176)));
}

// A step closer to target, at most pan_speed on each axis. Returns true once it's there.
bool pan(bn::camera_ptr& camera, const bn::fixed_point& target)
{
    bn::fixed_point gap = target - camera.position();
    camera.set_position(camera.position() + bn::fixed_point(bn::clamp(gap.x(), bn::fixed(-pan_speed), bn::fixed(pan_speed)),
                                                            bn::clamp(gap.y(), bn::fixed(-pan_speed), bn::fixed(pan_speed))));
    return camera.position() == target;
}

int main()
{
    bn::core::init();

    bn::camera_ptr camera = bn::camera_ptr::create(0, 0);
    bn::regular_bg_ptr world = bn::regular_bg_items::world.create_bg(0, 0);
    world.set_camera(camera);

    bn::sprite_ptr log = bn::sprite_items::log.create_sprite(log_position);
    log.set_camera(camera);
    bn::sprite_animate_action<4> snore = bn::create_sprite_animate_action_forever(
            log, 12, bn::sprite_items::log.tiles_item(), 4, 5, 6, 7);
    bool log_awake = false;

    auto wake_log = [&]()
    {
        log_awake = true;
        log.set_tiles(bn::sprite_items::log.tiles_item(), 8);
    };

    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(-100, -50);
    hero.set_camera(camera);
    hero_walk walk;
    camera.set_position(camera_target(hero.position()));

    // The box and its text have no camera: they stay on the screen, not in the world.
    bn::regular_bg_ptr box = bn::regular_bg_items::box.create_bg(0, 0);
    box.set_priority(0);
    box.set_visible(false);
    bn::sprite_text_generator text_generator(hud::font);
    text_generator.set_bg_priority(0);
    bn::vector<bn::sprite_ptr, 8> text_sprites;

    // Walking into the field starts the cutscene, once.
    constexpr bn::fixed_rect field(bn::fixed_point(0, -16), bn::fixed_size(72, 56));
    bool played = false;

    int current = -1;       // The step being played. -1: no cutscene.
    int step_frames = 0;    // Frames the current step has run.

    while(true)
    {
        if(current < 0)
        {
            int step_x = bn::keypad::right_held() - bn::keypad::left_held();
            int step_y = bn::keypad::down_held() - bn::keypad::up_held();
            hero.set_position(hero.position() + bn::fixed_point(step_x, step_y));
            walk.update(hero, step_x, step_y);
            camera.set_position(camera_target(hero.position()));

            if(! played && field.contains(hero.position()))
            {
                played = true;
                current = 0;
                step_frames = 0;
            }
        }
        else if(bn::keypad::start_pressed())
        {
            // Skipping runs what the remaining steps leave behind, so the world ends up the same
            // as when the cutscene plays. Text, waits and pans leave nothing: the box goes away,
            // and the camera follows the hero again.
            for(int index = current; index < script.size(); ++index)
            {
                if(script[index].what == action::wake_log)
                {
                    wake_log();
                }
            }

            text_sprites.clear();
            box.set_visible(false);
            current = -1;
        }
        else
        {
            walk.update(hero, 0, 0);   // No walking during the cutscene: the hero stands.
            const step& now = script[current];
            bool done = false;

            switch(now.what)
            {

            case action::say:
                if(step_frames == 0)
                {
                    box.set_visible(true);
                    text_generator.generate(-110, 46, now.text, text_sprites);
                }
                else if(bn::keypad::a_pressed())
                {
                    // Never on the step's first frame: a box only closes after it has been on screen.
                    text_sprites.clear();
                    box.set_visible(false);
                    done = true;
                }
                break;

            case action::pan_to_log:
                done = pan(camera, camera_target(log_position));
                break;

            case action::pan_back:
                done = pan(camera, camera_target(hero.position()));
                break;

            case action::wait:
                done = step_frames == now.frames;
                break;

            case action::wake_log:
                wake_log();
                done = true;
                break;

            default:
                BN_ERROR("Unknown action");
                break;
            }

            ++step_frames;

            // One step per frame at most: the next one starts on the next frame.
            if(done)
            {
                ++current;
                step_frames = 0;

                if(current == script.size())
                {
                    current = -1;
                }
            }
        }

        if(! log_awake)
        {
            snore.update();
        }

        bn::core::update();
    }
}
