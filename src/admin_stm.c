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
#include "../headers/admin_server.h"
#include "../headers/admin_stm.h"

const struct state_definition admin_states_definition[] = {
    {
        .state          = ADMIN_AUTH,
        .on_arrival     = admin_auth_init,
        .on_read_ready  = admin_auth_read,
        .on_write_ready = admin_auth_write
    },{
        .state          = ADMIN_REQUEST,
        .on_arrival     = admin_request_init,
        .on_read_ready  = admin_request_read,
        .on_write_ready = admin_request_write
    },
    {
        .state          = ADMIN_DONE
    },{
        .state          = ADMIN_ERROR
    },
};

const struct state_definition * admin_describe_states(void) {
    return admin_states_definition;
}
