#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/socket.h>
#include <netdb.h>
#include "../headers/metrics.h"
#include "../headers/logger.h"
#include "../headers/selector.h"
#include "../headers/socksv5_server.h"
#include "../headers/socksv5_stm.h"

static struct socksv5 * first = NULL; 

void socksv5_passive_accept(struct selector_key *key) {

    struct sockaddr_storage new_client_addr;
    struct socksv5 * socksv5 = NULL;
    socklen_t new_client_addr_len = sizeof(new_client_addr);
    if (get_concurrent_connections() < MAX_CONCURRENT_CONNECTIONS) {
        const int client_sock = accept(key->fd, (struct sockaddr*)&new_client_addr, &new_client_addr_len);

        if(client_sock == -1) {
            goto finally;
        } 

        if(selector_fd_set_nio(client_sock) == -1) {
            goto finally;
        }
        
        socksv5 = new_socksv5(client_sock);
        if(socksv5 == NULL) {
            goto finally;
        }
        
        memcpy(&socksv5->client_addr, &new_client_addr, new_client_addr_len);

        /*if (get_concurrent_connections() >= MAX_CONCURRENT_CONNECTIONS) {
            if(selector_set_interest_key(key, OP_NOOP) != SELECTOR_SUCCESS)
                goto finally;
        }*/

        if(selector_register(key->s, client_sock, &socksv5_active_handler, OP_READ, socksv5) != SELECTOR_SUCCESS) {
        //Ocurrio un error
            goto finally;
        }
        add_connection();
        return;
    finally:
        if(client_sock != -1) {
            close(client_sock);
        }
        if(socksv5 != NULL) {
            socksv5_destroy(key->data);
        }
        
    }
    return;
}

struct socksv5 * new_socksv5(int client_fd) {
    //Creamos la estructura socksv5
    struct socksv5 * socksv5 = calloc(1,sizeof(*socksv5));
    if(socksv5 == NULL){
        goto finally;
    }

    //Lo ubicamos en la lista
    if (first == NULL) {
        first = socksv5;
        socksv5->prev = NULL;
        socksv5->next = NULL;
    } else {
        struct socksv5 * aux = first;
        while(aux->next != NULL){
            aux = aux->next;
        }
        aux->next = socksv5;
        socksv5->prev = aux;
        socksv5->next = NULL;
    }
    
    //Se setean los atributos
    memset(socksv5, 0x00, sizeof(struct socksv5));

    socksv5->origin_fd = -1;
    socksv5->client_fd = client_fd;

    socksv5->stm.initial = GREETING_READ;
    socksv5->stm.max_state = ERROR;
    socksv5->stm.states = socksv5_describe_states();

    stm_init(&socksv5->stm);

    buffer_init(&socksv5->read_buffer,  N(socksv5->raw_buff_a), socksv5->raw_buff_a);
    buffer_init(&socksv5->write_buffer, N(socksv5->raw_buff_b), socksv5->raw_buff_b);

    finally:
    return socksv5;
}

void
destroy_socksv5_pool(void) {
    struct socksv5 * current = first;
    struct socksv5 * aux = current;

    while(aux != NULL) {
        current = aux;
        aux = aux->next;
        socksv5_destroy(current);
        free(current);
    }
}

void
socksv5_done(struct selector_key * key) {

    // 1. Cerramos los fd y los desregistramos del selector
    const int fds[] = {
            ATTACHMENT(key)->client_fd,
            ATTACHMENT(key)->origin_fd,
    };

    for(unsigned i = 0; i < N(fds); i++) {
        if(fds[i] != -1) {
            if(SELECTOR_SUCCESS != selector_unregister_fd(key->s, fds[i])) {
                abort();
            }
            close(fds[i]);
        }
    }
    
    // 2. Removemos la conexion de las metricas
    log_new_connection("Client disconnected", key);
    remove_connection();
    
    // 3. Lo eliminamos de la lista y eliminamos la estructura
    socksv5_destroy(ATTACHMENT(key));
}

void
socksv5_destroy(struct socksv5 *s) {
    //Lo saco de la lista
    if(s != NULL){
        if(s->next != NULL){
            s->next->prev = s->prev;
        }
        if(s->prev != NULL){
            s->prev->next = s->next;
        } else {
            first = s->next;
        } 
        // Elimino la estructura y hago los frees
    }
}


void
socksv5_read(struct selector_key *key) {
    struct state_machine *stm  = &ATTACHMENT(key)->stm;

    enum socksv5_global_state state = (enum socksv5_global_state) stm_handler_read(stm, key);

    if(state == ERROR || state == DONE) {
        socksv5_done(key);
    }
}

void
socksv5_write(struct selector_key *key) {
    struct state_machine * stm  = &ATTACHMENT(key)->stm;

    enum socksv5_global_state state = (enum socksv5_global_state) stm_handler_write(stm, key);

    if(state == ERROR || state == DONE) {
        socksv5_done(key);
    }
}

void
socksv5_block(struct selector_key *key) {
    struct state_machine * stm  = &ATTACHMENT(key)->stm;
    const enum socksv5_global_state state  = (enum socksv5_global_state)stm_handler_block(stm, key);

    if(state == ERROR || state == DONE) {
        socksv5_done(key);
    }
}

void
socksv5_timeout(struct selector_key * key) {
	struct state_machine * stm  = &ATTACHMENT(key)->stm;
	struct socksv5 * socksv5 = ATTACHMENT(key);

	time(&socksv5->last_update);
    printf("Timeout\n");
	enum socksv5_global_state state = (enum socksv5_global_state) stm_handler_timeout(stm, key);

	if(state == ERROR) {
		socksv5_done(key);
    }
}

