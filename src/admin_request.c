// Aca se define el request_read y request_write
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>

#include "../headers/admin_request_parser.h"
#include "../headers/admin_request.h"
#include "../headers/admin_server.h"
#include "../headers/admin_stm.h"
#include "../headers/buffer.h"


unsigned
admin_request_init(const unsigned state, struct selector_key *key){
    printf("Request_Init\n");
    struct admin_request_stm * admin_request_stm = &ADMIN_ATTACHMENT(key)->admin_req_stm;

    admin_request_stm->rb = &(ADMIN_ATTACHMENT(key)->read_buffer);
    admin_request_stm->wb = &(ADMIN_ATTACHMENT(key)->write_buffer);

    admin_req_parser_init(&admin_request_stm->admin_request_parser);
    return state;
}

unsigned
admin_request_read(struct selector_key *key) {
    printf("Request_Read\n");
    struct admin_request_stm * admin_request_stm = &ADMIN_ATTACHMENT(key)->admin_req_stm;

    size_t nbytes;
    uint8_t * where_to_write = buffer_write_ptr(admin_request_stm->rb, &nbytes);
    ssize_t ret = recv(key->fd, where_to_write, nbytes, 0); 

    uint8_t ret_state = ADMIN_REQUEST; // current state
    if(ret > 0) {
        buffer_write_adv(admin_request_stm->rb, ret);
        /*enum admin_req_state cmd = */admin_consume_req_buffer(admin_request_stm->rb, &admin_request_stm->admin_request_parser);
        if(selector_set_interest_key(key, OP_WRITE) != SELECTOR_SUCCESS) {
            goto finally;
        }
        return ret_state;
    }
finally:
    return ADMIN_ERROR;
}

unsigned
admin_request_write(struct selector_key *key) {
    printf("Request_Write\n");
    struct admin_request_stm * admin_request_stm = &ADMIN_ATTACHMENT(key)->admin_req_stm;

    admin_req_fill_msg(admin_request_stm->wb, &admin_request_stm->admin_request_parser);

    size_t nbytes;
    uint8_t * where_to_read = buffer_read_ptr(admin_request_stm->wb, &nbytes);
    ssize_t ret = send(key->fd, where_to_read, nbytes, 0);

    uint8_t ret_state = ADMIN_REQUEST;
    if(ret > 0) {
        buffer_read_adv(admin_request_stm->wb, ret);
        if(!buffer_can_read(admin_request_stm->wb)) {
            if(selector_set_interest_key(key, OP_READ) != SELECTOR_SUCCESS) {
                goto finally;
            }
        }
        return ret_state;
    }
finally:
    return ADMIN_ERROR;
}
