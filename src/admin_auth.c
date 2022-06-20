// Aca se define el greeting_read, greeting_write y otra funcion auxiliar para autenticar

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

#include "../headers/admin_auth_parser.h"
#include "../headers/admin_auth.h"
#include "../headers/admin_server.h"
#include "../headers/admin_stm.h"
#include "../headers/buffer.h"
#include "../headers/socksv5_stm.h"
#include "../headers/parameters.h"

unsigned
admin_auth_init(const unsigned state, struct selector_key *key){
    printf("Auth init\n");
    struct admin_auth_stm * admin_auth_stm = &ADMIN_ATTACHMENT(key)->admin_auth_stm;

    admin_auth_stm->rb = &(ADMIN_ATTACHMENT(key)->read_buffer);
    admin_auth_stm->wb = &(ADMIN_ATTACHMENT(key)->write_buffer);

    admin_auth_parser_init(&admin_auth_stm->admin_auth_parser);
    return state;
}


unsigned
admin_auth_read(struct selector_key *key) {
    printf("Auth read\n");
    struct admin_auth_stm * admin_auth_stm = &ADMIN_ATTACHMENT(key)->admin_auth_stm;

    size_t nbytes;
    uint8_t * where_to_write = buffer_write_ptr(admin_auth_stm->rb, &nbytes);
    ssize_t n = recv(key->fd, where_to_write, nbytes, 0); 
    
    uint8_t ret_state = ADMIN_AUTH; // current state
    if(n > 0) {
        printf("Recibí algo!!!\n");
        buffer_write_adv(admin_auth_stm->rb, n);
        enum admin_auth_state state = admin_consume_auth_buffer(admin_auth_stm->rb, &admin_auth_stm->admin_auth_parser);
        printf("STATE: %d\n", state);
        if(state == admin_auth_done || state == admin_auth_bad_syntax){
            if(state == admin_auth_done){
                admin_auth_stm->admin_auth_parser.ret = AUTH_BAD_CREDENTIALS;
                if(authenticate_admin(admin_auth_stm->admin_auth_parser.user, admin_auth_stm->admin_auth_parser.password)){
                    admin_auth_stm->admin_auth_parser.ret = AUTH_SUCCESS;
                }
            }else if(state == admin_auth_bad_syntax){
                admin_auth_stm->admin_auth_parser.ret = AUTH_BAD_SINTAX;
            }
            if(selector_set_interest_key(key, OP_WRITE) != SELECTOR_SUCCESS) {
                goto finally;
            }
        }
        printf("Y se lo llevó...\n");
        return ret_state;
    }
finally:
    return ADMIN_ERROR;
}

unsigned
admin_auth_write(struct selector_key *key) {
    printf("EN el auth write!!!\n");
    struct admin_auth_stm * admin_auth_stm = &ADMIN_ATTACHMENT(key)->admin_auth_stm;

    admin_auth_fill_msg(admin_auth_stm->wb, admin_auth_stm->admin_auth_parser.ret);

    size_t nbytes;
    uint8_t * where_to_read = buffer_read_ptr(admin_auth_stm->wb, &nbytes);
    ssize_t ret = send(key->fd, where_to_read, nbytes, 0);

    uint8_t ret_state = ADMIN_AUTH;
    if(ret > 0) {
        printf("Mandé algo!!\n");
        buffer_read_adv(admin_auth_stm->wb, ret);
        if(!buffer_can_read(admin_auth_stm->wb)) {
            if(selector_set_interest_key(key, OP_READ) != SELECTOR_SUCCESS) {
                goto finally;
            }
            if(admin_auth_stm->admin_auth_parser.ret == AUTH_SUCCESS) {
                ret_state = ADMIN_REQUEST;
            } else {
                ret_state = ADMIN_DONE;
            }
        }
        return ret_state;
    }
finally:
    return ADMIN_ERROR;
}
