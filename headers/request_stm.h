#ifndef PROTOS2022A_REQUESTSTM
#define PROTOS2022A_REQUESTSTM

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "selector.h"
#include "request_parser.h"

struct request_stm {
    struct request_parser request_parser; // tiene su estado 

    buffer * rb;
    buffer * wb;

    struct sockaddr_in  origin_addr_ipv4;
    struct sockaddr_in6 origin_addr_ipv6;
};

unsigned
request_read_init(const unsigned state, struct selector_key * key);

unsigned
request_read(struct selector_key * key);

unsigned
request_write(struct selector_key * key);

#endif
