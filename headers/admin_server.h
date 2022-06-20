#ifndef PROTOS2022A_ADMIN_SERVER
#define PROTOS2022A_ADMIN_SERVER

#include "../headers/selector.h"
#include "../headers/stm.h"
#include "../headers/admin_request.h"
#include "../headers/admin_auth.h"

#define ADMIN_ATTACHMENT(key) ((struct admin *)(key)->data)
#define N(x) (sizeof(x)/sizeof((x)[0]))

struct admin {
    int admin_fd;

    uint8_t raw_buff_a[MAX_BUFF_SIZE], raw_buff_b[MAX_BUFF_SIZE];
    buffer read_buffer, write_buffer;

    struct admin_auth_stm admin_auth_stm;  
    struct admin_request_stm admin_req_stm;
    struct state_machine stm; 
};

void admin_passive_accept(struct selector_key * key);

void admin_read(struct selector_key * key);

void admin_write(struct selector_key * key);

const struct state_definition * admin_describe_states(void);

#endif
