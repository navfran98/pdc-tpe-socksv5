#ifndef PROTOS2022A_GREETINGSTM
#define PROTOS2022A_GREETINGSTM

#include "selector.h"
#include "../src/parsers/greeting_parser.h"



struct greeting_stm {
    struct greeting_parser greeting_parser;

    buffer * rb; 
    buffer * wb; 

    int method_selected;  
};


unsigned
greeting_init(const unsigned state, struct selector_key *key);


unsigned
greeting_read(struct selector_key *key);


unsigned
greeting_write(struct selector_key *key);


#endif