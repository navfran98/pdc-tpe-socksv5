#ifndef PROTOS2022A_GREETINGPARSER
#define PROTOS2022A_GREETINGPARSER

#include <stdint.h>
#include "buffer.h"

#define MARSHALL_SPACE 2

#define SUPPORTED_VERSION 0x05

// SOCKSv5 METHODS
#define NO_AUTHENTICATION_REQUIRED        0x00
#define USERNAME_PASSWORD_AUTH            0x02
#define NO_ACCEPTABLE_METHODS             0xFF



enum greeting_state {
    greeting_reading_version,
    greeting_reading_nmethods,
    greeting_reading_methods,
    greeting_done,          
    greeting_unsupported_version, 
    greeting_bad_syntax
};

struct greeting_parser {
    enum greeting_state state;
    uint8_t methods_remaining;
    uint8_t *methods;
    uint8_t methods_index;  
};



void
greeting_parser_init(struct greeting_parser *hp);


enum greeting_state
consume_greeting_buffer(buffer *b, struct greeting_parser *hp);



enum greeting_state
parse_single_greeting_character(uint8_t c, struct greeting_parser *hp);


void
greeting_marshall(buffer *b, uint8_t method);


#endif
