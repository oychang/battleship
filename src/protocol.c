#include <string.h>
#include "protocol.h"

size_t
pack_request(char * buf, struct bs_req * request)
{
    size_t size = 0;

    buf[size++] = request->opcode;
    switch (request->opcode) {
    case NAME:
        // ensure null-terminated
        request->data.name[MAX_USERNAME_CHARS-1] = '\0';
        strncpy(&buf[size], request->data.name, MAX_USERNAME_CHARS);
        size += strlen(request->data.name) + 1;
        break;
    case PLACE:
        // assume all of these values get implicitly converted to numbers
        // that match up with the spec & will fit
        buf[size++] = request->data.ship.type;
        buf[size++] = request->data.ship.orientation;
        buf[size++] = request->data.ship.coord[0];
        buf[size++] = request->data.ship.coord[1];
        break;
    case FIRE:
        buf[size++] = request->data.coord[0];
        buf[size++] = request->data.coord[1];
        break;
    case CONNECT: case INFO: default:
        break;
    }

    return size;
}

size_t
pack_response(char * buf, struct bs_resp * response)
{
    size_t size = 0;

    buf[size++] = response->opcode;
    switch (response->opcode) {
    case ABOUT:
        buf[size++] = response->data.session.stage;

        response->data.session.names[0][MAX_USERNAME_CHARS-1] = '\0';
        strncpy(&buf[size], response->data.session.names[0],
            MAX_USERNAME_CHARS);
        size += strlen(response->data.session.names[0]) + 1;

        response->data.session.names[1][MAX_USERNAME_CHARS-1] = '\0';
        strncpy(&buf[size], response->data.session.names[1],
            MAX_USERNAME_CHARS);
        size += strlen(response->data.session.names[1]) + 1;
        break;
    case ERROR:
        // ensure null-terminated
        response->data.message[MAXSTRING-1] = '\0';
        strncpy(&buf[size], response->data.message, MAXSTRING);
        size += strlen(response->data.message) + 1;
        break;
    case FIN: case OK: case WAIT: default:
        break;
    }

    return size;
}

enum bs_req_opcode
parse_request(char * buf, struct bs_req * req)
{
    req->opcode = buf[0];
    switch (buf[0]) {
    case NAME:
        strncpy(req->data.name, &buf[1], MAX_USERNAME_CHARS);
        break;
    case PLACE:
        req->data.ship.type = buf[1];
        req->data.ship.orientation = buf[2];
        req->data.ship.coord[0] = buf[3];
        req->data.ship.coord[1] = buf[4];
        break;
    case FIRE:
        req->data.coord[0] = buf[1];
        req->data.coord[1] = buf[2];
        break;
    // noops -- these contain no additional data
    case CONNECT: case INFO: default:
        break;
    }

    return req->opcode;
}

enum bs_resp_opcode
parse_response(char * buf, struct bs_resp * resp)
{
    resp->opcode = buf[0];
    switch (buf[0]) {
    case ABOUT:
        strncpy(resp->data.session.names[0], &buf[1], MAX_USERNAME_CHARS);
        strncpy(resp->data.session.names[1],
            &buf[1 + strlen(resp->data.session.names[0]) + 1],
            MAX_USERNAME_CHARS);
        break;
    case ERROR:
        strncpy(resp->data.message, &buf[1], MAXSTRING);
        break;
    // noops -- these contain no additional data
    case FIN: case OK: case WAIT: default:
        break;
    }

    return resp->opcode;
}
