#ifndef PROTOS2022A_ORIGINCONNECTSTM
#define PROTOS2022A_ORIGINCONNECTSTM

#include "../headers/selector.h"
#include "../headers/socksv5_stm.h"
#include "../headers/request_stm.h"

unsigned connect_origin_init(const unsigned state, struct selector_key * key);

enum socksv5_global_state
connect_to_origin(struct selector_key *key, struct request_stm * req_stm);

enum socksv5_global_state
connect_through_ip(struct selector_key *key);

enum socksv5_global_state
connect_through_fqdn(struct selector_key * key);

void * connect_origin_thread(void * data);

unsigned connect_origin_block(struct selector_key *key);

enum socksv5_global_state
verify_connection(struct selector_key * key);

#endif
