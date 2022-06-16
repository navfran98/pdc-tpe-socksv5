#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "../headers/selector.h"
#include "../headers/request_stm.h"
#include "../headers/socksv5_server.h"
#include "../headers/socksv5_stm.h"
#include "../headers/logger.h"

unsigned
connect_origin_init(const unsigned state, struct selector_key *key) {
	struct socksv5 * socksv5 = ATTACHMENT(key);
	struct request_stm * req_stm = &ATTACHMENT(key)->client.request;

	unsigned ret_state = state;

    if(socksv5->origin_fd < 0) {
		ret_state = connect_to_origin(key, req_stm);
        //TODO: revisar esta parte de abajo
		if(ret_state == REQUEST_WRITE) {
            if(selector_set_interest(key->s, socksv5->client_fd, OP_WRITE) != SELECTOR_SUCCESS) {
                return ERROR_GLOBAL_STATE;
            }
		}
    }
    return ret_state;
}

static enum socksv5_global_state
connect_to_origin(struct selector_key *key, struct request_stm * req_stm) {
    if(req_stm->request_parser.atyp == REQUEST_THROUGH_IPV4 || req_stm->request_parser.atyp == REQUEST_THROUGH_IPV6)
        return connect_through_ip(key, req_stm);
    else
        return connect_through_fqdn(key, req_stm);
        //TODO: hay que corregir que si admita fqdn en el parser de request
}

static enum socksv5_global_state
connect_through_ip(struct selector_key *key, struct request_stm * req_stm){
    struct socksv5 * socksv5 = ATTACHMENT(key);
    struct request_stm * req_stm = &ATTACHMENT(key)->client.request;

    if(req_stm->request_parser.atyp == REQUEST_THROUGH_IPV4)
        socksv5->origin_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    else
        socksv5->origin_fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);

    if(socksv5->origin_fd < 0) {
        goto finally;
    }

    if(selector_fd_set_nio(socksv5->origin_fd) < 0) {
        goto finally;
    }

    if(req_stm->request_parser.atyp == REQUEST_THROUGH_IPV4){
        int addr_in_size = sizeof(struct sockaddr_in);
        memset(&req_stm->origin_addr_ipv4, 0, addr_in_size);
        req_stm->origin_addr_ipv4.sin_port = htons(req_stm->request_parser.port);
        req_stm->origin_addr_ipv4.sin_family = AF_INET;
        memcpy(&req_stm->origin_addr_ipv4.sin_addr, req_stm->request_parser.port, req_stm->request_parser.addr_len);

        if(connect(socksv5->origin_fd, (struct sockaddr*)&req_stm->origin_addr_ipv4, sizeof(req_stm->origin_addr_ipv4)) < 0) {
            if(errno == EINPROGRESS) {
                if (selector_register(key->s, socksv5->origin_fd, &socksv5_active_handler, OP_WRITE, key->data) != SELECTOR_SUCCESS) {
                    req_stm->request_parser.reply = GENERAL_SOCKS_SERVER_FAILURE;
                    goto finally;
                }
                return ORIGIN_CONNECT;
            } else {
                //TODO: revisar con que cargar el reply
                req_stm->request_parser.reply = GENERAL_SOCKS_SERVER_FAILURE;
                goto finally;
            }
        }
        return ORIGIN_CONNECT;
    }else{
        int addr_in_size = sizeof(struct sockaddr_in);
        memset(&req_stm->origin_addr_ipv6, 0, addr_in_size);
        req_stm->origin_addr_ipv6.sin6_port = htons(req_stm->request_parser.port);
        req_stm->origin_addr_ipv6.sin6_family = AF_INET6;
        memcpy(&req_stm->origin_addr_ipv6.sin6_addr, req_stm->request_parser.port, req_stm->request_parser.addr_len);

        if(connect(socksv5->origin_fd, (struct sockaddr*)&req_stm->origin_addr_ipv6, sizeof(req_stm->origin_addr_ipv6)) < 0) {
            if(errno == EINPROGRESS) {
                if (selector_register(key->s, socksv5->origin_fd, &socksv5_active_handler, OP_WRITE, key->data) != SELECTOR_SUCCESS) {
                    req_stm->request_parser.reply = GENERAL_SOCKS_SERVER_FAILURE;
                    goto finally;
                }
                return ORIGIN_CONNECT;
            } else {
                //TODO: revisar con que cargar el reply
                req_stm->request_parser.reply = GENERAL_SOCKS_SERVER_FAILURE;
                goto finally;
            }
        }
        return ORIGIN_CONNECT;
    }
finally:
    ;
    //TODO: habria que ver de cerrar los sockets.
    return REQUEST_WRITE;
}

static enum socksv5_global_state
connect_through_fqdn(struct selector_key * key, struct request_stm * req_stm){
    return DONE;
}