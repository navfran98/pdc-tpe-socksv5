#ifndef PROTOS2022A_SOCKSERVER
#define PROTOS2022A_SOCKSERVER

#include <stdint.h>
#include "./headers/buffer.h"

#define MAX_CONCURRENT_CONNECTIONS 500
#define N(x) (sizeof(x)/sizeof((x)[0]))


struct error_state {
	unsigned state;
	char * message;
	int code;
};

struct socks5 {
    int client_fd;
    int origin_fd;

    char * client_ip;
    uint16_t client_port;
    char * origin_ip;
    uint16_t origin_port;

    struct state_machine stm; // Gestor de máquinas de estado
    struct error_state err;
    
    struct users connected_user;

    time_t last_update; // Starts in 0. This connection will be cleaned when timeout reaches STATE_TIMEOUT


    uint8_t raw_buff_a[MAX_BUFF_SIZE], raw_buff_b[MAX_BUFF_SIZE];
    buffer read_buffer, write_buffer;
};

#define ATTACHMENT(key) ( (struct socks5*)(key)->data)

void
socksv5_passive_accept(struct selector_key *key);

void
socks5_read(struct selector_key *key);

void
socks5_write(struct selector_key *key);

void
socks5_timeout(struct selector_key *key);

static const fd_handler socksv5_active_handler = {
        .handle_read = socks5_read,
        .handle_write = socks5_write,
        .handle_timeout = socks5_timeout
};

#endif