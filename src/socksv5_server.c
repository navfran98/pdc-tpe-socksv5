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

static const unsigned max_pool = MAX_CONCURRENT_CONNECTIONS; //TODO: revisar este numero
static unsigned pool_size = 0;
static struct socksv5 * pool = 0; 

void socksv5_passive_accept(struct selector_key *key) {

    struct sockaddr_storage new_client_addr;
    struct socksv5 * socksv5 = NULL;
    socklen_t new_client_addr_len = sizeof(new_client_addr);

    const int client_sock = accept(key->fd, (struct sockaddr*)&new_client_addr, &new_client_addr_len);

    add_connection();

    if(client_sock == -1) {
        log_new_connection("Client failed to connect");
        goto finally;
    } else {
        log_new_connection("Client connected succesfully");
    }

    if(selector_fd_set_nio(client_sock) == -1) {
        goto finally;
    }
    
    socksv5 = new_socksv5(client_sock);
    if(socksv5 == NULL) {
        goto finally;
    }
    
    memcpy(&socksv5->client_ip, &new_client_addr, new_client_addr_len);

    if (get_concurrent_connections() == MAX_CONCURRENT_CONNECTIONS) {
        if(selector_set_interest_key(key, OP_NOOP) != SELECTOR_SUCCESS)
            goto finally;
    }

    if(selector_register(key->s, client_sock, &socksv5_active_handler, OP_WRITE, socksv5) != SELECTOR_SUCCESS) {
        remove_connection();
        goto finally;
    }

    return;

finally:
    if(client_sock != -1) {
        close(client_sock);
        log_new_connection("Client disconnected");
    }
    if(socksv5 != NULL) {
        socksv5_destroy(key->data);
    }
}

struct socksv5 * new_socksv5(int client_fd) {
     struct socksv5 * socksv5;

    if (pool == NULL) {
        socksv5 = malloc(sizeof(*socksv5));
    } else {
        socksv5 = pool;
        pool = pool->next;
        socksv5->next = 0;
    }

    if (socksv5 == NULL) {
        goto finally;
    }

    memset(socksv5, 0x00, sizeof(*socksv5));

    socksv5->origin_fd = -1;
    socksv5->client_fd = client_fd;

    socksv5->stm.initial   = GREETING_READ;
    socksv5->stm.max_state = ERROR_GLOBAL_STATE;
    socksv5->stm.states    = socksv5_describe_states();

    stm_init(&socksv5->stm);

    buffer_init(&socksv5->read_buffer,  N(socksv5->raw_buff_a), socksv5->raw_buff_a);
    buffer_init(&socksv5->write_buffer, N(socksv5->raw_buff_b), socksv5->raw_buff_b);

    socksv5->references = 1;
    finally:
    return socksv5;
}


static void
socksv5_destroy_(struct socksv5 *s) {
    free(s);
    remove_connection();
}


void
socksv5_destroy(struct socksv5 *s) {
    if(s == NULL) {
    } else if(s->references == 1) {
        if(s != NULL) {
            if(pool_size < max_pool) {
                s->next = pool;
                pool    = s;
                pool_size++;
            } else {
                socksv5_destroy_(s);
            }
        }
    } else {
        s->references -= 1;
    }
}

void
destroy_socksv5_pool(void) {
    struct socksv5 * next, * s;
    for(s = pool; s != NULL ; s = next) {
        next = s->next;
        socksv5_destroy(s);
    }
}

void
socksv5_done(struct selector_key *key) {
    const int fds[] = {
            ATTACHMENT(key)->client_fd,
            ATTACHMENT(key)->origin_fd,
    };

    if (ATTACHMENT(key)->origin_fd != -1) {
        remove_connection();
    }

    for(unsigned i = 0; i < N(fds); i++) {
        if(fds[i] != -1) {
            if(SELECTOR_SUCCESS != selector_unregister_fd(key->s, fds[i])) {
                abort();
            }
            close(fds[i]);
        }
    }
    log_new_connection("Client disconnected");
}

void
socksv5_read(struct selector_key *key) {
    struct state_machine *stm  = &ATTACHMENT(key)->stm;

    enum socksv5_global_state state = (enum socksv5_global_state) stm_handler_read(stm, key);

    if(state == ERROR_GLOBAL_STATE || state == DONE) {
    socksv5_done(key);
    }
}

void
socksv5_write(struct selector_key *key) {
    struct state_machine *stm  = &ATTACHMENT(key)->stm;

    enum socksv5_global_state state = (enum socksv5_global_state) stm_handler_write(stm, key);

    if(state == ERROR_GLOBAL_STATE || state == DONE) {
        socksv5_done(key);
    }
}

void
socksv5_block(struct selector_key *key) {
    struct state_machine *stm   = &ATTACHMENT(key)->stm;
    const enum socksv5_global_state state  = (enum socksv5_global_state)stm_handler_block(stm, key);

    if(state == ERROR_GLOBAL_STATE || state == DONE) {
        socksv5_done(key);
    }
}

void
socksv5_timeout(struct selector_key *key) {
	struct state_machine *stm  = &ATTACHMENT(key)->stm;
	struct socksv5 * socksv5 = ATTACHMENT(key);

	time(&socksv5->last_update);

	enum socksv5_global_state state = (enum socksv5_global_state) stm_handler_timeout(stm, key);

	if(state == ERROR_GLOBAL_STATE) {
		// TODO: habria que hacer un log sobre que hubo un error
		socksv5_destroy(socksv5);
	}
}

