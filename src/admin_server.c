// Aca se setea la estructura, el passive_accept, y active_handler
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>

#include "../headers/selector.h"
#include "../headers/admin_server.h"
#include "../headers/buffer.h"
#include "../headers/admin_stm.h"
#include "../headers/admin_request_parser.h"
#include "../headers/admin_auth_parser.h"


struct admin * new_admin(int fd);
static void destroy_admin(struct selector_key *key);

struct admin * first = NULL;

static const struct fd_handler admin_active_handler = {
        .handle_read = admin_read,
        .handle_write = admin_write,
};

void admin_passive_accept(struct selector_key *key) {

    struct sockaddr new_client_addr;
    struct admin * ad = NULL;
    socklen_t new_client_addr_len = sizeof(new_client_addr);

    int client_sock = accept(key->fd, &new_client_addr, &new_client_addr_len);
    if(client_sock == -1) {
        printf("Error aceptando la conexión!\n");
        goto finally;
    }

    if(selector_fd_set_nio(client_sock) == -1) {
        printf("Error seteando client_sock en no bloqueante\n");
        goto finally;
    }

    //a0 NULL

    ad = new_admin(key->fd);
    if(ad == NULL) {
        printf("ad dio null\n");
        goto finally;
    }

    if(selector_register(key->s, client_sock, &admin_active_handler, OP_READ, ad) != SELECTOR_SUCCESS) {
        printf("Error en el selector\n");
        goto finally;
    }

    return;

finally:
    if(client_sock != -1) {
        close(client_sock);
    }
    if(ad != NULL) {
        destroy_admin(key);
    }
}

struct admin * new_admin(int fd) {

    struct admin * admin = malloc(sizeof(*admin));

    if(admin == NULL) { goto finally; }
    memset(admin, 0x00, sizeof(*admin));

    if(first == NULL){
        first = admin;
        admin->next = NULL;
    }else{
        
        struct admin * aux = first;
        int i = 0;
        while(aux->next != NULL){
            aux=aux->next;
            i++;
        }
        aux->next = admin;
        admin->next = NULL;
        admin->list_index = i + 1;
    }
    //Se setean los atributos
    admin->admin_fd = fd;

    admin->stm.initial = ADMIN_AUTH;
    admin->stm.max_state = ADMIN_ERROR;
    admin->stm.states = admin_describe_states();
    admin->stm.current = NULL;
    stm_init(&admin->stm);

    buffer_init(&admin->read_buffer,  N(admin->raw_buff_a), admin->raw_buff_a);
    buffer_init(&admin->write_buffer, N(admin->raw_buff_b), admin->raw_buff_b);

    finally:
    return admin;
}



static void destroy_admin(struct selector_key *key){
    struct admin * ad = ADMIN_ATTACHMENT(key);

    if(selector_unregister_fd(key->s, key->fd) != SELECTOR_SUCCESS) {
        exit(EXIT_FAILURE);
    }
    if(key->fd != -1) {
        close(key->fd);
    }

    struct admin * aux = first;
    int i = 0;
    for(; i < (ad->list_index - 1); i++){
        aux = aux->next;
    }
    aux->next = ad->next;
    while(aux->next != NULL){
        i++;
        aux->list_index = i;
    }
    
    if(first == ad){
        first = NULL;
    }

    free_admin_auth_parser(&ad->admin_auth_stm.admin_auth_parser);
    free_parser(&ad->admin_req_stm.admin_request_parser);
    free(ad);
}

void free_all_admins(){
    struct admin * iterator = first;
    struct admin * aux = iterator;
    while(iterator != NULL){
        iterator = iterator->next;
        free_parser(&aux->admin_req_stm.admin_request_parser);
        free(aux);
        aux = iterator;
    }
}


void admin_read(struct selector_key *key) {
    struct state_machine *stm  = &ADMIN_ATTACHMENT(key)->stm;

    enum admin_state state = stm_handler_read(stm, key);

    if(state == ADMIN_ERROR || state == ADMIN_DONE) {
        destroy_admin(key);
    }
}


void admin_write(struct selector_key *key) {
    struct state_machine *stm  = &ADMIN_ATTACHMENT(key)->stm;

    enum admin_state state = stm_handler_write(stm, key);

    if(state == ADMIN_ERROR || state == ADMIN_DONE) {
        destroy_admin(key);
    }
}

