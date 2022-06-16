#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctype.h>
#include <memory.h>
#include <netdb.h>

#include "../headers/selector.h"
#include "../headers/parameters.h"
#include "../headers/socksv5_server.h"
#include "../headers/socksv5_stm.h"

const struct state_definition global_states_definition[] = {
    {
        .state = GREETING_READ,
        .on_arrival     = greeting_init,
        .on_read_ready  = greeting_read,
        
    },
    {
        .state = GREETING_WRITE,
        .on_write_ready = greeting_write,
    },
    {
        .state = AUTH_READ,
        .on_arrival     = auth_init,
        .on_read_ready  = auth_read,
    },
    {
        .state = AUTH_WRITE,
        .on_write_ready = auth_write,
    },
     {
        .state = REQUEST_READ,
    },
     {
        .state = REQUEST_WRITE,
    },
    {
        .state = ORIGIN_CONNECT,
    },
     {
        .state = RESPONSE_READ,
    },
    {
        .state = RESPONSE_WRITE,
    },
    {
        .state = DONE,
    },
    {
        .state = ERROR_GLOBAL_STATE,
    },
};

const struct state_definition * socksv5_describe_states(void) {
    return global_states_definition;
}


