#include <unistd.h>
#include <sys/socket.h>  // socket
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <arpa/inet.h>
#include "../headers/selector.h"
#include "../headers/request_stm.h"
#include "../headers/socksv5_server.h"
#include "../headers/socksv5_stm.h"
#include "../headers/logger.h"

#define ENOUGH_SPACE_TO_CONNECTION_LOG 200



unsigned
request_read_init(const unsigned state, struct selector_key *key) {
	struct request_stm *reqstm = &ATTACHMENT(key)->client.request;

    reqstm->rb = &(ATTACHMENT(key)->read_buffer);
    reqstm->wb = &(ATTACHMENT(key)->write_buffer);


    request_parser_init(&reqstm->request_parser);

    reqstm->origin_addrinfo = calloc(1, sizeof(struct addrinfo));
    if(reqstm->origin_addrinfo == NULL) {
        goto finally;
    }
    return state;

finally:
    return ERROR_GLOBAL_STATE;
}


unsigned
request_read(struct selector_key *key) {
    struct request_stm *reqstm = &ATTACHMENT(key)->client.request;

    size_t nbytes;
    uint8_t *where_to_write = buffer_write_ptr(&reqstm->rb, &nbytes);
    ssize_t ret = recv(key->fd, where_to_write, nbytes, 0);  // Non blocking !

    enum socks5_global_state state_toret = REQUEST_READ; // current state
    if(ret > 0) {
        buffer_write_adv(&reqstm->rb, ret);
        enum request_state state = consume_request_buffer(&reqstm->rb, &reqstm->request_parser);
        if(state == request_done) {
            // Client should wait until proxy connects to origin or tell user that there has been an error. 
            if(selector_set_interest_key(key, OP_NOOP) != SELECTOR_SUCCESS) {
                goto finally;
            }
			state_toret = ORIGIN_CONNECT; //me tengo que conectar al origen.

        }else if(state == request_error) {
            if(selector_set_interest_key(key, OP_WRITE) != SELECTOR_SUCCESS) {
                goto finally;
            }
            state_toret = REQUEST_WRITE; //le tengo que escribir que algo 
        }

    } else {
        goto finally;
    }

    return state_toret;

finally:
    return ERROR_GLOBAL_STATE;
}


unsigned
request_write(struct selector_key *key) {
    struct socksv5 *s5 = ATTACHMENT(key);
    struct request_stm *reqstm = &ATTACHMENT(key)->client.request;


    // Dejamos la respuesta para ser enviada en el buffer "wb"
    // Lo hago acá porque antes tenia que resolver con el origin, y eso puede cambiar mi respuesta.
    request_marshall(&reqstm->wb, &reqstm->request_parser);

    size_t nbytes;
    uint8_t *where_to_read = buffer_read_ptr(&reqstm->wb, &nbytes);
    ssize_t ret = send(key->fd, where_to_read, nbytes, MSG_NOSIGNAL);

    if(ret > 0) {
        buffer_read_adv(&reqstm->wb, nbytes);
        if(!buffer_can_read(&reqstm->wb)) {
            if(reqstm->request_parser.reply == SUCCEDED) {

                if(selector_set_interest_key(key, OP_READ) != SELECTOR_SUCCESS) {
                    // Ya le enviamos una Response positiva... pero hay error aca. Lastima. Vamos a cerrar la conexion
                    goto finally;
                }

                if(selector_set_interest(key->s, s5->origin_fd, OP_READ) != SELECTOR_SUCCESS) {
                    // Ya le enviamos una Response positiva... pero hay error aca. Lastima. Vamos a cerrar la conexion
                    goto finally;
                }

                return COPY; //este estado es interesante... 

            } else {
                // Se terminó de mandar el Response, pero era un Response de error. Cerramos la conexion
                return DONE;
            }

        } else {
            // Exit here, and keep waiting for future calls to this function to end reading buffer
            return REQUEST_WRITE;
        }
    } else {
        goto finally;
    }

finally:
    // No hace falta llamar a free_everything(), ya que el cambio de estado se encarga de hacerlo
    return ERROR_GLOBAL_STATE;
}
