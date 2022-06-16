#ifndef PROTOS2022A_AUTHPARSER
#define PROTOS2022A_AUTHPARSER

#include "buffer.h"

#define MARSHALL_SPACE 2

#define SUPPORTED_VERSION 0x01
//The VER field is set to X’05’ for this version of the protocol.
// PROTOCOL CODES
#define VALID_USER                        0x00
#define INVALID_USER                      0x01
#define VERSION_NOT_SUPPORTED             0x02
#define INTERNAL_ERROR                    0x03




enum auth_state {
    auth_reading_version,
    auth_reading_user_len,
    auth_reading_user,
    auth_reading_pass_len,
    auth_reading_pass,
    auth_bad_length,      
    auth_done,           
    auth_unsupported_version, 
    auth_bad_syntax         
};

struct auth_parser {
    enum auth_state state;
   
    uint8_t *user;
    uint8_t *password;

    uint8_t chars_remaining;
    uint8_t index; 
};


void
auth_parser_init(struct auth_parser *hp);



enum auth_state
consume_auth_buffer(buffer *b, struct auth_parser *hp);



enum auth_state
auth_parser_feed(uint8_t c, struct auth_parser *hp);


void
auth_marshall(buffer *b, uint8_t method);


#endif