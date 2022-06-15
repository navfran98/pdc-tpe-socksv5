#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <sys/socket.h>
#include <string.h>
#include "../../headers/greeting_stm.h"
#include "../../headers/greeting_parser.h"
#include "../../headers/socksv5_server.h"
#include "../../headers/socksv5_stm.h"
#include "../../headers/logger.h"



static int select_method(uint8_t *methods, uint8_t nmethods);


unsigned
greeting_init(const unsigned state, struct selector_key *key) {
	struct greeting_stm * gstm = &ATTACHMENT(key)->client.greeting;

    gstm->rb = &(ATTACHMENT(key)->read_buffer);
    gstm->wb = &(ATTACHMENT(key)->write_buffer);


    gstm->method_selected = -1;  

    greeting_parser_init(&gstm->greeting_parser);

    return state;   //TODO: cambiar funcion a que no retorne state, directamente esta función no lo cambia.. 
}


unsigned
greeting_read(struct selector_key *key) {
    struct greeting_stm * gstm = &ATTACHMENT(key)->client.greeting;

	size_t nbytes;
    uint8_t *where_to_write = buffer_write_ptr(&gstm->rb, &nbytes);
    ssize_t ret = recv(key->fd, where_to_write, nbytes, 0);  // Non blocking !

    uint8_t state_toret = GREETING_READ; // current state

    if(ret > 0) {

        buffer_write_adv(&gstm->rb, ret);
        enum greeting_state state = consume_greeting_buffer(&gstm->rb, &gstm->greeting_parser);
        if(state == greeting_done || state == greeting_bad_syntax || state == greeting_unsupported_version) {

            gstm->method_selected = select_method(gstm->greeting_parser.methods, gstm->greeting_parser.methods_remaining);

            if(selector_set_interest_key(key, OP_WRITE) != SELECTOR_SUCCESS) {
                goto finally;
            }
            // Dejamos la respuesta del greeting en el buffer "wb" para enviarlo en el próximo estado
            greeting_marshall(&gstm->wb, gstm->method_selected);

            state_toret = GREETING_WRITE;

        } 
    } else {
        
        goto finally;
    }
    return state_toret;

finally:

    //hubo un error en el selector o en el recv... 
    return ERROR_GLOBAL_STATE;
}


static int
select_method(uint8_t *methods, uint8_t nmethods) {
    int ret = NO_ACCEPTABLE_METHODS;

    if(methods != NULL) {
        for(uint8_t i = 0; i < nmethods; i++) {
            if(methods[i] == USERNAME_PASSWORD_AUTH) {
                return methods[i];  // Si el cliente esta dispuesto a usar usr/pass, entonces vamos por ahi
            } else if(methods[i] == NO_AUTHENTICATION_REQUIRED) {
                ret = methods[i];  // Si el cliente quiere usar NO_AUTH_REQUIRED, esta bien. Esperamos a ver si hay una mejor opción
            }
        }
    }
    return ret;
}



unsigned
greeting_write(struct selector_key *key) {
    struct greeting_stm *gstm = &ATTACHMENT(key)->client.greeting;
    struct socks5 *s5 = ATTACHMENT(key);

    size_t nbytes;
    uint8_t *where_to_read = buffer_read_ptr(&gstm->wb, &nbytes);

    ssize_t ret = send(key->fd, where_to_read, nbytes, MSG_NOSIGNAL);

    uint8_t state_toret = GREETING_WRITE; // current state
    if(ret > 0) {
        buffer_read_adv(&gstm->wb, nbytes);
        if(!buffer_can_read(&gstm->wb)) {

            if(selector_set_interest_key(key, OP_READ) != SELECTOR_SUCCESS) {
                goto finally;
            }

            if(gstm->method_selected == NO_AUTHENTICATION_REQUIRED) {
                // Ya estamos listos para leer el Request
                state_toret = REQUEST_READ;
            } else if(gstm->method_selected == USERNAME_PASSWORD_AUTH) {
                // Hay que leer ahora el user:pass
                state_toret = AUTH_READ;
            } else {
                goto finally;
            }

        }
    } else {
        goto finally;
    }

    return state_toret;

finally:
    return ERROR_GLOBAL_STATE;
}
