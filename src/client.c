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
    size_t req_len;
    int addr, resp_len;
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
            printf("Error: %s\n", &resp_buf[1]);
            break;
        default:
	    break;
    }

    // Request information about the game from the server
    request.opcode = INFO;
    req_len = pack_request(req_buf, &request);
    send(sockfd, req_buf, req_len, 0);

    close(sockfd);
    return EXIT_SUCCESS;
}
