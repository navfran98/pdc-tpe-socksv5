#ifndef PROTOS2022A_PARAMS
#define PROTOS2022A_PARAMS

#include <stdint.h>
#include <netinet/in.h>
#include <stdbool.h>

#define MAX_USERS 10
#define MAX_ADMINS 1

struct user {
    char *name;
    char *pass;
};

struct params {
    char * socks_ipv4;
    char * socks_ipv6;
    unsigned short socks_port;

    char * mng_ipv4; 
    char * mng_ipv6;
    unsigned short mng_port;

    //Opción para habilitar sniffing
    bool disectors_enabled;

    struct user    users[MAX_USERS];
    struct user    admin[MAX_ADMINS];
};

typedef struct params * params;

void parse_args(int argc, char **argv,  struct params * params);

extern params parameters;

#endif