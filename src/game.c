#include <stdio.h>
#include "game.h"
//=============================================================================
void
print_board(board_t board)
{
    int i, j;
    for (i = 0; i < ROWS; i++) {
        for (j = 0; j < COLUMNS; j++)
            printf("%d ", board[i][j]);
        printf("\n");
    }
}
//=============================================================================
