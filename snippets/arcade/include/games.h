#ifndef GAMES_H
#define GAMES_H

#include "bn_sprite_text_generator.h"

// Every game has the same shape: it creates everything it shows, plays until it ends and returns a score.
// All it created is gone by the time it returns, so the next scene starts from an empty screen.
// Games never touch the save: the hub keeps it.
int coin_dash(bn::sprite_text_generator& text_generator);
int dodge(bn::sprite_text_generator& text_generator);

#endif
