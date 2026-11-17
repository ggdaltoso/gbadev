#ifndef MAZE_H
#define MAZE_H

// The maze, one character per 16x16 cell: '#' is a crate, '.' is floor. The edge is all crates.
// 15x10 cells is exactly one screen. snippets/tools/pathfinding_assets.py draws maze.bmp from this.
namespace maze
{
    constexpr int cell_size = 16;
    constexpr int columns = 15;
    constexpr int rows = 10;

    constexpr const char* cells[rows] = {
        "###############",
        "#.....#.......#",
        "#.###.#.#####.#",
        "#.#.....#...#.#",
        "#.#.###.#.#.#.#",
        "#...#...#.#...#",
        "###.#.###.###.#",
        "#...#.....#...#",
        "#.#####.#...#.#",
        "###############",
    };
}

#endif
