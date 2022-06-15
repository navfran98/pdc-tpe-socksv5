
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

#define ENOUGH_SPACE_TO_auth_LOG 150

// Prototypes

unsigned
auth_init(const unsigned state, struct selector_key *key){
    struct auth_stm * authstm = &ATTACHMENT(key)->client.auth;

    authstm->rb = &(ATTACHMENT(key)->read_buffer);
    authstm->wb = &(ATTACHMENT(key)->write_buffer);

    auth_parser_init(&authstm->auth_parser);
    authstm->reply = AUTH_FAIL;

    return state; //TODO: hacer que esta función no devuelva state...


}


unsigned
auth_read(struct selector_key *key) {
    struct auth_stm *authstm = &ATTACHMENT(key)->client.auth;
    struct socksv5 *s5 = ATTACHMENT(key);

    size_t nbytes;
    uint8_t *where_to_write = buffer_write_ptr(authstm->rb, &nbytes);
    ssize_t n = recv(key->fd, where_to_write, nbytes, 0);  // Non blocking !

    uint8_t state_toret = AUTH_READ; // current state
    if(n > 0) {
        buffer_write_adv(authstm->rb, n);
        enum auth_state state = consume_auth_buffer(authstm->rb, &authstm->auth_parser);
        if(state == auth_done || state == auth_bad_syntax || state == auth_unsupported_version || state == auth_bad_length) {

            if(state == auth_done){
                if(authenticate_user(authstm->auth_parser.user, authstm->auth_parser.password)){
                    authstm->reply = AUTH_SUCCESS;

                    struct user authenticated_user = {
                            .name = (char*)authstm->auth_parser.user,
                            .pass = (char*)authstm->auth_parser.password
                    };
                    memcpy(&s5->connected_user, &authenticated_user, sizeof(authenticated_user));
                }else{
                    authstm->reply  = AUTH_FAIL;
                }
            }

            if(selector_set_interest_key(key, OP_WRITE) != SELECTOR_SUCCESS) {
                goto finally;
            }


            auth_marshall(authstm->wb, authstm->reply);

            state_toret = AUTH_WRITE;

        }
    } else {
        goto finally;
    }
    return state_toret;

finally:

    return ERROR_GLOBAL_STATE;
}


unsigned
auth_write(struct selector_key *key) {
    struct auth_stm *authstm = &ATTACHMENT(key)->client.auth;

    size_t nbytes;
    uint8_t *where_to_read = buffer_read_ptr(authstm->wb, &nbytes);
    ssize_t ret = send(key->fd, where_to_read, nbytes, MSG_NOSIGNAL);

    uint8_t state_toret = AUTH_WRITE; // current state
    if(ret > 0) {
        buffer_read_adv(authstm->wb, nbytes);
        if(!buffer_can_read(authstm->wb)) {

            if(selector_set_interest_key(key, OP_READ) != SELECTOR_SUCCESS) {
                goto finally;
            }

            if(authstm->reply == AUTH_SUCCESS) {
                // Ya estamos listos para leer el Request
                state_toret = REQUEST_READ;
            } else {
                state_toret = DONE;
            }

        }
    } else {
        goto finally;
    }
    return state_toret;

finally:
    return ERROR_GLOBAL_STATE;
}
