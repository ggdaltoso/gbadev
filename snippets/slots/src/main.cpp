#include <cstddef>

#include "bn_array.h"
#include "bn_bit.h"
#include "bn_core.h"
#include "bn_format.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_sram.h"
#include "bn_string.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_world.h"
#include "bn_sprite_items_board.h"
#include "bn_sprite_items_coin.h"
#include "bn_sprite_items_hero.h"

#include "font.h"
#include "hero_walk.h"

// One slot. Every slot has the same size, so slot i starts at i * sizeof(slot_data) in SRAM.
struct slot_data
{
    int format;
    int seconds;    // Play time.
    int coins;      // One bit per coin: bit i set means coin i was taken.
    unsigned checksum;   // Last, worked out from every byte before it.
};

constexpr int slot_format = 0x534c5431;
constexpr int slots_count = 3;
constexpr bn::array<bn::fixed_point, 5> coin_spots = { bn::fixed_point(-96, 10), bn::fixed_point(-40, 50),
        bn::fixed_point(30, 20), bn::fixed_point(88, 50), bn::fixed_point(100, -6) };

enum class slot_state
{
    empty,
    damaged,
    used
};

// The slot's bytes up to its checksum, read one by one. Any change, or a write cut in half,
// gives a different number. Unsigned, because it overflows on purpose.
unsigned checksum_of(const slot_data& slot)
{
    const auto* bytes = reinterpret_cast<const unsigned char*>(&slot);
    unsigned sum = 0;

    for(int index = 0; index < int(offsetof(slot_data, checksum)); ++index)
    {
        sum = sum * 31 + bytes[index];
    }

    return sum;
}

int slot_offset(int index)
{
    return index * int(sizeof(slot_data));
}

slot_state read_slot(int index, slot_data& slot)
{
    bn::sram::read_offset(slot, slot_offset(index));

    if(slot.format != slot_format)
    {
        return slot_state::empty;   // A new cart, or a slot never used.
    }

    if(slot.checksum != checksum_of(slot))
    {
        return slot_state::damaged;   // Our slot, but its bytes don't add up.
    }

    return slot_state::used;
}

// The slot is a copy, so setting its format and checksum here doesn't touch the caller's.
void write_slot(int index, slot_data slot)
{
    slot.format = slot_format;
    slot.checksum = checksum_of(slot);
    bn::sram::write_offset(slot, slot_offset(index));
}

bn::string<32> describe(int index)
{
    slot_data slot;

    switch(read_slot(index, slot))
    {

    case slot_state::empty:
        return bn::format<32>("Slot {}: empty", index + 1);

    case slot_state::damaged:
        return bn::format<32>("Slot {}: damaged", index + 1);

    default:
        return bn::format<32>("Slot {}: coins {}, {}s", index + 1, bn::popcount(unsigned(slot.coins)), slot.seconds);
    }
}

int main()
{
    bn::core::init();

    bn::regular_bg_ptr world = bn::regular_bg_items::world.create_bg(0, 0);
    bn::sprite_text_generator text_generator(hud::font);
    text_generator.set_center_alignment();

    bn::vector<bn::sprite_ptr, slots_count> boards;
    bn::vector<bn::sprite_ptr, slots_count> numbers;

    for(int index = 0; index < slots_count; ++index)
    {
        int x = (index - 1) * 72;
        boards.push_back(bn::sprite_items::board.create_sprite(x, -44));
        text_generator.generate(x, -28, bn::to_string<4>(index + 1), numbers);
    }

    bn::vector<bn::sprite_ptr, coin_spots.size()> coins;

    for(bn::fixed_point spot : coin_spots)
    {
        coins.push_back(bn::sprite_items::coin.create_sprite(spot));
    }

    // What a slot keeps: everything the game needs to come back to this moment.
    bn::sprite_ptr hero = bn::sprite_items::hero.create_sprite(0, 20);
    hero_walk walk;
    int coins_taken = 0;
    int frames = 0;

    // Read every slot once, and again only after a save: that's the only time SRAM changes.
    bn::array<bn::string<32>, slots_count> summaries;

    for(int index = 0; index < slots_count; ++index)
    {
        summaries[index] = describe(index);
    }

    bn::vector<bn::sprite_ptr, 16> top_text;
    bn::vector<bn::sprite_ptr, 16> bottom_text;
    bn::string<32> shown;
    int near_board = -1;

    while(true)
    {
        int step_x = bn::keypad::right_held() - bn::keypad::left_held();
        int step_y = bn::keypad::down_held() - bn::keypad::up_held();
        hero.set_position(bn::clamp(hero.x() + step_x, bn::fixed(-108), bn::fixed(108)),
                          bn::clamp(hero.y() + step_y, bn::fixed(-4), bn::fixed(60)));
        walk.update(hero, step_x, step_y);
        ++frames;

        for(int index = 0; index < coins.size(); ++index)
        {
            bn::fixed_point gap = coins[index].position() - hero.position();

            if(coins[index].visible() && bn::abs(gap.x()) < 10 && bn::abs(gap.y()) < 14)
            {
                coins[index].set_visible(false);
                coins_taken |= 1 << index;
            }
        }

        // In front of a board: its x, standing just below it.
        near_board = -1;

        for(int index = 0; index < slots_count; ++index)
        {
            if(bn::abs(hero.x() - boards[index].x()) < 16 && hero.y() < 0)
            {
                near_board = index;
            }
        }

        if(near_board >= 0)
        {
            if(bn::keypad::a_pressed())
            {
                write_slot(near_board, { 0, frames / 60, coins_taken, 0 });
                summaries[near_board] = describe(near_board);   // Read back from SRAM: what's really there.
            }
            else if(bn::keypad::b_pressed())
            {
                slot_data slot;

                if(read_slot(near_board, slot) == slot_state::used)
                {
                    // The hero stays at the board: loading at a save point puts you back there.
                    coins_taken = slot.coins;
                    frames = slot.seconds * 60;

                    for(int index = 0; index < coins.size(); ++index)
                    {
                        coins[index].set_visible(! (coins_taken & (1 << index)));
                    }
                }
            }
            else if(bn::keypad::select_pressed())
            {
                // Pull the plug halfway through a save: only the first half of the new bytes
                // reach SRAM, and the rest keeps the slot's old bytes.
                bn::array<int, 2> first_half = { slot_format, frames / 60 };
                bn::sram::write_offset(first_half, slot_offset(near_board));
                bn::core::reset();
            }
        }

        // The text changes only when what it says changes.
        bn::string<32> wanted = near_board >= 0 ? summaries[near_board] :
                bn::format<32>("Coins {}  Time {}s", bn::popcount(unsigned(coins_taken)), frames / 60);

        if(wanted != shown)
        {
            shown = wanted;
            top_text.clear();
            text_generator.generate(0, -72, shown, top_text);
            bottom_text.clear();
            text_generator.generate(0, 70, near_board >= 0 ? "A: save  B: load  SELECT: cut power" :
                                                             "Walk up to a board", bottom_text);
        }

        bn::core::update();
    }
}
