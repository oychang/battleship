#include <stdio.h>
#include "game.h"
//=============================================================================
void
print_board(board_t board)
{
    int i, j;
    printf("\n");
    printf("  (X)  0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9\n");
    printf("(Y)    + + + + + + + + + + + + + + + + + + +\n\n");
    for (i = 0; i < ROWS; i++) {
        for (j = 0; j < COLUMNS; j++)
            if (j == 0) {
	        printf(" %c +   %d | ", 65 + i, board[i][j]);
	    } else if (j == COLUMNS - 1) {
                printf("%d ", board[i][j]);
	    } else {
	        printf("%d | ", board[i][j]);
	    }
        if (i == ROWS - 1) {
	    printf("\n");
	} else {
            printf("\n-- +  --- --- --- --- --- --- --- --- --- ---\n");
	}
    }
    printf("\n");
}
//=============================================================================
int
get_ship_size(const enum cell type)
{
    switch (type) {
    case DESTROYER: return DESTROYER_SIZE;
    case SUBMARINE: return SUBMARINE_SIZE;
    case CRUISER: return CRUISER_SIZE;
    case BATTLESHIP: return BATTLESHIP_SIZE;
    case CARRIER: return CARRIER_SIZE;
    default: return -1;
    }
}
//=============================================================================
int
valid_position(board_t board, const enum orientation dir,
    const unsigned char coords[2], const enum cell ship_type)
{
    const int size = get_ship_size(ship_type);
    if (size == -1)
        return 0;

    int x = coords[0];
    int y = coords[1];
    int i;
    for (i = 0; i < size; i++) {
        if (dir == HORIZONTAL) {
            if (board[x][y+i] != EMPTY)
                return 0;
        } else {
            if (board[x+i][y] != EMPTY)
                return 0;
        }
    }

    return 1;
}
//=============================================================================
int
add_ship(board_t board, const enum orientation dir,
    const unsigned char coords[2], const enum cell ship_type)
{
    if (!valid_position(board, dir, coords, ship_type))
        return 0;

    const int size = get_ship_size(ship_type);
    int x = coords[0];
    int y = coords[1];
    int i;
    for (i = 0; i < size; i++) {
        if (dir == HORIZONTAL)
            board[x][y+i] = ship_type;
        else
            board[x+i][y] = ship_type;
    }

    return 1;
}
//=============================================================================
