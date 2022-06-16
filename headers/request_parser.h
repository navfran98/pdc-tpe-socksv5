#ifndef PROTOS2022A_REQUESTPARSER
#define PROTOS2022A_REQUESTPARSER

#include "buffer.h"
#include <stdint.h>

#define PROXY_SOCKS_REQUEST_SUPPORTED_VERSION 0x05
#define PROXY_SOCKS_REQUEST_RESERVED          0x00   // Fields marked RESERVED (RSV) must be set to X'00'.

#define CONNECT_COMMAND 0x01

#define IPv4_LENGTH 4
#define IPv6_LENGTH 16

// Socksv5 reply codes
#define SUCCEDED                          0x00
#define GENERAL_SOCKS_SERVER_FAILURE      0X01
#define CONNECTION_NOT_ALLOWED_BY_RULESET 0x02
#define NETWORK_UNREACHABLE               0x03
#define HOST_UNREACHABLE                  0x04
#define CONNECTION_REFUSED                0x05
#define TTL_EXPIRED                       0x06
#define COMMAND_NOT_SUPPORTED             0x07
#define ADDRESS_TYPE_NOT_SUPPORTED        0x08


// Address Types
#define REQUEST_THROUGH_IPV4 0x01
#define REQUEST_THROUGH_IPV6 0x04


// Los posibles estados en los que se puede encontrar el parser de request
enum request_state {
    request_reading_version,
    request_reading_command,
    request_reading_rsv,
    request_reading_atyp,
    request_reading_addr,
    request_reading_port,
    request_done,
    request_error
};


struct request_parser {
    enum request_state state;
    int atyp;  // need to store a "-1" to indicate error

    uint8_t *addr;
    int addr_len;  // need to store a "-1" to indicate error
    ssize_t port;   // must store negative value to represent "first time checking this value"
    uint8_t addr_index;  // para ir metiendo los bytes de a uno en 'destination_address'

    uint8_t reply;
};


void
request_parser_init(struct request_parser *rp);



enum request_state
consume_request_buffer(buffer *b, struct request_parser *rp);



enum request_state
request_parser_feed( uint8_t c, struct request_parser *rp);



void
request_marshall(buffer *b, struct request_parser *rp);



#endif
