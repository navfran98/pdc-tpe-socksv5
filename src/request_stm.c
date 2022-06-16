#include <unistd.h>
#include <sys/socket.h> 
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <arpa/inet.h>

#include "../headers/selector.h"
#include "../headers/request_stm.h"
#include "../headers/socksv5_server.h"
#include "../headers/socksv5_stm.h"
#include "../headers/logger.h"

#define ENOUGH_SPACE_TO_CONNECTION_LOG 200

unsigned
request_read_init(const unsigned state, struct selector_key *key) {
    printf("REQUEST INIT\n");
	struct request_stm * req_stm = &ATTACHMENT(key)->client.request;

    req_stm->rb = &(ATTACHMENT(key)->read_buffer);
    req_stm->wb = &(ATTACHMENT(key)->write_buffer);

    request_parser_init(&req_stm->request_parser);

    req_stm->origin_addrinfo = calloc(1, sizeof(struct addrinfo));
    if(req_stm->origin_addrinfo == NULL) {
        goto finally;
    }
    return state;
finally:
    return ERROR_GLOBAL_STATE;
}


unsigned
request_read(struct selector_key *key) {
    printf("REQUEST READ\n");
    struct request_stm *req_stm = &ATTACHMENT(key)->client.request;

    size_t nbytes;
    uint8_t * where_to_write = buffer_write_ptr(req_stm->rb, &nbytes);
    ssize_t ret = recv(key->fd, where_to_write, nbytes, 0);

    enum socksv5_global_state ret_state = REQUEST_READ;
    if(ret > 0) {
        buffer_write_adv(req_stm->rb, ret);
        enum request_state state = consume_request_buffer(req_stm->rb, &req_stm->request_parser);
        if(state == request_done) {
            if(selector_set_interest_key(key, OP_NOOP) != SELECTOR_SUCCESS) {
                goto finally;
            }
			ret_state = ORIGIN_CONNECT; 
        }else if(state == request_error) {
            if(selector_set_interest_key(key, OP_WRITE) != SELECTOR_SUCCESS) {
                goto finally;
            }
            ret_state = REQUEST_WRITE;
        }
    } else {
        goto finally;
    }
    return ret_state;
finally:
    return ERROR_GLOBAL_STATE;
}

unsigned
request_write(struct selector_key *key) {
    struct socksv5 * socksv5 = ATTACHMENT(key);
    struct request_stm * req_stm = &ATTACHMENT(key)->client.request;

    request_marshall(req_stm->wb, &req_stm->request_parser);

    size_t nbytes;
    uint8_t * where_to_read = buffer_read_ptr(req_stm->wb, &nbytes);
    ssize_t ret = send(key->fd, where_to_read, nbytes, 0);

    if(ret > 0) {
        buffer_read_adv(req_stm->wb, nbytes);
        if(!buffer_can_read(req_stm->wb)) {
            if(req_stm->request_parser.reply == SUCCEDED) {
                if(selector_set_interest_key(key, OP_READ) != SELECTOR_SUCCESS) {
                    goto finally;
                }
                if(selector_set_interest(key->s, socksv5->origin_fd, OP_READ) != SELECTOR_SUCCESS) {
                    goto finally;
                }
                return DONE; //ACA IRIA EL ESTADO DONDE YA ESTAN CONECTADOS Y EMPIEZAN A HABLARSE
            } else {
                return DONE;
            }
        } else {
            return REQUEST_WRITE;
        }
    } else {
        goto finally;
    }

finally:
    return ERROR_GLOBAL_STATE;
}
