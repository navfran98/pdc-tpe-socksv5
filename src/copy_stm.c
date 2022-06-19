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
#include "../headers/parameters.h"
#include "../headers/pop3_sniffer.h"

static void sniff_pop3(struct selector_key * key, uint8_t * ptr, ssize_t size);
static enum socksv5_global_state check_close_connection(struct selector_key * key, struct copy_stm * cp_stm);

unsigned copy_init(const unsigned state, struct selector_key * key) {
    struct copy_stm * cp_stm = &ATTACHMENT(key)->copy;
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
    struct copy_stm * copy_stm = &ATTACHMENT(key)->copy;

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


        if(key->fd == ATTACHMENT(key)->origin_fd && parameters->disectors_enabled){
            sniff_pop3(key,where_to_write,ret);
        }

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
    return ERROR;
}


unsigned
copy_write(struct selector_key *key) {
    struct socksv5 * socksv5 = ATTACHMENT(key);
    struct copy_stm * cp_stm = &ATTACHMENT(key)->copy;

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

        if(key->fd == ATTACHMENT(key)->origin_fd && parameters->disectors_enabled){
            sniff_pop3(key,where_to_read,ret);
        }

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
    return ERROR;
}

static enum socksv5_global_state check_close_connection(struct selector_key * key, struct copy_stm * cp_stm) {
    if(cp_stm->reading_client == false && cp_stm->reading_origin == false && cp_stm->writing_origin == false && cp_stm->writing_client == false) {
        log_new_connection("Client disconnected", key);
        return DONE;
    }
    return COPY;
}

static void sniff_pop3(struct selector_key *key, uint8_t * ptr, ssize_t size){

    struct pop3_sniffer * sniffer = &ATTACHMENT(key)->pop3sniffer;
    
    if(!sniffer->is_parsing){
        pop3_sniffer_init(sniffer);
    }
    if(sniffer->state != POP3_FINISH && sniffer->state != POP3_SNIFF_SUCCESSFUL){
        size_t count;
        uint8_t *pop3_ptr = buffer_write_ptr(&sniffer->buffer, &count);

        if((unsigned) size <= count){
            memcpy(pop3_ptr, ptr, size);
            buffer_write_adv(&sniffer->buffer, size);
        }
        else{
            memcpy(pop3_ptr, ptr, count);
            buffer_write_adv(&sniffer->buffer, count);
        }
        pop3_sniffer_consume(sniffer);
    }
}




