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
handle_error(struct bs_resp * resp, string message)
{
    resp->opcode = ERROR;
    strncpy(resp->data.message, message, MAXSTRING);
    resp->data.message[MAXSTRING-1] = '\0';
    return;
}
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

            return clientfd;
        // if data to read & not the server
        } else {
            int recvbytes = recv(fd, buf, sizeof(buf), 0);

            if (recvbytes <= 0) {
                close(fd);
                FD_CLR(fd, master);
                return -2;
            }

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
        .names = {"Player1", "Player2"},
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
    enum cell cell_value;

    while (session.stage != DONE) {
        int sock = select_wrapper(&master, &nfds, serverfd, &session, request);
        // 0 = first player, 1 = second player, -1 = neither
        int player = (sock == sockets[0] ? 0 : sock == sockets[1] ? 1 : -1);
        printf("player = %d\n", player);

        // if no useful data
        if (sock == -1) {
            fprintf(stderr, "select() network error\n");
            continue;
        // if disconnect
        } else if (sock == -2) {
            // in essence, once anyone disconnects, the game is dead
            // TODO: prepare fins, send to all remaining socks
            printf("got client disconnect\n");
            session.players--;
            session.stage = DONE;
            continue;
        // if too many connections
        } else if (player == -1 && session.players >= 2) {
            printf("too many\n");
            rp.opcode = ERROR;
            strncpy(rp.data.message, "Too many players", MAXSTRING);
            resp_len = pack_response(response, &rp);
            send(sock, response, resp_len, 0);
            continue;
        // if good initial connection
        } else if (player == -1 && session.stage == NOT_ENOUGH_PLAYERS) {
            sockets[session.players++] = sock;
            if (session.players == 2) {
                session.stage = PLACING_SHIPS;
                session.current_player = 0;
                // TODO: continue in this case?
            }
            select_wrapper(&master, &nfds, serverfd, &session, request);
        } else if (player != -1) {
            // if first player has connected but not second
            // or if not this player's turn yet past NOT_ENOUGH_PLAYERS
            if ((session.stage == NOT_ENOUGH_PLAYERS && sockets[0] == -1) ||
                (session.stage != NOT_ENOUGH_PLAYERS && session.current_player != player)) {
                printf("wait\n");
                rp.opcode = WAIT;
                resp_len = pack_response(response, &rp);
                send(sock, response, resp_len, 0);
                continue;
            }
        }

        // Congratulations on making it this far, request!
        // We are now guaranteed that this player is allowed to connect,
        // and if we're far enough in the game, that it is this player's turn.
        switch (parse_request(request, &rq)) {
        case CONNECT:
            rp.opcode = OK;
            break;
        case INFO:
            rp.opcode = ABOUT;
            rp.data.session.stage = session.stage;
            rp.data.session.players = session.players;
            strncpy(rp.data.session.names[0], session.names[0], MAX_USERNAME_CHARS);
            rp.data.session.names[0][MAX_USERNAME_CHARS-1] = '\0';
            strncpy(rp.data.session.names[1], session.names[1], MAX_USERNAME_CHARS);
            rp.data.session.names[1][MAX_USERNAME_CHARS-1] = '\0';
            break;
        case NAME:
            rp.opcode = OK;
            strncpy(session.names[player], rq.data.name, MAX_USERNAME_CHARS);
            session.names[player][MAX_USERNAME_CHARS-1] = '\0';
            break;
        case PLACE:
            // handle_place(&rp, &rq, &session);
            break;
        case FIRE:
            cell_value = session.boards[player][rq.data.coord[0]][rq.data.coord[1]];
            // if firing on something with no ship
            if (cell_value == EMPTY || cell_value == MISS || cell_value == HIT)
                rp.opcode = NOK;
            else {
                session.boards[player][rq.data.coord[0]][rq.data.coord[1]] = HIT;
                rp.opcode = OK;
            }
            break;
        default:
            handle_error(&rp, "Invalid Opcode");
            break;
        }

        resp_len = pack_response(response, &rp);
        send(sock, response, resp_len, 0);
    }

    close(serverfd);
    return EXIT_SUCCESS;
}
//=============================================================================
