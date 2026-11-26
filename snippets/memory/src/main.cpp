#include "bn_algorithm.h"
#include "bn_array.h"
#include "bn_bg_palettes.h"
#include "bn_color.h"
#include "bn_core.h"
#include "bn_format.h"
#include "bn_keypad.h"
#include "bn_memory.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_string.h"
#include "bn_timer.h"
#include "bn_timers.h"
#include "bn_vector.h"

#include "font.h"

constexpr int count = 2048;   // 8 KB of ints in each array.

constexpr bn::array<int, count> make_numbers()
{
    bn::array<int, count> numbers = {};

    for(int index = 0; index < count; ++index)
    {
        numbers[index] = index;
    }

    return numbers;
}

// The same numbers in the three places they can live.
constexpr bn::array<int, count> rom_numbers = make_numbers();          // constexpr: stays in ROM.
bn::array<int, count> iwram_numbers = make_numbers();                  // A global: copied to IWRAM at boot.
BN_DATA_EWRAM bn::array<int, count> ewram_numbers = make_numbers();    // A global moved to EWRAM.

// The same loop twice: once in ROM, where all code goes by default, and once copied to IWRAM.
[[gnu::always_inline]] inline int sum(const bn::array<int, count>& numbers)
{
    int result = 0;

    for(int number : numbers)
    {
        result += number;
    }

    return result;
}

// noipa stops the compiler from working the sums out ahead of time.
[[gnu::noipa]] int sum_with_rom_code(const bn::array<int, count>& numbers)
{
    return sum(numbers);
}

BN_CODE_IWRAM [[gnu::noipa]] int sum_with_iwram_code(const bn::array<int, count>& numbers)
{
    return sum(numbers);
}

// CPU cycles per number read, or -1 if the sum came out wrong.
int cycles_per_number(int (*sum_function)(const bn::array<int, count>&), const bn::array<int, count>& numbers)
{
    bn::timer timer;
    int result = sum_function(numbers);
    int cycles = timer.elapsed_ticks() * bn::timers::cpu_clocks_per_tick();
    return result == count * (count - 1) / 2 ? cycles / count : -1;
}

// Each call takes a 256-byte buffer from the stack, which lives at the top of IWRAM and grows down.
[[gnu::noipa]] int stack_at_depth(int depth)
{
    volatile char buffer[256];
    buffer[0] = 0;
    return depth == 0 ? bn::memory::used_stack_iwram() : stack_at_depth(depth - 1) + buffer[0];
}

int main()
{
    bn::core::init();
    bn::bg_palettes::set_transparent_color(bn::color(4, 6, 10));

    bn::sprite_text_generator text_generator(hud::font);
    bn::vector<bn::sprite_ptr, 64> text;
    text_generator.generate(-112, -72, bn::format<40>("IWRAM globals: {} of 32768", bn::memory::used_static_iwram()), text);
    text_generator.generate(-112, -60, bn::format<40>("EWRAM globals: {} of 262144", bn::memory::used_static_ewram()), text);
    text_generator.generate(-112, -48, bn::format<40>("ROM: {}", bn::memory::used_rom()), text);
    text_generator.generate(-112, -28, "Cycles to read a number", text);
    text_generator.generate(-112, -16, "data in:", text);
    const bn::array<int, count>* data[] = { &iwram_numbers, &ewram_numbers, &rom_numbers };
    constexpr bn::string_view data_names[] = { "IWRAM", "EWRAM", "ROM" };

    for(int column = 0; column < 3; ++column)
    {
        text_generator.generate(-8 + column * 46, -16, data_names[column], text);
        text_generator.generate(-8 + column * 46, -4, bn::to_string<8>(cycles_per_number(sum_with_rom_code, *data[column])), text);
        text_generator.generate(-8 + column * 46, 8, bn::to_string<8>(cycles_per_number(sum_with_iwram_code, *data[column])), text);
    }

    text_generator.generate(-112, -4, "code in ROM", text);
    text_generator.generate(-112, 8, "code in IWRAM", text);
    text_generator.generate(-112, 64, "Up and Down: depth", text);
    int fixed_text = text.size();
    int depth = 0;

    while(true)
    {
        int old_depth = depth;
        depth += bn::keypad::up_pressed() - bn::keypad::down_pressed();
        depth = bn::clamp(depth, 0, 32);   // Much deeper and the stack runs into the globals (see Pitfalls).

        if(depth != old_depth || text.size() == fixed_text)
        {
            text.shrink(fixed_text);
            int stack = stack_at_depth(depth);
            int free = 32768 - bn::memory::used_static_iwram() - stack;
            text_generator.generate(-112, 28, bn::format<40>("Stack at depth {}: {} bytes", depth, stack), text);
            text_generator.generate(-112, 40, bn::format<40>("IWRAM left between them: {}", free), text);
        }

        bn::core::update();
    }
}
