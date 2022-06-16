#include <stdlib.h>
#include "../headers/greeting_parser.h"

void
greeting_parser_init(struct greeting_parser * p) {
    p->state = greeting_reading_version;
    p->methods_remaining = 0;
    p->methods_index = 0;
    p->methods = NULL;
}



enum greeting_state
consume_greeting_buffer(buffer *b, struct greeting_parser *p) {
    enum greeting_state state = p->state; 

    while(buffer_can_read(b)) {
        const uint8_t c = buffer_read(b);
        state = greeting_parser_feed(c, p);
        if(state == greeting_done || state == greeting_unsupported_version || state == greeting_bad_syntax) {
            break;   // stop reading
        }
    }

    return state;
}



enum greeting_state
greeting_parser_feed(const uint8_t c, struct greeting_parser *p) {
    switch(p->state) {

        case greeting_reading_version:
            if(c == SOCKSV5_SUPPORTED_VERSION)
                p->state = greeting_reading_nmethods;
            else
                p->state = greeting_unsupported_version;
            break;
        
        case greeting_reading_nmethods:
            if(c <= 0) {
                // zero methods were given
                p->state = greeting_bad_syntax;
            } else {
                p->methods_remaining = c; 
                p->methods = calloc(c, sizeof(c));
                if(p->methods == NULL) {
                    p->state = greeting_bad_syntax;
                    return p->state;
                }
                p->state = greeting_reading_methods;
            }
            break;
        
        case greeting_reading_methods:
            p->methods[p->methods_index++] = c;
            if(p->methods_index == p->methods_remaining) {
                p->state = greeting_done;
            }
            break;
        
        case greeting_done:
        case greeting_unsupported_version:
        case greeting_bad_syntax:
            // return these states now
            break;
            
        default:
            // Impossible state!
            exit(EXIT_FAILURE);
    }
    return p->state;
}

int
greeting_marshall(buffer *b, const uint8_t method) {
    size_t space_left_to_write;
    uint8_t *where_to_write_next = buffer_write_ptr(b, &space_left_to_write);
    if(space_left_to_write < 2){
        return -1;
    }
    where_to_write_next[0] = SOCKSV5_SUPPORTED_VERSION;
    where_to_write_next[1] = method;
    buffer_write_adv(b, MARSHALL_SPACE);
    return 2;
}
