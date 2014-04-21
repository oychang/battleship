#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "server.h"
//=============================================================================
/* Get a valid socket for PORT & listen.
 * Returns -1 if failed to get a socket, otherwise a valid
 * bound & listening socket file descriptor.
 *
 * References:
 * http://www.beej.us/guide/bgnet/output/html/multipage/getaddrinfoman.html
 * http://www.beej.us/guide/bgnet/output/html/multipage/acceptman.html
 */
int
setup_server()
{
    struct addrinfo server;
    struct addrinfo * p = &server;

    struct addrinfo hints, *results;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_STREAM;

    int ai_error = getaddrinfo(NULL, PORT, &hints, &results);
    if (ai_error != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ai_error));
        return -1;
    }

    // getaddrinfo() returns a linked list of results
    int sockfd;
    static const int yes = 1;
    for (p = results; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            continue;
        } else if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
                              &yes, sizeof(int)) == -1) {
            perror("setsockopt");
        } else if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            continue;
        } else
            break;
    }

    freeaddrinfo(results);

    if (p == NULL) {
        fprintf(stderr, "failed to bind socket\n");
        return -1;
    } else if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        return -1;
    }

    return sockfd;
}
//=============================================================================
void
handle_info(struct bs_resp * resp, struct bs_session * s)
{
    resp->opcode = ABOUT;

    resp->data.session.stage = s->stage;

    strncpy(resp->data.session.names[0], s->names[0], MAX_USERNAME_CHARS);
    strncpy(resp->data.session.names[1], s->names[1], MAX_USERNAME_CHARS);

    return;
}
//=============================================================================
void
handle_error(struct bs_resp * resp, string message)
{
    resp->opcode = ERROR;

    message[MAXSTRING-1] = '\0';
    strncpy(resp->data.message, message, MAXSTRING);
    return;
}
//=============================================================================
// TODO: implement
void
handle_name(struct bs_resp * resp, struct bs_req * rq, struct bs_session * s)
{
    if (s->players == 2) {
        handle_error(resp, "Too many players");
        return;
    }

    resp->opcode = OK;

    return;
}
//=============================================================================
// TODO: implement
void
handle_place(struct bs_resp * resp, struct bs_req * rq, struct bs_session * s)
{
    return;
}
//=============================================================================
// TODO: implement
void
handle_fire(struct bs_resp * resp, struct bs_req * rq, struct bs_session * s)
{
    // Get player, then work with opponent's board
    // Check if already taken
    // Check if a hit
    // It's a miss

    // Handle current player
    // Still current player if already taken
    // Multiple shots if a hit

    return;
}
//=============================================================================
void
handle_connect(struct bs_resp * resp, struct bs_session * s)
{
    s->players++;
    resp->opcode = OK;
    return;
}

//=============================================================================
int main(void)
{

    // Get a socket to listen on PORT
    const int serverfd = setup_server();
    if (serverfd == -1)
        return EXIT_FAILURE;

    // Setup initial game state
    struct bs_session session = {
        .stage = NOT_ENOUGH_PLAYERS,
        .names = {"Player 1", "Player 2"},
        .players = 0,

        .current_player = -1,
        .boards = {{}, {}},
    };

    while (session.stage != DONE) {
        int clientfd;
        struct sockaddr_storage client;
        socklen_t addrlen = sizeof(struct sockaddr_storage);
        if ((clientfd = accept(serverfd, (struct sockaddr *)&client,
                &addrlen)) == -1) {
            perror("accept");
            return EXIT_FAILURE;
        }

        // Get request
        buffer request;
        struct bs_req rq;
        if ((recv(clientfd, request, MAXBUF, 0)) == -1) {
            perror("recv");
            return EXIT_FAILURE;
        }

        struct bs_resp rp;
        switch (parse_request(request, &rq)) {
        case CONNECT:
            handle_connect(&rp, &session);
        case INFO:
            handle_info(&rp, &session);
            break;
        case NAME:
            // TODO: add socket to session
            handle_name(&rp, &rq, &session);
            break;
        case PLACE:
            handle_place(&rp, &rq, &session);
            break;
        case FIRE:
            handle_fire(&rp, &rq, &session);
            break;
        default:
            handle_error(&rp, "Invalid Opcode");
            break;
        }

        // Send back to originating client
        /*buffer response;
        size_t len = pack_response(response, &rp);
        send(clientfd, response, len, 0);*/
        close(clientfd);
    }

    close(serverfd);
    return EXIT_SUCCESS;
}
//=============================================================================
