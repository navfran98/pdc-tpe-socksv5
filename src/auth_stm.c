#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

#include "../headers/auth_parser.h"
#include "../headers/auth_stm.h"
#include "../headers/socksv5_server.h"
#include "../headers/socksv5_stm.h"
#include "../headers/logger.h"
#include "../headers/parameters.h"

unsigned
auth_init(const unsigned state, struct selector_key *key){
    struct auth_stm * auth_stm = &ATTACHMENT(key)->auth;

    auth_stm->rb = &(ATTACHMENT(key)->read_buffer);
    auth_stm->wb = &(ATTACHMENT(key)->write_buffer);

    auth_parser_init(&auth_stm->auth_parser);
    auth_stm->reply = AUTH_FAIL;
    return state;
}

unsigned
auth_read(struct selector_key *key) {
    struct auth_stm * auth_stm = &ATTACHMENT(key)->auth;
    struct socksv5 * socksv5 = ATTACHMENT(key);

    size_t nbytes;
    uint8_t * where_to_write = buffer_write_ptr(auth_stm->rb, &nbytes);
    ssize_t n = recv(key->fd, where_to_write, nbytes, 0); 

    uint8_t ret_state = AUTH_READ; // current state
    if(n > 0) {
        buffer_write_adv(auth_stm->rb, n);
    
        enum auth_state state = consume_auth_buffer(auth_stm->rb, &auth_stm->auth_parser);
        if(state == auth_done || state == auth_bad_syntax || state == auth_unsupported_version || state == auth_bad_length) {
            if(state == auth_done){
                if(authenticate_user(auth_stm->auth_parser.user, auth_stm->auth_parser.password)){
                    auth_stm->reply = AUTH_SUCCESS;
                    
                    struct user authenticated_user = {
                        .name = (char*)auth_stm->auth_parser.user,
                        .pass = (char*)auth_stm->auth_parser.password
                    };
                    memcpy(&socksv5->connected_user, &authenticated_user, sizeof(authenticated_user));
                }else{
                    auth_stm->reply  = AUTH_FAIL;
                }
            }

            if(selector_set_interest_key(key, OP_WRITE) != SELECTOR_SUCCESS) {
                goto finally;
            }
            auth_fill_msg(auth_stm->wb, auth_stm->reply);
            ret_state = AUTH_WRITE;
        }
    } else {
        goto finally;
    }
    return ret_state;
finally:
    return ERROR;
}

unsigned
auth_write(struct selector_key *key) {
    struct auth_stm *auth_stm = &ATTACHMENT(key)->auth;

    size_t nbytes;
    uint8_t * where_to_read = buffer_read_ptr(auth_stm->wb, &nbytes);
    ssize_t ret = send(key->fd, where_to_read, nbytes, 0);

    uint8_t ret_state = AUTH_WRITE;
    if(ret > 0) {
        buffer_read_adv(auth_stm->wb, nbytes);
        if(!buffer_can_read(auth_stm->wb)) {
            if(selector_set_interest_key(key, OP_READ) != SELECTOR_SUCCESS) {
                goto finally;
            }

            if(auth_stm->reply == AUTH_SUCCESS) {
                ret_state = REQUEST_READ;
            } else {
                ret_state = DONE;
            }
        }
    } else {
        goto finally;
    }
    return ret_state;

finally:
    return ERROR;
}
