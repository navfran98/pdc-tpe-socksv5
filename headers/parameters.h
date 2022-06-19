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
    char * socksv5_ipv4;
    char * socksv5_ipv6;
    unsigned short socksv5_port;

    char * mng_ipv4; 
    char * mng_ipv6;
    unsigned short mng_port;

    //Opción para habilitar pop3 sniffing
    bool disectors_enabled;

    struct user users[MAX_USERS];
    struct user admin[MAX_ADMINS];

    unsigned user_count;
    unsigned admin_count;
};

typedef struct params * params;

void parse_args(int argc, char **argv);

extern params parameters;

bool authenticate_user(uint8_t * username, uint8_t * password);
bool authenticate_admin(uint8_t * username, uint8_t * password);

#endif
