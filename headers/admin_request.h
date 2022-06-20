#ifndef PROTOS2022A_ADMIN_REQUEST
#define PROTOS2022A_ADMIN_REQUEST

#include <unistd.h>

#include "buffer.h"
#include "selector.h"
#include "admin_request_parser.h"

struct admin_request_stm {
    struct req_parser admin_request_parser;

    buffer * rb; 
    buffer * wb; 
};

unsigned admin_request_init(const unsigned state, struct selector_key * key);

unsigned admin_request_read(struct selector_key * key);

unsigned admin_request_write(struct selector_key * key);

#endif
