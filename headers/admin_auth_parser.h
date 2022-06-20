#ifndef PROTOS2022A_ADMIN_AUTH_PARSER
#define PROTOS2022A_ADMIN_AUTH_PARSER

#include "buffer.h"

#define AUTH_SUCCESS 0
#define AUTH_BAD_CREDENTIALS 1 
#define AUTH_BAD_SINTAX 2

enum admin_auth_state {
    admin_auth_reading_user_len,
    admin_auth_reading_user,
    admin_auth_reading_pass_len,
    admin_auth_reading_pass,
    admin_auth_done,           
    admin_auth_bad_syntax         
};

typedef struct admin_auth_parser {
    enum admin_auth_state state;
    int ret;
    uint8_t * user;
    uint8_t * password;
    uint8_t chars_remaining;
    uint8_t index; 
}admin_auth_parser;

void
admin_auth_parser_init(struct admin_auth_parser * pars);

enum admin_auth_state
admin_consume_auth_buffer(buffer * buff, struct admin_auth_parser * pars);

enum admin_auth_state
admin_auth_parser_feed(const uint8_t c, struct admin_auth_parser * pars);

int
admin_auth_fill_msg(buffer * buff, uint8_t ret);

#endif
