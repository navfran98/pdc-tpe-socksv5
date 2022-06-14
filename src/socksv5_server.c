
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/socket.h>
#include <netdb.h>
#include "metrics.h"
#include "utils/selector.h"
#include "socksv5_server.h"

static const unsigned max_pool = MAX_CONCURRENT_CONNECTIONS; //TODO: revisar este numero
static unsigned pool_size = 0;
static struct pop3 * pool = 0; 

void sockv5_passive_accept(struct selector_key *key) {

    struct sockaddr_storage new_client_addr;
    struct sockv5 * sockv5 = NULL;
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
    
    sockv5 = new_sockv5(client_sock);
    if(sockv5 == NULL) {
        goto finally;
    }
    
    memcpy(&sockv5->client_addr, &new_client_addr, new_client_addr_len);

    if (get_concurrent_connections() == MAX_CONCURRENT_CONNECTIONS) {
        if(selector_set_interest_key(key, OP_NOOP) != SELECTOR_SUCCESS)
            goto finally;
    }

    if(selector_register(key->s, client_sock, &socksv5_active_handler, OP_WRITE, p3) != SELECTOR_SUCCESS) {
        remove_connection();
        goto finally;
    }

    return;

finally:
    if(client_sock != -1) {
        close(client_sock);
        log_new_connection("Client disconnected");
    }
    if(p3 != NULL) {
        sockv5_destroy(key->data);
    }
}

static struct sockv5 * new_sockv5(int client_fd) {
     struct sockv5 * sockv5;

    if (pool == NULL) {
        sockv5 = malloc(sizeof(*sockv5));
    } else {
        sockv5       = pool;
        pool       = pool->next;
        sockv5->next = 0;
    }

    if (sockv5 == NULL) {
        goto finally;
    }

    memset(sockv5, 0x00, sizeof(*sockv5));

    sockv5->origin_fd         = -1;
    sockv5->client_fd         = client_fd;

    sockv5->stm    .initial   = GREETING_READ;
    sockv5->stm    .max_state = ERROR;
    sockv5->stm    .states    = sockv5_describe_states();

    stm_init(&sockv5->stm);

    buffer_init(&sockv5->read_buffer,  N(sockv5->raw_buff_a), sockv5->raw_buff_a);
    buffer_init(&sockv5->write_buffer, N(sockv5->raw_buff_b), sockv5->raw_buff_b);

    sockv5->references = 1;

    finally:
    return sockv5;
}


static void
sockv5_destroy_(struct sockv5 *s) {
    if(s->origin_resolution != NULL) {
        freeaddrinfo(s->origin_resolution);
        s->origin_resolution = 0;
    }
    sockv5_session_destroy(&s->session);
    free(s);
    remove_connection();
}


void
sockv5_destroy(struct sockv5 *s) {
    if(s == NULL) {
    } else if(s->references == 1) {
        if(s != NULL) {
            if(pool_size < max_pool) {
                s->next = pool;
                pool    = s;
                pool_size++;
            } else {
                sockv5_destroy_(s);
            }
        }
    } else {
        s->references -= 1;
    }
}

void
destroy_sockv5_pool(void) {
    struct sockv5 * next, * s;
    for(s = pool; s != NULL ; s = next) {
        next = s->next;
        sockv5_destroy(s);
    }
}

void
sockv5_done(struct selector_key *key) {
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
    sockv5_session_destroy(&ATTACHMENT(key)->session);
}

void
sockv5_read(struct selector_key *key) {
    struct state_machine *stm  = &ATTACHMENT(key)->stm;

    enum sockv5_global_state state = (enum sockv5_global_state) stm_handler_read(stm, key);

     if(state == ERROR || state == DONE) {
        sockv5_done(key);
     }
}

void
sockv5_write(struct selector_key *key) {
    struct state_machine *stm  = &ATTACHMENT(key)->stm;

    enum sockv5_global_state state = (enum sockv5_global_state) stm_handler_write(stm, key);

    if(state == ERROR || state == DONE) {
        sockv5_done(key);
    }
}

void
sockv5_block(struct selector_key *key) {
    struct state_machine *stm   = &ATTACHMENT(key)->stm;
    const enum sockv5_global_state state  = (enum sockv5_global_state)stm_handler_block(stm, key);

    if(state == ERROR || state == DONE) {
        sockv5_done(key);
    }
}

void
sockv5_close(struct selector_key *key) {
    sockv5_destroy(ATTACHMENT(key));
}
