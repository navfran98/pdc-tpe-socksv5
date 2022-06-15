#include <stdlib.h>
#include <stdio.h>   // used by print_current_hello_parser()
#include "../headers/auth_parser.h"

void 
auth_parser_init(struct auth_parser *p) {
    p->state = auth_reading_version;
    p->chars_remaining = 0;
    p->user = NULL;
    p->password = NULL;
    p->index = 0; 
}

enum auth_state
consume_auth_buffer(buffer *b, struct auth_parser *p) {
    enum auth_state state = p->state;  // Le damos un valor por si no se entra en el while

    while(buffer_can_read(b)) {
        const uint8_t c = buffer_read(b);
        state = auth_parser_feed(c, p);
        if(state == auth_done || state == auth_unsupported_version || state == auth_bad_syntax || state == auth_bad_length) {
            break;   // stop reading
        }
    }

    return state;
}

enum auth_state
auth_parser_feed(const uint8_t c, struct auth_parser *p) {
    switch(p->state) {

        case auth_reading_version:
            if(c == SUPPORTED_VERSION){
                p->state = auth_reading_user_len;
            }
            else
                p->state = auth_unsupported_version;
            break;
        
        case auth_reading_user_len:
            if(c <= 0) {
                p->state = auth_bad_length;
            } else {
                p->chars_remaining = c; 
                p->user = calloc(c + 1, sizeof(c));
                if(p->user == NULL) {
                    p->state = auth_bad_syntax;
                    return p->state;
                }
                p->state = auth_reading_user;
            }
            break;
        
        case auth_reading_user:
            p->user[p->index++] = c;
            if(p->index == p->chars_remaining) {
                p->user[p->index] = '\0';
                p->state = auth_reading_pass_len;
            }
            break;

        case auth_reading_pass_len:
            if(c <= 0) {
                // password length is 0
                p->state = auth_bad_length;
            } else {
                p->chars_remaining = c;
                p->index = 0;
                p->password = calloc(c + 1, sizeof(c));
                if(p->password == NULL) {
                    p->state = auth_bad_syntax;
                    return p->state;
                }
                p->state = auth_reading_pass;
            }
            break;

        case auth_reading_pass:
            p->password[p->index++] = c;
            if(p->index == p->chars_remaining) {
                p->password[p->index] = '\0';
                p->state = auth_done;
            }
            break;
        
        case auth_bad_length:
        case auth_done:
        case auth_unsupported_version:
        case auth_bad_syntax:
            // return these states now
            break;
            
        default:
            // Impossible state!
            exit(EXIT_FAILURE);
    }
    return p->state;
}


void
auth_marshall(buffer *b, const uint8_t code) {
    size_t space_left_to_write;
    uint8_t *where_to_write_next = buffer_write_ptr(b, &space_left_to_write);

    where_to_write_next[0] = SUPPORTED_VERSION;
    where_to_write_next[1] = code;
    buffer_write_adv(b, MARSHALL_SPACE);
}
