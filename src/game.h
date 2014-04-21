#ifndef GAME_H
#define GAME_H
//=============================================================================
// Dimensions of game board
#define COLUMNS         10
#define ROWS            10

#define NUMBER_SHIPS    5
// Number of cells each ship occupies
#define DESTROYER_SIZE  2
#define SUBMARINE_SIZE  3
#define CRUISER_SIZE    3
#define BATTLESHIP_SIZE 4
#define CARRIER_SIZE    5
//=============================================================================
enum cell {
    // Ship-agnostic pieces
    EMPTY = 0, MISS = 1, HIT = 2,
    // Ship pieces
    DESTROYER = 3, SUBMARINE = 4,
    CRUISER = 5, BATTLESHIP = 6,
    CARRIER = 7
};
enum orientation {HORIZONTAL = 0, VERTICAL = 1};
//=============================================================================
typedef enum cell board_t[COLUMNS][ROWS];
//=============================================================================
void print_board(board_t board);
//=============================================================================
#endif
