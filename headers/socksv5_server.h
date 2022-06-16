#ifndef PROTOS2022A_SOCKSERVER
#define PROTOS2022A_SOCKSERVER

#include <stdint.h>
#include "../headers/stm.h"
#include "../headers/buffer.h"
#include "../headers/auth_stm.h"
#include "../headers/greeting_stm.h"
#include "../headers/parameters.h"

#define MAX_CONCURRENT_CONNECTIONS 500
#define N(x) (sizeof(x)/sizeof((x)[0]))


struct error_state {
	unsigned state;
	char * message;
	int code;
};


typedef struct socksv5 {
    int client_fd;
    int origin_fd;

    char * client_ip;
    uint16_t client_port;
    char * origin_ip;
    uint16_t origin_port;

    struct user connected_user;


    union
    {
        struct greeting_stm greeting;
        struct auth_stm auth;
        struct request_stm request;
    } client;

    struct state_machine stm; // Gestor de máquinas de estado
    struct error_state err;
    int references;

    time_t last_update; // Starts in 0. This connection will be cleaned when timeout reaches STATE_TIMEOUT
    struct socksv5 * next;

    uint8_t raw_buff_a[MAX_BUFF_SIZE], raw_buff_b[MAX_BUFF_SIZE];
    buffer read_buffer, write_buffer;
} socksv5;

#define ATTACHMENT(key) ( (struct socksv5*)(key)->data)

struct socksv5 * 
new_socksv5(int client_fd);

void
socksv5_passive_accept(struct selector_key *key);

void
socksv5_read(struct selector_key *key);

void
socksv5_write(struct selector_key *key);

void
socksv5_timeout(struct selector_key *key);

void
destroy_socksv5_pool(void);

void
socksv5_destroy(struct socksv5 *s);

void
socksv5_done(struct selector_key *key);

void
socksv5_close(struct selector_key *key);

void
socksv5_block(struct selector_key *key);

static const fd_handler socksv5_active_handler = {
        .handle_read = socksv5_read,
        .handle_write = socksv5_write,
        .handle_timeout = socksv5_timeout 
};


#endif
