#ifndef PROTOS2022A_COPYSTM
#define PROTOS2022A_COPYSTM

#include "selector.h"
#include "buffer.h"

struct copy_stm {
    buffer * rb;
    buffer * wb;

    bool reading_client; 
    bool writing_client; 
    bool reading_origin; 
    bool writing_origin; 
};


unsigned
copy_init(const unsigned state, struct selector_key *key);

unsigned
copy_read(struct selector_key * key);

unsigned
copy_write(struct selector_key * key);


#endif
