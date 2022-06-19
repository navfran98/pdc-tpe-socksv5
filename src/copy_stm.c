#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "../headers/copy_stm.h"
#include "../headers/socksv5_server.h"
#include "../headers/socksv5_stm.h"
#include "../headers/logger.h"
#include "../headers/metrics.h"

static enum socksv5_global_state check_close_connection(struct selector_key * key, struct copy_stm * cp_stm);

unsigned copy_init(const unsigned state, struct selector_key * key) {
    struct copy_stm * cp_stm = &ATTACHMENT(key)->client.copy;

    cp_stm->rb = &(ATTACHMENT(key)->read_buffer);
    cp_stm->wb = &(ATTACHMENT(key)->write_buffer);
    
    cp_stm->reading_client = true;
    cp_stm->writing_client = true;
    cp_stm->reading_origin = true;
    cp_stm->writing_origin = true;

    return state;
}


unsigned copy_read(struct selector_key *key) {
    struct socksv5 * socksv5 = ATTACHMENT(key);
    struct copy_stm * copy_stm = &ATTACHMENT(key)->client.copy;

    buffer * buff;
    uint8_t fd_target;

    if(key->fd == socksv5->client_fd) {
        // Estamos leyendo del cliente
        buff = copy_stm->wb;
        fd_target = socksv5->origin_fd;

    } else if(key->fd == socksv5->origin_fd) {
        // Estamos leyendo del origin
        buff = copy_stm->rb;
        fd_target = socksv5->client_fd;

    } else {
        exit(EXIT_FAILURE);
    }

    size_t nbytes;
    uint8_t * where_to_write = buffer_write_ptr(buff, &nbytes);
    ssize_t ret = recv(key->fd, where_to_write, nbytes, 0);
    if(ret > 0) {
        // Sumo los bytes recibidos
        add_total_bytes_transferred(ret);

        buffer_write_adv(buff, ret);

        if(selector_set_interest_reduction(key->s, key->fd, OP_READ) != SELECTOR_SUCCESS) {
            goto finally;
        }
        if(selector_set_interest_additive(key->s, fd_target, OP_WRITE) != SELECTOR_SUCCESS) {
            goto finally;
        }

    } else if(ret == 0 || errno == ECONNRESET) {
        if(selector_set_interest_reduction(key->s, key->fd, OP_READ) != SELECTOR_SUCCESS) {
            goto finally;
        }
        
        if(key->fd == socksv5->origin_fd) {
            copy_stm->reading_origin = false;
        } else if(key->fd == socksv5->client_fd) {
            copy_stm->reading_client = false;
        }

        if (!buffer_can_read(buff)) {
            if (shutdown(fd_target, SHUT_WR) < 0) {
                goto finally;
            }
            if(key->fd == socksv5->origin_fd) {
                copy_stm->writing_client = false;
            } else if(key->fd == socksv5->client_fd) {
                copy_stm->writing_origin = false;
            }
        }
    } else {
        goto finally;
    }
    return check_close_connection(key, copy_stm);
finally:
    return ERROR_GLOBAL_STATE;
}


unsigned
copy_write(struct selector_key *key) {
    struct socksv5 * socksv5 = ATTACHMENT(key);
    struct copy_stm * cp_stm = &ATTACHMENT(key)->client.copy;

    buffer * buff;
    uint8_t fd_target;

    if(key->fd == socksv5->client_fd){
        // Estamos escribiendo al cliente
        buff = cp_stm->rb;
        fd_target = socksv5->origin_fd;
    } else if(key->fd == socksv5->origin_fd){
        // Estamos escribiendo al origin
        buff = cp_stm->wb;
        fd_target = socksv5->client_fd;
    } else {
        // impossible
        exit(EXIT_FAILURE);
    }

    size_t nbytes;
    uint8_t * where_to_read = buffer_read_ptr(buff, &nbytes);
    ssize_t ret = send(key->fd, where_to_read, nbytes, 0);
    if(ret > 0) {
        buffer_read_adv(buff, ret);
        if(!buffer_can_read(buff)) {
            if(selector_set_interest_reduction(key->s, key->fd, OP_WRITE) != SELECTOR_SUCCESS) {
                goto finally;
            }

            if(key->fd == socksv5->client_fd && cp_stm->reading_origin == false) {
                cp_stm->writing_client = false;
                if(shutdown(key->fd, SHUT_WR) < 0) {
                    goto finally;
                }
            } else if(key->fd == socksv5->origin_fd && cp_stm->reading_client == false) {
                cp_stm->writing_origin = false;
                if(shutdown(key->fd, SHUT_WR) < 0) {
                    goto finally;
                }
            }
        }

        if((key->fd == socksv5->client_fd && cp_stm->reading_origin == true) ||
            (key->fd == socksv5->origin_fd && cp_stm->reading_client == true)) {
            if (selector_set_interest_additive(key->s, fd_target, OP_READ) != SELECTOR_SUCCESS) {
                goto finally;
            }
        }
        return check_close_connection(key, cp_stm);
    } else {
        goto finally;
    }
finally:
    return ERROR_GLOBAL_STATE;
}

static enum socksv5_global_state check_close_connection(struct selector_key * key, struct copy_stm * cp_stm) {
    if(cp_stm->reading_client == false && cp_stm->reading_origin == false && cp_stm->writing_origin == false && cp_stm->writing_client == false) {
        printf("ATYP -> %d\n", ATTACHMENT(key)->client.request.request_parser.atyp);
        log_new_connection("Client disconnected", key);
        return DONE;
    }
    return COPY;
}
