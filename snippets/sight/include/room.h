#ifndef ROOM_H
#define ROOM_H

// The room, one character per 16x16 cell: '#' is a crate, '.' is floor.
// 15x10 cells is exactly one screen. snippets/tools/collision_assets.py draws room.bmp from this.
namespace room
{
    constexpr int cell_size = 16;
    constexpr int columns = 15;
    constexpr int rows = 10;

    constexpr const char* cells[rows] = {
        "###############",
        "#.............#",
        "#..#.....#....#",
        "#..#......#...#",
        "#.............#",
        "#.............#",
        "#....##.......#",
        "#..........#..#",
        "#.............#",
        "###############",
    };
}

#endif
