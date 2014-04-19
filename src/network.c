#include <string.h>
#include "network.h"

size_t
pack_request(char * buf, struct bs_req * request)
{
    size_t size = 0;

    buf[size++] = request->opcode;
    switch (request->opcode) {
    case INFO:
        break;
    case NAME:
        // ensure null-terminated
        request->data.name[MAX_USERNAME_CHARS-1] = '\0';
        strncpy(&buf[size], request->data.name, MAX_USERNAME_CHARS);
        size += strlen(request->data.name);
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
    default:
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
        break;
    case ERROR:
        // ensure null-terminated
        response->data.message[MAXSTRING-1] = '\0';
        strncpy(&buf[size], response->data.message, MAXSTRING);
        size += strlen(response->data.message);
        break;
    default:
        break;
    }

    return size;
}

void
parse_request(char * buf, struct bs_req * request)
{

}

void
parse_response(char * buf, struct bs_resp * response)
{

}
