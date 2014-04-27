#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "game.h" // board_t, enum orientation
//=============================================================================
// The pseudo well-known port that server listens for new connections on,
// client sends to.
#define PORT "5000"
//=============================================================================
// Maximum length of a user-input name
#define MAX_USERNAME_CHARS 8
// string is used for error messages
#define MAXSTRING 64
typedef char string[MAXSTRING];
//=============================================================================
enum game_state {
    // Initial state...game has not yet begun
    NOT_ENOUGH_PLAYERS = 0,
    // At least one player is in the process of placing ships
    PLACING_SHIPS = 1,
    // Game in progress
    PLAYING = 2,
    // Game done (probably redundant)
    DONE = 3
};
//=============================================================================
struct bs_session {
    // Public fields
    enum game_state stage;
    char names[2][MAX_USERNAME_CHARS];
    // Presumably either 0, 1, 2 if max players are 2
    int players;

    // Private fields
    // Current player, either 0 (player 1) or 1 (player 2)
    int current_player;
    // Internal representation of each board
    board_t boards[2];
};
//=============================================================================
// Assume bytes, same endian encoding, left-to-right packing.
// Suppose TCP, client<->server<->client model.
/**************************** BattleShip Request *****************************/
// Whenever a client sends a message to the server, it will send a buffer
// with one of the following opcodes:
// 0 = connect
//     Request is an attempt at an initial connection for client.
//     No data required.
// 1 = info
//     Client wants to know some information about the game session.
//     No data required.
// 2 = name
//     Client wants to change its name to a string.
//     char name[] required.
// 3 = place
//     Client wants to place a ship in a certain part of the board.
//     This may only happen during the PLACING_SHIPS game state.
//     Require the type of ship, orientation of ship, and top-left coord.
// 4 = fire
//     Client wants to fire a shot at a coord.
//     May only happen during PLAYING game state.
//     unsigned char[] required.
// 5 = ready?
//     Client wants to know if it can do it's next move.
//     Happens as the result of any request.
//     Tells client to poll in a certain time in the future.
//     No data required.
enum bs_req_opcode {
    CONNECT = 0, INFO = 1,
    NAME = 2, PLACE = 3,
    FIRE = 4, READY = 5
};
struct bs_req {
    enum bs_req_opcode opcode;
    union {
        char          name[MAX_USERNAME_CHARS];
        unsigned char coord[2];
        struct {
            enum cell        type;
            enum orientation orientation;
            unsigned char    coord[2];
        } ship;
    } data;
};
//=============================================================================
/**************************** BattleShip Response ****************************/
// As a response to a client's request, a server will send a buffer with one
// of the following opcodes:
// 0 = ok
//     Acknowledgment of receiving a request, successfully executed.
//     This is the response to a CONNECT, and well-formed NAME.
//     No other data is returned.
// 1 = about
//     Response contains information about the game session.
//     This is the response to a INFO request.
//     We return a subset of the fields in a `bs_session`
// 2 = wait
//     We are waiting for more clients to finish their business.
//     In the game, we operate lock-step with each player going about their
//     business in sequence.
//     This is the response to any request.
// 3 = hit
//     Player successfully scored a hit on the desired cell.
//     This is the response to a FIRE.
//     No other data is returned.
// 4 = miss
//     Player missed a hit on the desired cell.
//     This is the response to a FIRE.
//     No other data is returned.
// 5 = fin
//     The game is over.
//     This is the response to any request.
//     No other data is returned.
// 6 = error
//     The request could be not completed for a certain reason.
//     This is the response to any request.
//     A char[] string is requrned.
enum bs_resp_opcode {
    OK = 0, ABOUT = 1, WAIT = 2,
    HIT = 3, MISS = 4,
    FIN = 5, ERROR = 6
};
struct bs_resp {
    enum bs_resp_opcode opcode;
    union {
        string message;
        struct bs_session session;
    } data;
};
//=============================================================================
// These are utility functions that pack a buffer in TFTP-style
// with the value of a bs_req/bs_resp consistently.
// These functions are used within server & client.

// Returns the number of chars (bytes) in the buf
size_t pack_request(char * buf, struct bs_req * request);
size_t pack_response(char * buf, struct bs_resp * response);

enum bs_req_opcode parse_request(char * buf, struct bs_req * request);
enum bs_resp_opcode parse_response(char * buf, struct bs_resp * response);
//=============================================================================
#endif
