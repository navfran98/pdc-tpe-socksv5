#include <stdlib.h>
#include "../headers/greeting_parser.h"

void
greeting_parser_init(struct greeting_parser * pars) {
    pars->state = greeting_reading_version;
    pars->methods_remaining = 0;
    pars->methods_index = 0;
    pars->methods = NULL;
}

enum greeting_state
consume_greeting_buffer(buffer * buff, struct greeting_parser * pars) {
    enum greeting_state state = pars->state; 

    while(buffer_can_read(buff)) {
        const uint8_t c = buffer_read(buff);
        state = greeting_parser_feed(c, pars);
        if(state == greeting_done || state == greeting_unsupported_version || state == greeting_bad_syntax) {
            break;
        }
    }
    return state;
}

enum greeting_state
greeting_parser_feed(const uint8_t c, struct greeting_parser * pars) {
    switch(pars->state) {

        case greeting_reading_version:
            if(c == SOCKSV5_SUPPORTED_VERSION)
                pars->state = greeting_reading_nmethods;
            else
                pars->state = greeting_unsupported_version;
            break;
        
        case greeting_reading_nmethods:
            if(c <= 0) {
                pars->state = greeting_bad_syntax;
            } else {
                pars->methods_remaining = c; 
                pars->methods = calloc(c, sizeof(c));
                if(pars->methods == NULL) {
                    pars->state = greeting_bad_syntax;
                    return pars->state;
                }
                pars->state = greeting_reading_methods;
            }
            break;
        
        case greeting_reading_methods:
            pars->methods[pars->methods_index++] = c;
            if(pars->methods_index == pars->methods_remaining) {
                pars->state = greeting_done;
            }
            break;
        
        case greeting_done:
        case greeting_unsupported_version:
        case greeting_bad_syntax:
            break;
        default:
            exit(EXIT_FAILURE);
    }
    return pars->state;
}

int
greeting_marshall(buffer * buff, const uint8_t method) {
    size_t space_left;
    uint8_t * where_to_write = buffer_write_ptr(buff, &space_left);
    if(space_left < 2){
        return -1;
    }
    where_to_write[0] = SOCKSV5_SUPPORTED_VERSION;
    where_to_write[1] = method;
    buffer_write_adv(buff, MARSHALL_SPACE);
    return 2;
}
