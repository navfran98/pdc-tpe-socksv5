#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <arpa/inet.h>

#include "../headers/selector.h"
#include "../headers/logger.h"
#include "../headers/socksv5_server.h"

#define TIMESTAMP_LEN 30
#define N(x) (sizeof(x)/sizeof((x)[0]))
#define ATTACHMENT(key) ((struct socksv5*)(key)->data)


//TODO: revisar si se puede cambiar esto para imprimir bien la ipv6
void ipv6_expander(const struct in6_addr * addr, char * str) {
    sprintf(str,"%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
    (int)addr->s6_addr[0], (int)addr->s6_addr[1],
    (int)addr->s6_addr[2], (int)addr->s6_addr[3],
    (int)addr->s6_addr[4], (int)addr->s6_addr[5],
    (int)addr->s6_addr[6], (int)addr->s6_addr[7],
    (int)addr->s6_addr[8], (int)addr->s6_addr[9],
    (int)addr->s6_addr[10], (int)addr->s6_addr[11],
    (int)addr->s6_addr[12], (int)addr->s6_addr[13],
    (int)addr->s6_addr[14], (int)addr->s6_addr[15]);
}

void log_new_connection(char * msg, struct selector_key * key){
    struct socksv5 * socksv5 = ATTACHMENT(key);

    // 1. Armo el string para la fecha y hora
    char time_str[TIMESTAMP_LEN] = {0};
    unsigned n = N(time_str);
    time_t now = 0;
    time(&now);

    strftime(time_str, n, "%F %T", gmtime(&now));

    // 2. Armo el string con los datos de la conexión y lo printeo
    int atyp = socksv5->request.request_parser.atyp;
    ssize_t client_port = htons(((struct sockaddr_in*)&socksv5->client_addr)->sin_port);
    ssize_t origin_port = htons(socksv5->request.origin_addr_ipv4.sin_port);
    int status_code = socksv5->request.request_parser.reply;
    
    if(atyp == IPV4){
        char * client_address = malloc(16*sizeof(char));
        char * origin_address = malloc(16*sizeof(char));
        struct in_addr client_ad = ((struct sockaddr_in *)&socksv5->client_addr)->sin_addr;
        strcpy(client_address, inet_ntoa(client_ad));
        struct in_addr orig_ad = socksv5->request.origin_addr_ipv4.sin_addr;
        strcpy(origin_address,inet_ntoa(orig_ad));
        if(socksv5->connected_user.name != NULL){
            char * username = socksv5->connected_user.name;
            fprintf(stdout, "[%s] - %s: %s\tA\t%s\t%lu\t%s\t%lu\t%d\n", time_str, msg, username, client_address, client_port, origin_address,origin_port, status_code);
        } else {
            fprintf(stdout, "[%s] - %s: Non-auth user\tA\t%s\t%lu\t%s\t%lu\t%d\n", time_str, msg, client_address, client_port, origin_address,origin_port, status_code);
        }
        free(client_address);
        free(origin_address);
    } else {
        char * client_address = malloc(64*sizeof(char));
        char * origin_address = malloc(64*sizeof(char));
        struct in6_addr client_ad = ((struct sockaddr_in6*)&socksv5->client_addr)->sin6_addr;
        ipv6_expander(&client_ad, client_address);
        struct in6_addr orig_ad = socksv5->request.origin_addr_ipv6.sin6_addr;
        ipv6_expander(&orig_ad, origin_address);
        
        if(socksv5->connected_user.name != NULL){
            char * username = socksv5->connected_user.name;
            fprintf(stdout, "[%s] - %s:\t%s\tA\t%s\t%lu\t%s\t%lu\t%d\n", time_str, msg, username, client_address, client_port, origin_address, origin_port, status_code);
        } else {
            fprintf(stdout, "[%s] - %s:\tA\t%s\t%lu\t%s\t%lu\t%d\n", time_str, msg, client_address, client_port, origin_address, origin_port, status_code);
        free(client_address);
        free(origin_address); 
        }
    }
}

void log_pop3_sniff(struct selector_key * key){
    struct socksv5 * socksv5 = ATTACHMENT(key);
    // 1. Armo el string para la fecha y hora
    char time_str[TIMESTAMP_LEN] = {0};
    unsigned n = N(time_str);
    time_t now = 0;
    time(&now);
    strftime(time_str, n, "%F %T", gmtime(&now));

    // 2. Armo el string con los datos de la conexión y lo printeo
    int atyp = socksv5->request.request_parser.atyp;
    ssize_t origin_port = htons(socksv5->request.origin_addr_ipv4.sin_port);
    char * origin_address;
    if(atyp == IPV4){
        origin_address = calloc(16,sizeof(char));
        struct in_addr orig_ad = socksv5->request.origin_addr_ipv4.sin_addr;
        strcpy(origin_address,inet_ntoa(orig_ad));
    } else {
        origin_address = calloc(64,sizeof(char));
        struct in6_addr orig_ad = socksv5->request.origin_addr_ipv6.sin6_addr;
        ipv6_expander(&orig_ad, origin_address);
    }

    if(socksv5->connected_user.name != NULL){   
        char * username = socksv5->connected_user.name;
        printf("[%s] - POP3 Sniff\t%s\tP\tPOP3\t%s\t%lu\t%s\t%s\n", time_str, username,origin_address,origin_port, socksv5->pop3sniffer.user, socksv5->pop3sniffer.pass);
    } else {
        printf("[%s] - POP3 Sniff\tP\tPOP3\t%s\t%lu\t%s\t%s\n", time_str, origin_address, origin_port, socksv5->pop3sniffer.user, socksv5->pop3sniffer.pass);       
    }
    free(origin_address);
}

