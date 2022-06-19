#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "../headers/request_parser.h"

void
request_parser_init(struct request_parser * req_pars) {
    req_pars->state = request_reading_version;
    req_pars->addr_index = 0;
    req_pars->atyp = -1;
    req_pars->port = -1;  
    req_pars->addr_len = -1;
    req_pars->addr = NULL;
}

enum request_state
consume_request_buffer(buffer * buff, struct request_parser *req_pars) {
    enum request_state state = req_pars->state; 

    while(buffer_can_read(buff)) {
        const uint8_t c = buffer_read(buff);
        state = request_parser_feed(c, req_pars);
        if(state == request_done) {
            request_parser_feed(c, req_pars);
            break;
        } else if(state == request_error) {
            break; 
        }
    }
    return state;
}

enum request_state
request_parser_feed(const uint8_t c, struct request_parser * req_pars) {
    switch(req_pars->state) {

        case request_reading_version:
            if(c == PROXY_SOCKS_REQUEST_SUPPORTED_VERSION)
                req_pars->state = request_reading_command;
            else {
                req_pars->reply = CONNECTION_NOT_ALLOWED_BY_RULESET; 
                req_pars->state = request_error;
            }
            break;

        case request_reading_command:
            if(c == CONNECT_COMMAND)
                req_pars->state = request_reading_rsv;
            else {
                req_pars->reply = COMMAND_NOT_SUPPORTED;
                req_pars->state = request_error;
            }
            break;
        
        case request_reading_rsv:
            if(c == PROXY_SOCKS_REQUEST_RESERVED)
                req_pars->state = request_reading_atyp;
            else {
                req_pars->reply = CONNECTION_NOT_ALLOWED_BY_RULESET; 
                req_pars->state = request_error;
            }
            break;
        
        case request_reading_atyp:
            if(c == IPV4) {
                req_pars->addr_len = 4;
                req_pars->addr = calloc(4 + 1, sizeof(c));
                if(req_pars->addr == NULL) {
                    req_pars->state = request_error;
                    req_pars->reply = GENERAL_SOCKS_SERVER_FAILURE; 
                }
            } else if(c == IPV6) {
                req_pars->addr_len = 16;
                req_pars->addr = calloc(16 + 1, sizeof(c)); 
                if(req_pars->addr == NULL) {
                    req_pars->state = request_error;
                    req_pars->reply = GENERAL_SOCKS_SERVER_FAILURE;
                }
            }else if(c==FQDN){
                req_pars->addr_len = 0;
            } else {
                req_pars->reply = ADDRESS_TYPE_NOT_SUPPORTED;
                req_pars->state = request_error;
                break;
            }
            req_pars->atyp = c;
            req_pars->state = request_reading_addr;
            break;

        case request_reading_addr:
            if(req_pars->addr_index == 0 && req_pars->addr_len == 0) {
                if(c == 0) {
					req_pars->addr[req_pars->addr_index] = 0;
					req_pars->state = request_reading_port; //El fqdn está vacío, pero no me importa dejo que rompa cuando intente conectarse.
					break;
				}
                req_pars->addr_len = c;
                req_pars->addr = calloc(c + 1, sizeof(c)); 
                if(req_pars->addr == NULL) {
                    req_pars->state = request_error;
                    req_pars->reply = GENERAL_SOCKS_SERVER_FAILURE;
                }
            } else {
                req_pars->addr[req_pars->addr_index] = c;
				req_pars->addr_index++;
                if(req_pars->addr_index == req_pars->addr_len) {
                    req_pars->addr[req_pars->addr_index] = '\0';
                    req_pars->state = request_reading_port;
                }
            }
            break;
        
        case request_reading_port:
            if(req_pars->port == -1){ 
                req_pars->port = c * 256;
            } else {
                req_pars->port += c;
                req_pars->state = request_done;
            }
            break;
        
        case request_done:
            req_pars->reply = SUCCEDED;
            break;
            
        case request_error:
            req_pars->reply = GENERAL_SOCKS_SERVER_FAILURE;
            break;
        
        default:
            exit(EXIT_FAILURE);
    }
    return req_pars->state;
}

void request_fill_msg(buffer * buff, struct request_parser * req_pars){
    size_t space_left_to_write;
    uint8_t *where_to_write = buffer_write_ptr(buff, &space_left_to_write);

    uint16_t total_space = 10;

    int i = 0;

    where_to_write[i++] = PROXY_SOCKS_REQUEST_SUPPORTED_VERSION;
    where_to_write[i++] = req_pars->reply;
    where_to_write[i++] = 0x00;  // RSV

    where_to_write[i++] = IPV4;
    for(int j = i; j < IPv4_LENGTH + i; j++) {
        where_to_write[j] = 0x00;   // BIND.ADDR
    }
    i += IPv4_LENGTH;
    where_to_write[i++] = 0x00;  // BIND.PORT
    where_to_write[i]   = 0x00;  // BIND.PORT

    buffer_write_adv(buff, total_space);
}

