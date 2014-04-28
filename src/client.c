#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
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
    int addr, resp_len;
    int ships_to_place = NUMBER_SHIPS;
    board_t client_board = {};
    char ship_placement;
    
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

    for(option = host_info; option != NULL; option = option->ai_next) {
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
    printf("Sending connect request to server.\n");

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
    if (strlen(player_name) < MAX_USERNAME_CHARS - 1) {
        player_name[strlen(player_name) - 1] = '\0';
    }
    printf("Provided name was: %s\n", player_name);
    request.opcode = NAME;
    strcpy(request.data.name, player_name);
    printf("Packaged name was: %s\n", request.data.name);
    req_len = pack_request(req_buf, &request);
    printf("Pacakage length is: %zd\n", req_len);
    send(sockfd, req_buf, req_len, 0);
    printf("Sending player naming request to server.\n");

    // Listen for a response
    if ((resp_len = recv(sockfd, resp_buf, MAXDATASIZE - 1, 0)) == -1) {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    printf("name responese opcode (0 = okay) = %d\n", resp_buf[0]);

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
    printf("Received response length: %d\n", resp_len);

    if (parse_response(resp_buf, &response) == ABOUT) {
        printf("\nBATTLESHIP Game Information\n");
        printf("---------------------------\n");
        printf("Current game state: ");
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
        }
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
                break;
            case 2:
	        printf("Ship to place: SUBMARINE   size = %d\n", 
	               get_ship_size(SUBMARINE));
                break;
            case 3:
                printf("Ship to place: CRUISER     size = %d\n",
                       get_ship_size(CRUISER));
	        break;
            case 4:
                printf("Ship to place: BATTLESHIP  size = %d\n",
                       get_ship_size(BATTLESHIP));
                break;
            case 5:
	        printf("Ship to place: CARRIER     size = %d\n",
	               get_ship_size(CARRIER));
	        break;
        }
        printf("Enter ship orientation (0 for horizontal, 1 for vertical): ");
        ship_placement = fgetc(stdin);
        fflush(stdin);
        printf("Entered ship orientation: %c\n", ship_placement);
        printf("Enter the x coordinate: ");
        ship_placement = fgetc(stdin);
        fflush(stdin);
        printf("Entered x coordinate: %c\n", ship_placement);
        printf("Enter the y coordinate: ");
        ship_placement = fgetc(stdin);
        fflush(stdin);
        printf("Entered y coordinate: %c\n", ship_placement);
        ships_to_place--;
        // need to set up the request structure for each ship being placed
        req_len = pack_request(req_buf, &request);    
    }

    close(sockfd);
    return EXIT_SUCCESS;
}
