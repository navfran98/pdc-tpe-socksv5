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
            if(c == REQUEST_THROUGH_IPV4) {
                req_pars->addr_len = 4;
                req_pars->addr = calloc(4 + 1, sizeof(c));
                if(req_pars->addr == NULL) {
                    req_pars->state = request_error;
                    req_pars->reply = GENERAL_SOCKS_SERVER_FAILURE; 
                }
            } else if(c == REQUEST_THROUGH_IPV6) {
                req_pars->addr_len = 16;
                req_pars->addr = calloc(16 + 1, sizeof(c)); 
                if(req_pars->addr == NULL) {
                    req_pars->state = request_error;
                    req_pars->reply = GENERAL_SOCKS_SERVER_FAILURE;
                }
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
					req_pars->state = request_reading_port;
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

void request_marshall(buffer * buff, struct request_parser * req_pars){
    size_t space_left;
    uint8_t * where_to_write = buffer_write_ptr(buff, &space_left);
    int len = floor(log10(abs(req_pars->port)))+1; 
    if(req_pars->atyp == 3){
        if(space_left < 6 + req_pars->addr_len + len){
            // return -1;
            return;
        }
        where_to_write[0] = PROXY_SOCKS_REQUEST_SUPPORTED_VERSION;
        where_to_write[1] = req_pars->reply;
        where_to_write[2] = 0x00;
        where_to_write[3] = req_pars->atyp;
        where_to_write[4] = req_pars->addr_len;
        int i = 5;
        for(int j = 0; i < req_pars->addr_len; i++, j++){
            where_to_write[i] = req_pars->addr[j];
        }
        snprintf(where_to_write, len, "%d", req_pars->port);
        buffer_write_adv(buff, i+len);
        // return 2;
        return;
    } else {
        if(space_left < 5 + req_pars->addr_len + len){
            // return -1;
            return;
        }
        where_to_write[0] = PROXY_SOCKS_REQUEST_SUPPORTED_VERSION;
        where_to_write[1] = req_pars->reply;
        where_to_write[2] = 0x00;
        where_to_write[3] = req_pars->atyp;
        int i = 4;
        for(int j = 0; i < req_pars->addr_len; i++, j++){
            where_to_write[i] = req_pars->addr[j];
        }
        int len = floor(log10(abs(req_pars->port)))+1; 
        snprintf(where_to_write, len, "%d", req_pars->port);
        buffer_write_adv(buff, i+len);
        // return 2;
        return;
    }
}
