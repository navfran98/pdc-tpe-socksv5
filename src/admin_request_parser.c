// Aca se parsea la ida y vuelta de requests
#include "../headers/metrics.h"
#include "../headers/parameters.h"
#include "../headers/buffer.h"
#include "../headers/admin_request.h"
#include "../headers/admin_auth_parser.h"
#include "../headers/admin_request_parser.h"

#include <stdlib.h>
#include <stdio.h>   // used by print_current_hello_parser()
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#define CHUNK_SIZE 50

void load_metrics(uint8_t * answer, char * aux);
void load_users(uint8_t * answer, char ** aux);
int create_user(struct req_parser * pars);
int change_buffer_size(char * new_buff_size);

void add_string_space(char * str, size_t space){
    size_t size = strlen(str);
    str = realloc(str,sizeof(*str)*size + size);
}

void 
admin_req_parser_init(struct req_parser * pars) {
   pars->state = req_reading_cmd;
   pars->user = NULL;
   pars->password = NULL;
   pars->new_buff_size = NULL;
   pars->index = 0; 
}

enum admin_req_state
admin_consume_req_buffer(buffer * buff, struct req_parser * pars) {
     enum admin_req_state state = pars->state;

    while(buffer_can_read(buff)){
        const uint8_t c = buffer_read(buff);
        state = admin_req_parser_feed(c, pars);
        if(state == req_done) {
           break;
        }
    }
    return state;
}

enum admin_req_state
admin_req_parser_feed(const uint8_t c, struct req_parser * pars) {
    switch(pars->state) {
        case req_reading_cmd:
            if(c == '1') {
                pars->state = req_reading_metrics_cmd;
            }else if(c == '2'){
                pars->state = req_reading_add_user_cmd;
            } else if(c == '3'){
                pars->state = req_reading_list_user_cmd;
            } else if(c == '4'){
                pars->state = req_reading_change_buff_size_cmd;
            }else{
                pars->state = req_invalid_cmd;
            }
            pars->cmd_selected = c;
            break;

        case req_reading_add_user_cmd:
            if(c == ' ') {
                //hacer malloc para USER_LEN
                pars->user = malloc(USER_LEN * sizeof(char));
                pars->state = req_reading_user;
            } else {
                pars->state = req_invalid_cmd;
            }
            break;
       
        case req_reading_user:
            //si me llega un ' ' me voy a la contra
            if(c == ' '){
                //hago malloc para pass usando PASS_LEN
                pars->password = malloc(PASS_LEN * sizeof(char));
                pars->user[pars->index] = '\0';
                pars->index = 0;
                pars->state = req_reading_pass;
            }else{
                if(c != '\n'){
                    if(pars->index == USER_LEN){
                        add_string_space(pars->user, CHUNK_SIZE);
                    }
                    pars->user[pars->index++] = c;
                }else{
                    pars->state = req_invalid_cmd;
                }
            }
        
            break;

        case req_reading_pass:
            if(c == ' '){
               pars->state = req_invalid_cmd;
            }else if(c == '\n'){
               pars->password[pars->index] = '\0';
               pars->state = req_done;
               pars->index = 0;
            }else{
                if(pars->index == PASS_LEN){
                    add_string_space(pars->password, CHUNK_SIZE);
                }
                pars->password[pars->index++] = c;
            }
           break;

        case req_reading_change_buff_size_cmd:
            if(c == ' '){
               //hago malloc para el buffsize usando NEW_BUFF_LEN
               pars->new_buff_size = malloc(NEW_BUFF_SIZE * sizeof(char));
               pars->state = req_reading_buff_size;
            } else{ 
               pars->state = req_invalid_cmd;
            }
            break;
        
        case req_reading_buff_size:
            if(c == '\n'){
                pars->new_buff_size[pars->index++] = '\0';
                pars->state = req_done;
            }else if(c >= '0' && c <= '9'){
                if(pars->index == NEW_BUFF_SIZE){
                    add_string_space(pars->new_buff_size, CHUNK_SIZE);
                }
                pars->new_buff_size[pars->index++] = c;
            }else{
                pars->state = req_invalid_cmd;
            }
            break;

        case req_reading_metrics_cmd:
        case req_reading_list_user_cmd:
            if(c == '\n'){
                pars->state = req_done;
            }else{
                pars->state = req_invalid_cmd;
            }
            break;
       
        case req_done:
        case req_invalid_cmd:
            break;
        default:
            exit(EXIT_FAILURE);
    }
    return pars->state;
    
}

void admin_req_fill_msg(buffer * buff, struct req_parser * pars){
    size_t space_left;
    unsigned state = pars->state;
    uint8_t * where_to_write = buffer_write_ptr(buff, &space_left);
    char * aux = malloc(ANSWER_LEN * sizeof(char));
    int ret;
    if(space_left < 1){
        return ;  //TODO: ver este return!!!
    }
    if(state == req_invalid_cmd){
        sprintf(aux, "\nInvalid command\n");
        memcpy(where_to_write, aux, strlen(aux) + 1);
    } else if(req_done){
        switch(pars->cmd_selected){
            case '1':
                load_metrics(where_to_write, aux);
                break;
            case '2':
                ret = create_user(pars);
                if(ret == 0){
                    sprintf(aux, "\nSuccess - New user added\n");
                }else{
                    sprintf(aux, "\nError - already max number of users exist\n");
                }
                memcpy(where_to_write, aux, strlen(aux) + 1);
                break;
            case '3': 
                load_users(where_to_write, &aux);
                break;
            case '4':
                ret = change_buffer_size(pars->new_buff_size);
                if(ret == 0){
                    sprintf(aux, "\nSuccess - Buffer size changed\n");
                }else{
                    sprintf(aux, "\nError - Invalid buffer size given\n");
                }
                memcpy(where_to_write, aux, strlen(aux) + 1);
                break;
        }
    }
    buffer_write_adv(buff, strlen(aux)+1);
    free(aux);
    admin_req_parser_init(pars);
}

void load_metrics(uint8_t * answer, char * aux){
    long historical_connections, concurrent_connections;
    unsigned long long total_bytes;
    historical_connections = get_historical_connections();
    concurrent_connections = get_concurrent_connections();
    total_bytes = get_total_bytes_transferred();
    sprintf(aux, "\nHistorical connections: %ld\nConcurrent connections: %ld\nTotal bytes transfered: %lld\n", historical_connections, concurrent_connections, total_bytes);
    memcpy(answer, aux, strlen(aux) + 1);
}

void load_users(uint8_t * answer, char ** aux){
    *aux[0]=0;
    for(unsigned int i = 0; i < parameters->user_count; i++) {
        char * name = parameters->users[i].name;
        char * pass = parameters->users[i].pass;
        size_t user_string_size = 25 + strlen(name) + strlen(pass); //'username: '= 10 + ' - password: ' = 13 + '\n' = 1 + '\0' = 1

        *aux = realloc(*aux, strlen(*aux) + user_string_size * sizeof(**aux));
        sprintf(*aux + strlen(*aux), "username: %s - password: %s\n", name, pass);
    }
    memcpy(answer, *aux, strlen(*aux) + 1);
}

//retorna 1 en caso exitoso y 0 si falla
int create_user(struct req_parser * pars){
    char * name = pars->user;
    char * pass = pars->password;
    return add_user(name, pass);
}

int change_buffer_size(char * new_buff_size){
    uint64_t new_size = 0;
    for(int i = 0; i < strlen(new_buff_size); i++){
        new_size = new_size * 10 + (new_buff_size[i] - '0');
    }
    return set_buff_size(new_size);
}
