#include <stdlib.h>
#include <stdio.h>   // used by print_current_hello_parser()
#include "../headers/auth_parser.h"

void 
auth_parser_init(struct auth_parser * pars) {
    pars->state = auth_reading_version;
    pars->chars_remaining = 0;
    pars->user = NULL;
    pars->password = NULL;
    pars->index = 0; 
}

enum auth_state
consume_auth_buffer(buffer * buff, struct auth_parser * pars) {
    enum auth_state state = pars->state;

    while(buffer_can_read(buff)) {
        const uint8_t c = buffer_read(buff);
        state = auth_parser_feed(c, pars);
        if(state == auth_done || state == auth_unsupported_version || state == auth_bad_syntax || state == auth_bad_length) {
            break;
        }
    }

    return state;
}

enum auth_state
auth_parser_feed(const uint8_t c, struct auth_parser * pars) {
    switch(pars->state) {

        case auth_reading_version:
            if(c == SUPPORTED_VERSION){
                pars->state = auth_reading_user_len;
            }
            else
                pars->state = auth_unsupported_version;
            break;
        
        case auth_reading_user_len:
            if(c <= 0) {
                pars->state = auth_bad_length;
            } else {
                pars->chars_remaining = c; 
                pars->user = calloc(c + 1, sizeof(c));
                if(pars->user == NULL) {
                    pars->state = auth_bad_syntax;
                    return pars->state;
                }
                pars->state = auth_reading_user;
            }
            break;
        
        case auth_reading_user:
            pars->user[pars->index++] = c;
            if(pars->index == pars->chars_remaining) {
                pars->user[pars->index] = '\0';
                pars->state = auth_reading_pass_len;
            }
            break;

        case auth_reading_pass_len:
            if(c <= 0) {
                pars->state = auth_bad_length;
            } else {
                pars->chars_remaining = c;
                pars->index = 0;
                pars->password = calloc(c + 1, sizeof(c));
                if(pars->password == NULL) {
                    pars->state = auth_bad_syntax;
                    return pars->state;
                }
                pars->state = auth_reading_pass;
            }
            break;

        case auth_reading_pass:
            pars->password[pars->index++] = c;
            if(pars->index == pars->chars_remaining) {
                pars->password[pars->index] = '\0';
                pars->state = auth_done;
            }
            break;
        
        case auth_bad_length:
        case auth_done:
        case auth_unsupported_version:
        case auth_bad_syntax:
            break;
            
        default:
            exit(EXIT_FAILURE);
    }
    return pars->state;
}

void
auth_fill_msg(buffer * buff, const uint8_t code) {
    size_t space_left;
    uint8_t * where_to_write = buffer_write_ptr(buff, &space_left);

    where_to_write[0] = SUPPORTED_VERSION;
    where_to_write[1] = code;
    buffer_write_adv(buff, MSG_SPACE);
}
