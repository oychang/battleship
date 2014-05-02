#ifndef GAME_H
#define GAME_H
//=============================================================================
// Dimensions of game board
#define COLUMNS         10
#define ROWS            10

// The number of cells in our ship
#define NUMBER_SHIPS    5
// Number of cells that a board saturated with ships will have
#define TOTAL_SHIP_AREA 17
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
// Returns 0 if board is not full, 1 if full
int board_full(board_t board);

// Counts the number of times ship cells occurs.
int count_ship_tiles(board_t board);

// Prints out the `enum cell` value of the board cells to stdout.
void print_board(board_t board);

// Returns the number of cells that ship `type` will occupy.
int get_ship_size(const enum cell type);

// Returns 0 if ship will not fit (i.e., it collides with a non-empty cell),
// 1 if it will. Assume all obstacles are ships.
int
valid_position(board_t board, const enum orientation dir,
    const int coords[2], const enum cell ship_type);

// Attempts to add a ship to the board board.
// It is assumed that coords are two (in bound) coordinates of board, of the
// top-left of the ship, assuming (0, 0) is at the top-left corner of board.
//
// Returns 0 if ship will not fit, 1 if will and added.
int
add_ship(board_t board, const enum orientation dir,
    const int coords[2], const enum cell ship_type);
//=============================================================================
#endif
