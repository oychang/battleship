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
/*void
handle_info(struct bs_resp * resp, struct bs_session * s)
{
    resp->opcode = ABOUT;

    resp->data.session.stage = s->stage;

    strncpy(resp->data.session.names[0], s->names[0], MAX_USERNAME_CHARS);
    strncpy(resp->data.session.names[1], s->names[1], MAX_USERNAME_CHARS);

    return;
}*/
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
/*void
handle_name(struct bs_resp * resp, struct bs_req * rq, struct bs_session * s)
{
    if (s->players == 2) {
        handle_error(resp, "Too many players");
        return;
    }

    resp->opcode = OK;

    return;
}*/
//=============================================================================
// TODO: implement
/*void
handle_place(struct bs_resp * resp, struct bs_req * rq, struct bs_session * s)
{
    return;
}*/
//=============================================================================
// TODO: implement
/*void
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
}*/
//=============================================================================
/*void
handle_connect(struct bs_resp * resp, struct bs_session * s)
{
    s->players++;
    resp->opcode = OK;
    return;
}*/
//=============================================================================
/* Wraps around the setup nastiness that select() requires.
 * Returns...
 * -1 if network error,
 * -2 if disconnect
 * sockfd if a good connection or dropped connection or too many
 */
int
select_wrapper(fd_set * master, int * nfds, int serverfd,
    struct bs_session * s, char * buf)
{
    // Copy list
    // TODO: check if this actually works
    fd_set readfds;
    FD_ZERO(&readfds);
    readfds = *master;
    if (select((*nfds)+1, &readfds, NULL, NULL, NULL) == -1) {
        perror("select");
        return -1; // network error
    }

    // Run through the sets from select() for data
    int fd;
    for (fd = 0; fd <= *nfds; fd++) {
        // if file descriptor has no data to read
        if (!FD_ISSET(fd, &readfds)) {
            continue;
        // if data to read & it's the server, i.e. new connection
        } else if (fd == serverfd) {
            if (s->stage != NOT_ENOUGH_PLAYERS)
                return fd; // attempt to join game in progress

            // Accept the connection
            struct sockaddr_storage client;
            socklen_t addrlen = sizeof(client);
            int clientfd = accept(serverfd, (struct sockaddr *)&client,
                &addrlen);
            if (clientfd == -1) {
                perror("accept");
                return -1;
            }

            // Add to our set of connections
            FD_SET(clientfd, master);
            if (clientfd > *nfds)
                *nfds = clientfd;

            printf("connect\n");

            return clientfd;
        // if data to read & not the server
        } else {
            int recvbytes = recv(fd, buf, sizeof(buf), 0);

            if (recvbytes <= 0) {
                printf("close\n");
                close(fd);
                FD_CLR(fd, master);
                return -2;
            }

            printf("data from previously connected\n");
            return fd;
        }
    }

    return -1;
}
//=============================================================================
int main(void)
{

    // Get a socket to listen on PORT
    const int serverfd = setup_server();
    if (serverfd == -1)
        return EXIT_FAILURE;

    // Setup file descriptor sets for use with select().
    // http://beej.us/guide/bgnet/output/html/multipage/advanced.html
    fd_set master;
    FD_ZERO(&master);
    // Add server to fd list
    FD_SET(serverfd, &master);
    int nfds = serverfd;

    // Setup initial game data
    struct bs_session session = {
        .stage = NOT_ENOUGH_PLAYERS,
        .names = {"Player 1", "Player 2"},
        .players = 0,

        .current_player = -1,
        .boards = {{}, {}},
    };
    int sockets[2] = {-1, -1};
    buffer request;
    buffer response;
    struct bs_req rq;
    struct bs_resp rp;
    size_t resp_len;

    while (session.stage != DONE) {
        int sock = select_wrapper(&master, &nfds, serverfd, &session, request);
        // 0 = first player, 1 = second player, -1 = neither
        int player = (sock == sockets[0] ? 0 : sock == sockets[1] ? 1 : -1);

        // if no useful data
        if (sock == -1) {
            fprintf(stderr, "select() network error\n");
            continue;
        // if disconnect
        } else if (sock == -2) {
            // TODO: prepare fins, send to all remaining socks
            printf("got client disconnect\n");
            session.stage = DONE;
            continue;
        // if too many connections
        } else if (!(sock == sockets[0] || sock == sockets[1])) {
            rp.opcode = ERROR;
            rp.data.message = "Too many players";
            resp_len = pack_response(response, &rp);
            send(sock, response, resp_len, 0);
            continue;
        // if good initial connection
        } else if (session.stage == NOT_ENOUGH_PLAYERS && player == -1) {
            sockets[session.players++] = sock;
            if (session.players == 2) {
                session.stage = PLACING_SHIPS;
                session.current_player = 0;
            }
            continue;
        } else if (player != -1) {
            // if first player has connected but not second
            // or if not this player's turn yet past NOT_ENOUGH_PLAYERS
            if ((session.stage == NOT_ENOUGH_PLAYERS)
                || (session.current_player != player)) {
                rp.opcode = WAIT;
                resp_len = pack_response(response, &rp);
                send(sock, response, resp_len, 0);
                continue;
            }
        }

        // congratulations on making it this far, request
        // this means that we're actually playing the game now
        // Get request
        switch (parse_request(request, &rq)) {
        // case CONNECT:
        //     handle_connect(&rp, &session);
        // case INFO:
        //     handle_info(&rp, &session);
        //     break;
        // case NAME:
        //     // TODO: add socket to session
        //     handle_name(&rp, &rq, &session);
        //     break;
        // case PLACE:
        //     handle_place(&rp, &rq, &session);
        //     break;
        // case FIRE:
        //     handle_fire(&rp, &rq, &session);
        //     break;
        default:
            handle_error(&rp, "Invalid Opcode");
            break;
        }

        /*buffer response;
        size_t len = pack_response(response, &rp);
        send(clientfd, response, len, 0);*/
    }

    close(serverfd);
    return EXIT_SUCCESS;
}
//=============================================================================
