#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "../headers/selector.h"
#include "../headers/request_stm.h"
#include "../headers/socksv5_server.h"
#include "../headers/socksv5_stm.h"
#include "../headers/logger.h"

unsigned
connect_origin_init(const unsigned state, struct selector_key *key) {

    printf("-----------------------\nORIGIN CONNECT\n\n");
	struct socksv5 * socksv5 = ATTACHMENT(key);
	struct request_stm * req_stm = &ATTACHMENT(key)->client.request;

	unsigned ret_state = state;

    if(socksv5->origin_fd < 0) {
		ret_state = connect_to_origin(key, req_stm);
        printf("Vuelvo con estado %d\n", ret_state);
	}
    return ret_state;
}

enum socksv5_global_state
connect_to_origin(struct selector_key * key, struct request_stm * req_stm) {
    if(req_stm->request_parser.atyp == REQUEST_THROUGH_IPV4 || req_stm->request_parser.atyp == REQUEST_THROUGH_IPV6){
        printf("Me voy a conectar mediante una IP\n");
        return connect_through_ip(key);
    }
    else{
        printf("Me voy a conectar mediante un FQDN\n");
        return connect_through_fqdn(key);
    }
}

enum socksv5_global_state
connect_through_ip(struct selector_key *key){
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
        //int addr_in_size = sizeof(struct sockaddr_in);
        //memset(&req_stm->origin_addr_ipv4, 0, addr_in_size);
        //req_stm->origin_addr_ipv4.sin_port = htons(req_stm->request_parser.port);
        //req_stm->origin_addr_ipv4.sin_family = AF_INET;
        //memcpy(&req_stm->origin_addr_ipv4.sin_addr, &req_stm->request_parser.addr, req_stm->request_parser.addr_len);
        
        if(connect(socksv5->origin_fd, (struct sockaddr*)&req_stm->origin_addr_ipv4, sizeof(req_stm->origin_addr_ipv4)) < 0) {
            if(errno == EINPROGRESS) {
                /*if(selector_set_interest_key(key, OP_NOOP) != SELECTOR_SUCCESS){
                    return ERROR_GLOBAL_STATE;
                }*/
                if (selector_register(key->s, socksv5->origin_fd, &socksv5_active_handler, OP_WRITE, key->data) != SELECTOR_SUCCESS) {
                    return ERROR_GLOBAL_STATE;
                }
            } else {
                req_stm->request_parser.reply = HOST_UNREACHABLE;
                goto finally;
            }
        }
        return ORIGIN_CONNECT;
    }else{
        //int addr_in_size = sizeof(struct sockaddr_in);
        // memset(&req_stm->origin_addr_ipv6, 0, addr_in_size);
        // req_stm->origin_addr_ipv6.sin6_port = htons(req_stm->request_parser.port);
        // req_stm->origin_addr_ipv6.sin6_family = AF_INET6;
        // memcpy(&req_stm->origin_addr_ipv6.sin6_addr, &req_stm->request_parser.port, req_stm->request_parser.addr_len);

        if(connect(socksv5->origin_fd, (struct sockaddr*)&req_stm->origin_addr_ipv6, sizeof(req_stm->origin_addr_ipv6)) < 0) {
            if(errno == EINPROGRESS) {
                if (selector_register(key->s, socksv5->origin_fd, &socksv5_active_handler, OP_WRITE, key->data) != SELECTOR_SUCCESS) {
                    return ERROR_GLOBAL_STATE;
                }
            } else {
                req_stm->request_parser.reply = HOST_UNREACHABLE;
                goto finally;
            }
        }
        return ORIGIN_CONNECT;
    }
finally:
    ;
    printf("Llegué a finally\n");
    if(selector_set_interest_key(key, OP_WRITE) != SELECTOR_SUCCESS){
        return ERROR_GLOBAL_STATE;
    }
    return REQUEST_WRITE;
}

enum socksv5_global_state
connect_through_fqdn(struct selector_key * key){
    pthread_t tid;
    struct selector_key* k = malloc(sizeof(*key));
    enum socksv5_global_state ret = ORIGIN_CONNECT;

    if(k == NULL) {
        ret = ERROR_GLOBAL_STATE;
    } else {
        memcpy(k, key, sizeof(*k));
        if((pthread_create(&tid, 0, connect_origin_thread, k)) == -1) {
            ret = ERROR_GLOBAL_STATE;
        }else{
            selector_set_interest_key(key, OP_NOOP);
        }
    }
    return ret;
}

