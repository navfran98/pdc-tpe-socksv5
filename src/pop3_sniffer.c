#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include "../headers/pop3_sniffer.h"
#include "../headers/buffer.h"
#include "../headers/selector.h"
#include "../headers/socksv5_server.h"
#include "../headers/logger.h"


#define N(x) (sizeof(x)/sizeof((x)[0]))

static void reset_indexes(struct pop3_sniffer* sniffer, uint8_t left){
    sniffer->read = 0;
    sniffer->remaining = left;
}

void pop3_sniffer_init(struct pop3_sniffer* sniffer){
    sniffer->state = POP3_INIT;

    memset(sniffer->buff, 0, POP3_BUFF);
    buffer_init(&sniffer->buffer, N(sniffer->buff), sniffer->buff);

    sniffer->read = 0;
    sniffer->remaining = strlen("+OK");
    sniffer->is_parsing = true;
    sniffer->user = calloc(CREDENTIALS_SIZE,sizeof(char));
    sniffer->pass = calloc(CREDENTIALS_SIZE,sizeof(char));
}

static enum pop3_sniffer_state read_ok_msg(struct pop3_sniffer* sniffer, uint8_t ch){

    enum pop3_sniffer_state st = POP3_INIT;

    if(tolower(ch) == tolower(*("+OK" + sniffer->read))){
        sniffer->read++;
        sniffer->remaining--;

        if(sniffer->remaining == 0){
            st = POP3_USER_COMMAND;
            sniffer->read = 0;
            sniffer->remaining = strlen("USER ");
        }
    }
    else{
        st = POP3_FINISH;
    }
    return st;
}

enum pop3_sniffer_state read_user_command(struct pop3_sniffer* sniffer, uint8_t ch){

    enum pop3_sniffer_state st = POP3_USER_COMMAND;

    if(tolower(ch) == tolower(*("USER " + sniffer->read))){
        sniffer->read++;
        sniffer->remaining--;

        if(sniffer->remaining == 0){
            sniffer->read = 0;
            st = POP3_READ_USER;
        }        
    }
    else{
        if(sniffer->read != 0){
            reset_indexes(sniffer, strlen("USER "));
        }
    } 
    return st;
}

enum pop3_sniffer_state read_user(struct pop3_sniffer* sniffer, uint8_t ch){

    enum pop3_sniffer_state st = POP3_READ_USER;

    if(ch != '\n' && ch != '\r'){
        if(sniffer->read < CREDENTIALS_SIZE){
            sniffer->user[sniffer->read++] = ch;
        }
    }
    else{
        sniffer->user[sniffer->read] = '\0';
        sniffer->read = 0;
        sniffer->remaining = strlen("PASS ");
        sniffer->check_read = 0;
        sniffer->check_remaining = strlen("-ERR");
        st = POP3_PASS_COMMAND;
}
    return st;
}

enum pop3_sniffer_state read_pass_command(struct pop3_sniffer* sniffer, uint8_t ch){

    enum pop3_sniffer_state st = POP3_PASS_COMMAND;

    if(tolower(ch) == tolower(*("PASS " + sniffer->read))){
        sniffer->read++;
        sniffer->remaining--;

        if(sniffer->remaining == 0){
            sniffer->read = 0;
            st = POP3_READ_PASS;
        }        
    }
    else{
        if(sniffer->read != 0){
            reset_indexes(sniffer, strlen("PASS "));
        }
    } 
    return st;
}

enum pop3_sniffer_state read_pass(struct pop3_sniffer* sniffer, uint8_t ch){

    enum pop3_sniffer_state st = POP3_READ_PASS;

    if(ch != '\n' && ch != '\r'){
        if(sniffer->read < CREDENTIALS_SIZE){
            sniffer->pass[sniffer->read++] = ch;
        }
    }
    else{
        sniffer->pass[sniffer->read] = '\0';
        sniffer->read = 0;
        sniffer->check_read = 0;
        st = POP3_CHECK_CREDS; 
}
    return st;
}

enum pop3_sniffer_state check_response(struct pop3_sniffer* sniffer, uint8_t ch){

    enum pop3_sniffer_state st = POP3_CHECK_CREDS;

    if(tolower(ch) == tolower(*("+OK" + sniffer->read))){
        sniffer->read++;
        if(sniffer->read == strlen("+OK")){
            st = POP3_SNIFF_SUCCESSFUL;
        }
    }
    else if(tolower(ch) == tolower(*("-ERR" + sniffer->check_read))){
        sniffer->check_read++;
        if(sniffer->check_read == strlen("-ERR")){
            st = POP3_USER_COMMAND;
        }
    }
    return st;
}

enum pop3_sniffer_state parse_pop3_sniffer(struct pop3_sniffer* sniffer, uint8_t ch){
    switch (sniffer->state)
    {
    case POP3_INIT:
        sniffer->state = read_ok_msg(sniffer, ch);
        break;
    case POP3_USER_COMMAND:
        sniffer->state = read_user_command(sniffer, ch);
        break;
    case POP3_READ_USER:
        sniffer->state = read_user(sniffer, ch);
        break;
    case POP3_PASS_COMMAND:
        sniffer->state = read_pass_command(sniffer, ch);
        break;
    case POP3_READ_PASS:
        sniffer->state = read_pass(sniffer, ch);
        break;
    case POP3_CHECK_CREDS:
        sniffer->state = check_response(sniffer, ch);
        break;    
    case POP3_SNIFF_SUCCESSFUL: 
    case POP3_FINISH:
        break;    
    default:
        break;
    }
    return sniffer->state;
}

enum pop3_sniffer_state pop3_sniffer_consume(struct selector_key * key, struct pop3_sniffer * sniffer){
    while(buffer_can_read(&sniffer->buffer) && sniffer->state != POP3_FINISH && sniffer->state != POP3_SNIFF_SUCCESSFUL){
        uint8_t ch = buffer_read(&sniffer->buffer);
        parse_pop3_sniffer(sniffer, ch);
    }
    if(sniffer->state == POP3_SNIFF_SUCCESSFUL){
       //printf("**** POP3 SNIFFER: ****\nUsername: %s\nPassword: %s\n***********************\n", sniffer->user, sniffer->pass);
        // char * aux = calloc(5*CREDENTIALS_SIZE,sizeof(char));
        // sprintf(aux, "%s\t%s", sniffer->user, sniffer->pass);
        log_pop3_sniff(key);
        //free(aux);
    }
    return sniffer->state;
}