#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <sys/socket.h>
#include <string.h>

#include "../headers/greeting_stm.h"
#include "../headers/greeting_parser.h"
#include "../headers/socksv5_server.h"
#include "../headers/socksv5_stm.h"
#include "../headers/logger.h"

static int select_method(uint8_t * methods, uint8_t number_of_methods);

unsigned
greeting_init(const unsigned state, struct selector_key *key) {
	struct greeting_stm * g_stm = &ATTACHMENT(key)->greeting;
    g_stm->rb = &(ATTACHMENT(key)->read_buffer);
    g_stm->wb = &(ATTACHMENT(key)->write_buffer);

    g_stm->method_selected = -1;  
    greeting_parser_init(&g_stm->greeting_parser);

    return state;
}

unsigned
greeting_read(struct selector_key *key) {
    struct greeting_stm * g_stm = &ATTACHMENT(key)->greeting;

	size_t nbytes;
    uint8_t * where_to_write = buffer_write_ptr(g_stm->rb, &nbytes);
    ssize_t ret = recv(key->fd, where_to_write, nbytes, 0);
    uint8_t ret_state = GREETING_READ;

    if(ret > 0) {
        buffer_write_adv(g_stm->rb, ret);
        enum greeting_state state = consume_greeting_buffer(g_stm->rb, &g_stm->greeting_parser);
        if(state == greeting_done || state == greeting_bad_syntax || state == greeting_unsupported_version) {
                        
            g_stm->method_selected = select_method(g_stm->greeting_parser.methods, g_stm->greeting_parser.methods_remaining);
            if(selector_set_interest_key(key, OP_WRITE) != SELECTOR_SUCCESS) {
                goto finally;
            }

            if(greeting_marshall(g_stm->wb, g_stm->method_selected) == -1){
                return ERROR;
            }

            ret_state = GREETING_WRITE;
        } else{
        }
    } else {
        goto finally;
    }
    return ret_state;
finally:
    return ERROR;
}


static int
select_method(uint8_t * methods, uint8_t number_of_methods) {
    int ret = NO_ACCEPTABLE_METHODS;

    if(methods != NULL) {
        for(uint8_t i = 0; i < number_of_methods; i++) {
            if(methods[i] == USERNAME_PASSWORD_AUTH) {
                return methods[i]; 
            } else if(methods[i] == NO_AUTHENTICATION_REQUIRED) {
                ret = methods[i];
            }
        }
    }
    return ret;
}

unsigned
greeting_write(struct selector_key *key) {
    struct greeting_stm *g_stm = &ATTACHMENT(key)->greeting;

    size_t nbytes;
    uint8_t * where_to_read = buffer_read_ptr(g_stm->wb, &nbytes);

    ssize_t ret = send(key->fd, where_to_read, nbytes, 0);
    uint8_t ret_state = GREETING_WRITE;
    if(ret > 0) {
        buffer_read_adv(g_stm->wb, nbytes);
        if(!buffer_can_read(g_stm->wb)) {
            if(selector_set_interest_key(key, OP_READ) != SELECTOR_SUCCESS) {
                goto finally;
            }
            if(g_stm->method_selected == NO_AUTHENTICATION_REQUIRED) {
                ret_state = REQUEST_READ;
            } else if(g_stm->method_selected == USERNAME_PASSWORD_AUTH) {
                ret_state = AUTH_READ;
            } else {
                goto finally;
            }
        }
    } else {
        goto finally;
    }
    return ret_state;
finally:
    return ERROR;
}
