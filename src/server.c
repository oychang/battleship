#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "server.h"
//=============================================================================
/* Get a valid socket to listen use with PORT.
 * Returns -1 if failed to get a socket, otherwise a valid
 * bound socket file descriptor.
 *
 * References:
 * http://www.beej.us/guide/bgnet/output/html/multipage/getaddrinfoman.html
 * http://www.beej.us/guide/bgnet/output/html/multipage/acceptman.html
 */
int
getaddrinfo_wrapper(struct addrinfo *p)
{
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
    for (p = results; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            continue;
        } else if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            continue;
        } else
            break;
    }
    freeaddrinfo(results);

    if (p == NULL)
        return -1;

    static const int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        perror("setsockopt");
    return sockfd;
}
//=============================================================================
int main(void)
{
    struct addrinfo server;
    int serverfd = getaddrinfo_wrapper(&server);
    if (serverfd == -1)
        return EXIT_FAILURE;
    if (listen(serverfd, BACKLOG) == -1) {
        perror("listen");
        return EXIT_FAILURE;
    }

    while (true) {
        // Accept connection to server
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
        parse_request(request, &rq);

        // TODO: Decide how to respond


        // Send back to originating client
        buffer response;
        struct bs_resp rp;
        size_t len = pack_response(response, &rp);
        send(clientfd, response, len, 0);
        close(clientfd);
    }

    close(serverfd);
    return EXIT_SUCCESS;
}
//=============================================================================
