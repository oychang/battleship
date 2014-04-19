#ifndef NETWORK_H
#define NETWORK_H

#include "game.h"
//=============================================================================
#define MAXSTRING 64
#define MAX_USERNAME_CHARS 8
typedef char string[MAXSTRING];
//=============================================================================
enum game_state {
    NOT_ENOUGH_PLAYERS = 0,
    PLACING_SHIPS = 1,
    PLAYING = 2,
    DONE = 3
};
//=============================================================================
struct bs_session {
    enum game_state stage;
    char names[2][MAX_USERNAME_CHARS];
};
//=============================================================================
/* BattleShip Request
 * Opcode:
 * 0 = INFO = request player names, game status
 * 1 = NAME = set name
 * 2 = PLACE = put ship at coord
 * 3 = FIRE = put a shot at the coord
 * note, there is no way to request the board for either player
 */
struct bs_req {
    enum {
        INFO = 0,
        NAME = 1,
        PLACE = 2,
        FIRE = 3
    } opcode;

    union {
        char          name[MAX_USERNAME_CHARS];
        unsigned char coord[2];
        struct {
            enum cell     type;
            unsigned char orientation;
            unsigned char coord[2];
        } ship;
    } data;
};
//=============================================================================
/* BattleShip Response
 *
 */
struct bs_resp {
    enum {
        OK = 0,
        ABOUT = 1,
        WAIT = 2,
        ERROR = 3
    } opcode;

    union {
        string message;
        struct bs_session session;
    } data;
};
//=============================================================================
size_t pack_request(char * buf, struct bs_req * request);
size_t pack_response(char * buf, struct bs_resp * response);
//=============================================================================
#endif
