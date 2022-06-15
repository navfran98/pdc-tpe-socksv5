#ifndef PROTOS2022A_AUTHSTM
#define PROTOS2022A_AUTHSTM

#include "selector.h"
#include "auth_parser.h"

#define AUTH_SUCCESS 0
#define AUTH_FAIL 1



struct auth_stm {
    struct auth_parser auth_parser;

    buffer * rb; 
    buffer * wb;

    uint8_t reply; 
};


unsigned
auth_init(const unsigned state, struct selector_key *key);

unsigned
auth_read(struct selector_key *key);

unsigned
auth_write(struct selector_key *key);

#endif
