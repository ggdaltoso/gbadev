#include "bn_core.h"
#include "bn_format.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_level.h"
#include "bn_sound_items.h"
#include "bn_sprite_items_beam.h"
#include "bn_sprite_items_player.h"
#include "bn_sprite_items_target.h"

#include "font.h"

// Every beam warns for this long, then hurts for this long. One second of warning: 6 frames are
// enough to walk out of a beam, the rest is time to notice it.
constexpr int warning_frames = 60;
constexpr int active_frames = 20;

// The schedule: when each beam starts, in frames since the loop began, and where. Sorted by time.
struct scheduled_beam
{
    int start;
    int x;
};

constexpr scheduled_beam schedule[] = {
    { 0, -60 }, { 50, 40 }, { 100, -10 }, { 130, 80 }, { 160, -90 },
    { 210, 0 }, { 230, 30 }, { 250, -30 }, { 300, 60 }, { 320, -60 },
};

constexpr int loop_frames = 400;
constexpr int ground = 44;

// A beam that has started. Its phase isn't stored: it comes from its age.
struct beam
{
    int x;
    int age = 0;
    bn::sprite_ptr target;
    bn::sprite_ptr top;
    bn::sprite_ptr bottom;
};

int main()
{
    bn::core::init();

    bn::regular_bg_ptr level = bn::regular_bg_items::level.create_bg(0, 0);
    bn::sprite_ptr player = bn::sprite_items::player.create_sprite(0, ground - 12);
    bn::vector<beam, 8> beams;
    int frame = 0;
    int next = 0;
    int hits = 0;
    int safe_frames = 0;   // After a hit, a moment when beams don't hurt.

    bn::sprite_text_generator text_generator(hud::font);
    text_generator.set_center_alignment();
    bn::vector<bn::sprite_ptr, 4> text_sprites;
    text_generator.generate(0, -70, "Hits: 0", text_sprites);

    while(true)
    {
        int direction = bn::keypad::right_held() - bn::keypad::left_held();
        player.set_x(bn::clamp(player.x() + direction * 2, bn::fixed(-110), bn::fixed(110)));

        if(direction != 0)
        {
            player.set_horizontal_flip(direction > 0);   // The art faces left.
        }

        // Start every beam whose time has come. <= and not ==, so a skipped frame never loses one.
        while(next < int(sizeof(schedule) / sizeof(schedule[0])) && schedule[next].start <= frame)
        {
            int x = schedule[next].x;
            beams.push_back(beam{ x, 0, bn::sprite_items::target.create_sprite(x, ground + 8),
                                  bn::sprite_items::beam.create_sprite(x, -28),
                                  bn::sprite_items::beam.create_sprite(x, 12) });
            beams.back().top.set_visible(false);
            beams.back().bottom.set_visible(false);
            bn::sound_items::beep.play();   // Once, when the warning starts.
            ++next;
        }

        if(safe_frames > 0)
        {
            --safe_frames;
        }

        for(beam& falling : beams)
        {
            bool warning = falling.age < warning_frames;
            bool active = ! warning && falling.age < warning_frames + active_frames;

            // The warning blinks; the beam is only shown, and only hurts, while it's active.
            falling.target.set_visible(warning && falling.age / 6 % 2 == 0);
            falling.top.set_visible(active);
            falling.bottom.set_visible(active);

            if(active && safe_frames == 0 && bn::abs(player.x() - falling.x) < 12)
            {
                ++hits;
                safe_frames = 60;
                text_sprites.clear();
                text_generator.generate(0, -70, bn::format<16>("Hits: {}", hits), text_sprites);
            }

            ++falling.age;
        }

        bn::erase_if(beams, [](const beam& done) { return done.age >= warning_frames + active_frames; });
        player.set_visible(safe_frames / 4 % 2 == 0);

        // The schedule plays again from the top.
        if(++frame >= loop_frames)
        {
            frame = 0;
            next = 0;
        }

        bn::core::update();
    }
}
