#include <stdlib.h>
#include <stdio.h>   // used by print_current_request_parser()
#include "../headers/request_parser.h"

void
request_parser_init(struct request_parser *rp) {
    rp->state = request_reading_version;
    rp->addr_index = 0;
    rp->atyp = -1;
    rp->port = -1;  
    rp->addr_len = -1;
    rp->addr = NULL;
}

enum request_state
consume_request_buffer(buffer *b, struct request_parser *rp) {
    enum request_state state = rp->state; 

    while(buffer_can_read(b)) {
        const uint8_t c = buffer_read(b);
        state = request_parser_feed(c, rp);
        if(state == request_done) {
            // ejecutamos una vez más para generar el request_reply
            request_parser_feed(c, rp);
            break;
        } else if(state == request_error) {
            break;   // stop reading
        }
    }

    return state;
}


enum request_state
request_parser_feed(const uint8_t c, struct request_parser *rp) {
    switch(rp->state) {

        case request_reading_version:
            if(c == PROXY_SOCKS_REQUEST_SUPPORTED_VERSION)
                rp->state = request_reading_command;
            else {
                rp->reply = CONNECTION_NOT_ALLOWED_BY_RULESET; //Checkiar si esto es lo que hay que devolver...
                rp->state = request_error;
            }
            break;

        case request_reading_command:
            if(c == CONNECT_COMMAND)
                rp->state = request_reading_rsv;
            else {
                rp->reply = COMMAND_NOT_SUPPORTED;
                rp->state = request_error;
            }
            break;
        
        case request_reading_rsv:
            if(c == PROXY_SOCKS_REQUEST_RESERVED)
                rp->state = request_reading_atyp;
            else {
                rp->reply = CONNECTION_NOT_ALLOWED_BY_RULESET; // ?? RSV como que no me importa, podría directamente pasar al proximo estado y sacar este else?
                rp->state = request_error;
            }
            break;
        
        case request_reading_atyp:
            if(c == REQUEST_THROUGH_IPV4) {
                rp->addr_len = 4;
                rp->addr = calloc(4 + 1, sizeof(c));  // +1 for \0
                if(rp->addr == NULL) {
                    rp->state = request_error
;
                    rp->reply = GENERAL_SOCKS_SERVER_FAILURE; //Un error en un calloc debería tirar error del server, no?
                }
            } else if(c == REQUEST_THROUGH_IPV6) {
                rp->addr_len = 16;
                rp->addr = calloc(16 + 1, sizeof(c)); // +1 for \0
                if(rp->addr == NULL) {
                    rp->state = request_error
;
                    rp->reply = GENERAL_SOCKS_SERVER_FAILURE;
                }
            } else {
                rp->reply = ADDRESS_TYPE_NOT_SUPPORTED;
                rp->state = request_error;
                break;
            }
            
            rp->atyp = c;
            rp->state = request_reading_addr;
            break;

        case request_reading_addr:
            if(rp->addr_index == 0 && rp->addr_len == 0) {
            	if(c == 0) {
					rp->addr[rp->addr_index] = 0;
					rp->state = request_reading_port;
					break;
				}
                // reading first byte from FQDN
                rp->addr_len = c;
                rp->addr = calloc(c + 1, sizeof(c)); // +1 for \0
                if(rp->addr == NULL) {
                    rp->state = request_error
;
                    rp->reply = GENERAL_SOCKS_SERVER_FAILURE;
                }
            } else {
                // Save the byte
                rp->addr[rp->addr_index] = c;
				rp->addr_index++;
                if(rp->addr_index == rp->addr_len) {
                    // ya pusimos todos los bytes
                    rp->addr[rp->addr_index] = '\0';
                    rp->state = request_reading_port;
                }
            }
            break;
        
        case request_reading_port:
            if(rp->port == -1){ 
                // first time here
                rp->port = c * 256;
            } else {
                rp->port += c;
                rp->state = request_done;
            }
            break;
        
        case request_done:
            rp->reply = SUCCEDED;
            break;
            
        case request_error:
            rp->reply = GENERAL_SOCKS_SERVER_FAILURE;
            break;
        
        default:
            // Impossible state!
            exit(EXIT_FAILURE);
    }
    return rp->state;
}
