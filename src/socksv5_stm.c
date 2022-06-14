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

#include "utils/selector.h"
#include "utils/parameters.h"
#include "socksv5_server.h"
#include "socksv5_stm.h"


/*
const struct state_definition global_states_definition[] = {
    {
        .state          = RESOLVE_ORIGIN,
        .on_write_ready = resolve_origin,
        .on_block_ready = resolve_origin_done,
    },
    {
        .state          = VERIFY_CONNECTION,
        .on_write_ready = verify_connection,
    },
    {
        .state = HELLO,
        .on_arrival     = hello_init,
        .on_read_ready  = hello_read,
        .on_write_ready = hello_write,
    },
  
    {
        .state = REQUEST,
        .on_arrival     = request_init,
        .on_read_ready  = request_read,
        .on_write_ready = request_write,
        
    },
    {
        .state = RESPONSE,
        .on_arrival       = response_init,
        .on_read_ready    = response_read,
        .on_write_ready   = response_write,
        //.on_departure     = response_close,
    },
    {
        .state = DONE,
    },
    {
        .state = ERROR_GLOBAL_STATE,
    },
};
*/

const struct state_definition global_states_definition[] = {
    {
        .state = GREETING_READ,
    },
    {
        .state = GREETING_WRITE,
    },
    {
        .state = AUTH_READ,
    },
    {
        .state = AUTH_WRITE,
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


