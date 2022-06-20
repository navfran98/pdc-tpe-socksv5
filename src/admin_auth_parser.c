#include <stdlib.h>
#include <stdio.h>   // used by print_current_hello_parser()
#include <stdbool.h>
#include <string.h>

#include "../headers/admin_auth_parser.h"
#include "../headers/buffer.h"

void 
admin_auth_parser_init(struct admin_auth_parser * pars) {
    pars->state = admin_auth_reading_user_len;
    pars->chars_remaining = 0;
    pars->user = NULL;
    pars->password = NULL;
    pars->index = 0; 
}

enum admin_auth_state
admin_consume_auth_buffer(buffer * buff, struct admin_auth_parser * pars) {
    enum admin_auth_state state = pars->state;

    while(buffer_can_read(buff)) {
        const uint8_t c = buffer_read(buff);
        state = admin_auth_parser_feed(c, pars);
        if(state == admin_auth_done || state == admin_auth_bad_syntax) {
            break;
        }
    }
    return state;
}

enum admin_auth_state
admin_auth_parser_feed(const uint8_t c, struct admin_auth_parser * pars) {
    switch(pars->state) {
        case admin_auth_reading_user_len:
            if(c <= 0) {
                pars->state = admin_auth_bad_syntax;
            } else {
                pars->chars_remaining = c; 
                pars->user = calloc(c + 1, sizeof(c));
                if(pars->user == NULL) {
                    pars->state = admin_auth_bad_syntax;
                    return pars->state;
                }
                pars->state = admin_auth_reading_user;
            }
            break;
        
        case admin_auth_reading_user:
            pars->user[pars->index++] = c;
            if(pars->index == pars->chars_remaining) {
                pars->user[pars->index] = '\0';
                pars->state = admin_auth_reading_pass_len;
            }
            break;

        case admin_auth_reading_pass_len:
            if(c <= 0) {
                pars->state = admin_auth_bad_syntax;
            } else {
                pars->chars_remaining = c;
                pars->index = 0;
                pars->password = calloc(c + 1, sizeof(c));
                if(pars->password == NULL) {
                    pars->state = admin_auth_bad_syntax;
                    return pars->state;
                }
                pars->state = admin_auth_reading_pass;
            }
            break;

        case admin_auth_reading_pass:
            pars->password[pars->index++] = c;
            if(pars->index == pars->chars_remaining) {
                pars->password[pars->index] = '\0';
                pars->state = admin_auth_done;
            }
            break;
        
        case admin_auth_done:
        case admin_auth_bad_syntax:
            break;
            
        default:
            exit(EXIT_FAILURE);
    }
    return pars->state;
}

int
admin_auth_fill_msg(buffer * buff, uint8_t ret) {
    // 0 -> "Succesfully connected"
    // 1 -> "Wrong credentials"
    // 2 -> "Bad syntax"
    size_t space_left;
    uint8_t * where_to_write = buffer_write_ptr(buff, &space_left);
    if(space_left < 1){
        return -1;
    }
    where_to_write[0] = ret;
    buffer_write_adv(buff, 1);
    return 0;
}