void * connect_origin_thread(void *data) {
    struct selector_key * key = (struct selector_key *) data;
    struct socksv5 * socksv5 = ATTACHMENT(key);
    struct request_stm * req_stm = &ATTACHMENT(key)->client.request;

    pthread_detach(pthread_self());
    socksv5->origin_resolution = 0;
    struct addrinfo hints = {
            .ai_family    = AF_UNSPEC, 
            .ai_socktype  = SOCK_STREAM,  
            .ai_flags     = AI_PASSIVE,  
            .ai_protocol  = 0,           
            .ai_canonname = NULL,
            .ai_addr      = NULL,
            .ai_next      = NULL,
    };

    char buff[7];
    snprintf(buff, sizeof(buff), "%lu",req_stm->request_parser.port);

    if (0 != getaddrinfo((char*)req_stm->request_parser.addr, buff, &hints, &socksv5->origin_resolution)){
        fprintf(stderr, "Domain name resolution error\n"); 
    }
    selector_notify_block(key->s, key->fd);
    free(data);
    
    return 0;
}


 unsigned connect_origin_block(struct selector_key *key) {
    printf("Entro en el block\n");
    struct socksv5 * socksv5 =  ATTACHMENT(key);
    struct request_stm * req_stm = &ATTACHMENT(key)->client.request;

    enum socksv5_global_state ret = ORIGIN_CONNECT;

    if(socksv5->origin_resolution == 0) {
        goto finally;
    }
    socksv5->origin_resolution_current = socksv5->origin_resolution;
    
    while(socksv5->origin_resolution_current != NULL){
        if(socksv5->origin_resolution_current->ai_family == AF_INET){
            req_stm->request_parser.atyp = REQUEST_THROUGH_IPV4;
            memcpy(&req_stm->origin_addr_ipv4, socksv5->origin_resolution_current->ai_addr, socksv5->origin_resolution_current->ai_addrlen);
        }else if(socksv5->origin_resolution_current->ai_family == AF_INET6){
            req_stm->request_parser.atyp = REQUEST_THROUGH_IPV6;
            memcpy(&req_stm->origin_addr_ipv6, socksv5->origin_resolution_current->ai_addr, socksv5->origin_resolution_current->ai_addrlen);
        }
        //req_stm->request_parser.addr_len = socksv5->origin_resolution_current->ai_addrlen;
        //memcpy(&req_stm->origin_addr_ipv4, socksv5->origin_resolution_current->ai_addr, socksv5->origin_resolution_current->ai_addrlen);
        
        if((ret=connect_through_ip(key)) == ORIGIN_CONNECT ){
            printf("Me conecté por FQDN\n");
            freeaddrinfo(socksv5->origin_resolution);
            socksv5->origin_resolution = 0;
            return ret;
        }
        if(ret == ERROR_GLOBAL_STATE){
            return ERROR_GLOBAL_STATE;
        }
        socksv5->origin_resolution_current = socksv5->origin_resolution_current->ai_next;
    }
    freeaddrinfo(socksv5->origin_resolution);
    socksv5->origin_resolution = 0;
finally:
    if (selector_set_interest(key->s, socksv5->client_fd, OP_WRITE) != SELECTOR_SUCCESS) {
        printf("ME MUERO\n");
        return ERROR_GLOBAL_STATE;
    }
    printf("ME VOY AL WRITE\n");
    req_stm->request_parser.reply = HOST_UNREACHABLE;
    return REQUEST_WRITE;
 }


enum socksv5_global_state
verify_connection(struct selector_key * key) {
    printf("---------------\nVERIFY CONNECTION\n\n");
    printf("%d", key->fd);
    struct socksv5 * socksv5 = ATTACHMENT(key);

    struct request_stm * req_stm = &ATTACHMENT(key)->client.request;
    unsigned int error_type = 1, error_len = sizeof(error_type);
    if( getsockopt(socksv5->origin_fd, SOL_SOCKET, SO_ERROR, &error_type, &error_len)!= 0){
        printf("VIVA GUEMES\n");
        return ERROR_GLOBAL_STATE;
    }
    if ( error_type != 0) {
        printf("PINCHO EL ORIGIN CONNECT :C\n");
        if(error_type == ECONNREFUSED) {
            req_stm->request_parser.reply = CONNECTION_REFUSED;
        } else if(error_type == ENETUNREACH) {
            req_stm->request_parser.reply = NETWORK_UNREACHABLE;
        } else if(error_type == EHOSTUNREACH) {
            req_stm->request_parser.reply = HOST_UNREACHABLE;
        } else {
            req_stm->request_parser.reply = GENERAL_SOCKS_SERVER_FAILURE;
        }
        goto finally;
    }else if (error_type == 0) {
            printf("Supuestamente logré conectarme\n");
            if(selector_set_interest_key(key, OP_NOOP) != SELECTOR_SUCCESS){
                return ERROR_GLOBAL_STATE;
            }
            if(selector_set_interest(key->s, socksv5->client_fd, OP_WRITE) != SELECTOR_SUCCESS){
                return ERROR_GLOBAL_STATE;
            }
            req_stm->request_parser.reply = SUCCEDED;
            return REQUEST_WRITE;
    } else {
        printf("Aborto clandestino\n");
        abort();
    }
finally:
    if (selector_set_interest_key(key, OP_NOOP) != SELECTOR_SUCCESS) {
        return ERROR_GLOBAL_STATE;
    }
    if(selector_set_interest(key->s, socksv5->client_fd, OP_WRITE) != SELECTOR_SUCCESS) {
        return ERROR_GLOBAL_STATE;
    }
    return REQUEST_WRITE;
}