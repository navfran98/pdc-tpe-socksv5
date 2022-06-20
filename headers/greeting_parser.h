#ifndef PROTOS2022A_GREETINGPARSER
#define PROTOS2022A_GREETINGPARSER

#include <stdint.h>
#include "buffer.h"

#define MSG_SPACE 2
#define SOCKSV5_SUPPORTED_VERSION 0x05

static const uint8_t NO_AUTHENTICATION_REQUIRED = 0x00;
static const uint8_t USERNAME_PASSWORD_AUTH = 0x02;
static const uint8_t NO_ACCEPTABLE_METHODS = 0xFF;

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
    uint8_t * methods;
    uint8_t methods_index;  
};

void
greeting_parser_init(struct greeting_parser * pars);

enum greeting_state
consume_greeting_buffer(buffer * buff, struct greeting_parser * pars);

enum greeting_state
greeting_parser_feed(uint8_t c, struct greeting_parser * pars);

int
greeting_fill_msg(buffer * buff, uint8_t method);


#endif
