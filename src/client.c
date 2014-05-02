#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "protocol.h"
#include "game.h"

#define MAXDATASIZE 100 // max number of bytes we can get at once

int main(int argc, char *argv[]) {

    int sockfd;
    struct bs_req request;
    struct bs_resp response;
    char req_buf[MAXDATASIZE];
    char resp_buf[MAXDATASIZE];
    char player_name[MAX_USERNAME_CHARS];
    size_t req_len;
    int addr, resp_len, index;
    int ships_to_place = NUMBER_SHIPS;
    board_t client_board = {};
    board_t opp_board = {};
    int ship_placement, strike_target;
    int strike_indicator = -1;
    int fire_x, fire_y, invalid_location;

    struct addrinfo host_addr, *host_info, *option;

    // Get hostname from user; should be second argument in argv
    if (argc != 2) {
        fprintf(stderr, "usage: client hostname\n");
        exit(EXIT_FAILURE);
    }

    memset(&host_addr, 0, sizeof(struct addrinfo));
    host_addr.ai_family = AF_UNSPEC;
    host_addr.ai_socktype = SOCK_STREAM;
    if ((addr = getaddrinfo(argv[1], PORT, &host_addr, &host_info)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(addr));
        return EXIT_FAILURE;
    }

    for (option = host_info; option != NULL; option = option->ai_next) {
        if ((sockfd = socket(option->ai_family, option->ai_socktype,
                 option->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }
        if (connect(sockfd, option->ai_addr, option->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }
        break;
    }
    if (option == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return EXIT_FAILURE;
    }
    freeaddrinfo(host_info);

    // Try to connect by sending a connect request (use protocol.c)
    request.opcode = CONNECT;
    req_len = pack_request(req_buf, &request);
    send(sockfd, req_buf, req_len, 0);

    // Listen for a response
    if ((resp_len = recv(sockfd, resp_buf, MAXDATASIZE, 0)) == -1) {
        perror("recv");
        exit(EXIT_FAILURE);
    }

    switch (parse_response(resp_buf, &response)) {
    case OK:
        printf("Alert: Successfully connected to server!\n");
        break;
    case ERROR:
        printf("Error: %s\n", response.data.message);
        break;
    default:
        printf("What opcode is this?!?\n");
        break;
    }

    // Send request to establish player name (use protocol.c)
    printf("Please provide your name (max 7 chars): ");
    fgets(player_name, MAX_USERNAME_CHARS, stdin);
    for (index = 0; index < MAX_USERNAME_CHARS; index++) {
        if (player_name[index] == '\n') {
            player_name[index] = '\0'; // replace newline with null char
            break;
        }
    }
    request.opcode = NAME;
    strcpy(request.data.name, player_name);
    request.data.name[strlen(player_name)] = '\0';
    req_len = pack_request(req_buf, &request);
    send(sockfd, req_buf, req_len, 0);
    printf("Sending player naming request to server.\n");

    // Listen for a response
    if ((resp_len = recv(sockfd, resp_buf, MAXDATASIZE - 1, 0)) == -1) {
        perror("recv");
        exit(EXIT_FAILURE);
    }

    // Request information about the game from the server
    request.opcode = INFO;
    req_len = pack_request(req_buf, &request);
    send(sockfd, req_buf, req_len, 0);

    // Listen for a response
    if ((resp_len = recv(sockfd, resp_buf, MAXDATASIZE - 1, 0)) == -1) {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    resp_buf[resp_len] = '\0';

    if (parse_response(resp_buf, &response) == ABOUT) {
        printf("\nBATTLESHIP Game Information\n");
        printf("---------------------------\n");
/*        printf("Current game state: ");
        switch (response.data.session.stage) {
        case NOT_ENOUGH_PLAYERS:
            printf("Not enough players. Waiting for players to join...\n");
            break;
        case PLACING_SHIPS:
            printf("Both players are placing their ships.\n");
            break;
        case PLAYING:
            printf("Game is underway. Players are battling it out!\n");
            break;
        case DONE:
            printf("Game has concluded.");
            break;
        default:
            printf("What status is this?!?\n");
            break;
        }*/

        printf("Player 1: %s\n", response.data.session.names[0]);
        printf("Player 2: %s\n", response.data.session.names[1]);
    } else {
        printf("What opcode is this?!?\n");
    }

    printf("Beginning to poll server to see if ready!\n");
    // Poll server to see if the stage is set for game to begin
    request.opcode = READY;
    req_len = pack_request(req_buf, &request);

    send(sockfd, req_buf, req_len, 0);
    // Listen for a response; if WAIT, then go through loop
    if ((resp_len = recv(sockfd, resp_buf, MAXDATASIZE - 1, 0)) == -1) {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    resp_buf[resp_len] = '\0';
    // TODO: treat lack of response as TIME TO DIE
    while (parse_response(resp_buf, &response) == WAIT) {
        printf("Server is not yet ready; trying again in 5 seconds\n");
        sleep(5);
        send(sockfd, req_buf, req_len, 0);
        // Listen for a response; if WAIT, then go through loop again
        if ((resp_len = recv(sockfd, resp_buf, MAXDATASIZE - 1, 0)) == -1) {
            perror("recv");
            exit(EXIT_FAILURE);
        }
        resp_buf[resp_len] = '\0';
    }

    // If we got here, that means two clients have connected to server
    // The client is now able to place ships
    printf("ALERT: The time has come to place your ships, admiral!\n");
    print_board(client_board);
    request.opcode = PLACE;
    while (ships_to_place > 0) {
        switch (ships_to_place) {
        case 1:
        printf("Ship to place: DESTROYER   size = %d\n",
                   get_ship_size(DESTROYER));
            request.data.ship.type = DESTROYER;
            break;
        case 2:
        printf("Ship to place: SUBMARINE   size = %d\n",
               get_ship_size(SUBMARINE));
            request.data.ship.type = SUBMARINE;
            break;
        case 3:
            printf("Ship to place: CRUISER     size = %d\n",
                   get_ship_size(CRUISER));
            request.data.ship.type = CRUISER;
            break;
        case 4:
            printf("Ship to place: BATTLESHIP  size = %d\n",
                   get_ship_size(BATTLESHIP));
            request.data.ship.type = BATTLESHIP;
            break;
        case 5:
        printf("Ship to place: CARRIER     size = %d\n",
               get_ship_size(CARRIER));
            request.data.ship.type = CARRIER;
            break;
        }

        printf("Enter ship orientation (0 = horizontal, 1 = vertical) : ");
        scanf("%d" ,&ship_placement);
        while (! (ship_placement == 0 || ship_placement == 1)) {
            scanf("%d" ,&ship_placement);
            printf("Invalid input. Enter 0 for horizontal, 1 for vertical: ");
        }
        request.data.ship.orientation = ship_placement;

        printf("Enter x coordinate (0 through 9) to designate column  : ");
        scanf("%d", &ship_placement);
        while (ship_placement < 0 || ship_placement > 9) {
            printf("Invalid coordinate. Enter one integer between 0 and 9: ");
            scanf("%d", &ship_placement);
        }
        request.data.ship.coord[0] = ship_placement;

        printf("Enter the y coordinate (A through J) to designate row : ");
        scanf(" %c", (char*)&ship_placement);
        ship_placement = toupper(ship_placement);
        while (ship_placement < 'A' || ship_placement > 'J') {
            printf("Invalid coordinate. Enter a character between A and J : ");
            scanf(" %c", (char*)&ship_placement);
            ship_placement = toupper(ship_placement);
        }
        request.data.ship.coord[1] = ship_placement - 'A';

        req_len = pack_request(req_buf, &request);
        send(sockfd, req_buf, req_len, 0);
        if ((resp_len = recv(sockfd, resp_buf, MAXDATASIZE - 1, 0)) == -1) {
            perror("recv");
            exit(EXIT_FAILURE);
        }
        resp_buf[resp_len] = '\0';
        switch (parse_response(resp_buf, &response)) {
        case OK:
            printf("ALERT: Successfully placed the ship!\n");
            // printf("Coordinates: %d, %d\n", request.data.ship.coord[0], request.data.ship.coord[1]);
            add_ship(client_board, request.data.ship.orientation,
            request.data.ship.coord, request.data.ship.type);
            ships_to_place--;
            break;
        case NOK:
            printf("ALERT: Ship doesn't fit at given coordinates!\n");
            break;
        default:
            printf("What status is this?!?\n");
            break;
        }

        print_board(client_board);
    }

    printf("Well done, admiral! Your ships are in battle positions!\n");
    request.opcode = READY;
    req_len = pack_request(req_buf, &request);
    do {
        printf("Waiting for other player to place ships...\n");
        sleep(5);
        if (send(sockfd, req_buf, req_len, 0) == -1)
            perror("send");
        if (recv(sockfd, resp_buf, MAXDATASIZE, 0) == -1)
            perror("recv");

        parse_response(resp_buf, &response);
    } while (response.opcode != OK);

    // print the player names
    // assume nothing here breaks, heh
    printf("\nShips placed! The game is afoot.\n");
    request.opcode = ABOUT;
    req_len = pack_request(req_buf, &request);
    send(sockfd, req_buf, req_len, 0);
    recv(sockfd, resp_buf, MAXDATASIZE, 0);
    parse_response(resp_buf, &response);
    printf("Player 1: %s, Player 2: %s\n", response.data.session.names[0],
        response.data.session.names[1]);

    // Start firing
    // should be ready to accept a OK (good place), NOK (bad place),
    // wait (not turn), or fin (game done)
    request.opcode = READY;
    req_len = pack_request(req_buf, &request);
    send(sockfd, req_buf, req_len, 0);
    do {
        recv(sockfd, resp_buf, MAXDATASIZE, 0);
        switch (parse_response(resp_buf, &response)) {

        case NOK:
            printf("Hit nothing...\n");
            // todo: add hit marker to opponent board
            // xxx: note fall through, might not be what we want
            strike_indicator = 1; // 1 designates miss, by default 0 hit
        case OK:
	    // comparing with 0 and 1 allows compare with other int
            // if the first run through, no hit or miss result
            // todo: valid_position -> add to opponent board ->
            // custom draw function, perhaps with x's for hits, o's for misses
	    if (strike_indicator == 0) {
                opp_board[fire_x][fire_y] = 'x';
            } else if (strike_indicator == 1) {
                opp_board[fire_x][fire_y] = 'o';
            }
            strike_indicator = 0; // reset indicator for next run through

            // this is where we can do a fire
            // todo: probably want a while input loop like in placing ships
            request.opcode = FIRE;
            print_board(opp_board);
            printf("Ship's cannons primed for firing. Issue the command!\n");

            do {
	        invalid_location = 0;
                printf("Enter target x [column] coordinate (0 through 9)  : ");
                scanf("%d", &strike_target);
                while (strike_target < 0 || strike_target > 9) {
                    printf("Invalid coordinate. Enter an int (0 through 9): ");
                    scanf("%d", &strike_target);
                }
                fire_x = request.data.coord[0] = strike_target;

                printf("Enter target y [row] coordinate (A through J)     : ");
                scanf(" %c", (char*)&strike_target);
	        strike_target = toupper(strike_target);
                while (strike_target < 'A' || strike_target > 'J') {
                    printf("Invalid coordinate. Enter a char (A through J): ");
                    scanf(" %c", (char*)&strike_target);
                    strike_target = toupper(strike_target);
                }
                fire_y = request.data.coord[1] = strike_target - 'A';

                if (opp_board[fire_x][fire_y] != 0) {
                    printf("ALERT: Previously shot location; try again!\n");
                    invalid_location = 1;
                }
            } while (invalid_location == 1);
            req_len = pack_request(req_buf, &request);
            send(sockfd, req_buf, req_len, 0);
            break;
        case WAIT:
            printf("Waiting for other player to finish firing...\n");
            sleep(5);
            request.opcode = READY;
            req_len = pack_request(req_buf, &request);
            send(sockfd, req_buf, req_len, 0);
            break;
        case FIN:
            printf("Server is shutting down game...\n");
            break;
        default:
            printf("Unknown opcode %d\n", response.opcode);
            request.opcode = READY;
            req_len = pack_request(req_buf, &request);
            send(sockfd, req_buf, req_len, 0);
            break;
        }
    } while (response.opcode != FIN);

    if (count_ship_tiles(opp_board) == 0)
        printf("You Won!\n");
    else
        printf("You lost!\n");


    close(sockfd);
    return EXIT_SUCCESS;
}
