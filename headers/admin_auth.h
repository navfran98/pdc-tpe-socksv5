#ifndef PROTOS2022A_ADMIN_AUTH
#define PROTOS2022A_ADMIN_AUTH

#include <unistd.h>

#include "buffer.h"
#include "selector.h"
#include "admin_auth_parser.h"


struct admin_auth_stm {
    buffer * rb;
    buffer * wb; 

    struct admin_auth_parser admin_auth_parser;
};

unsigned admin_auth_init(const unsigned state, struct selector_key *key);

unsigned admin_auth_read(struct selector_key * key);

unsigned admin_auth_write(struct selector_key * key);

#endif
