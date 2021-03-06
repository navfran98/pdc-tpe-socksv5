#include <stdio.h>     /* for printf */
#include <stdlib.h>    /* for exit */
#include <limits.h>    /* LONG_MIN et al */
#include <string.h>    /* memset */
#include <errno.h>
#include <getopt.h>

#include "../headers/parameters.h"
#include "../headers/config.h"

params parameters;

static unsigned short
check_port(const char *s) {
    char * end = 0;
    const long sl = strtol(s, &end, 10);

    if (end == s|| '\0' != * end || ((LONG_MIN == sl || LONG_MAX == sl) && ERANGE == errno) || sl < 0 || sl > USHRT_MAX) {
        fprintf(stderr, "port should in in the range of 1-65536: %s\n", s);
        exit(1);
        return 1;
    }
    return (unsigned short)sl;
}

int splitstr(char * str,int len, char delim, char * str1, char * str2){
    int flag = 0;
    int j = 0;
    for(int i=0; i < len; i++){
        if(flag == 0){
            if(str[i] == delim)
                flag++;
            else
                str1[i] = str[i];
        }else{
            str2[j++] = str[i];
        } 
    }
    return flag; 
}


static void user(char * s, struct user * user) {
    int len = strlen(s); 
    char * name = calloc(len,sizeof(char));
    char * pass = calloc(len,sizeof(char));
    splitstr(s,len, ':', name, pass);
    if(strlen(pass)  == 0 || strlen(name)  == 0) {
        fprintf(stderr, "Invalid user declaration\n");
        exit(1);
    } else if(strlen(pass) > MAX_CREDENTIALS_LEN || strlen(name) > MAX_CREDENTIALS_LEN){
        fprintf(stderr, "Credentials must have less than 50 characters\n");
        exit(1);
    } else {
        user->name = name;
        user->pass = pass;
    }
}

int add_user(char * name, char * pass){
    if(strlen(pass) > MAX_CREDENTIALS_LEN || strlen(name) > MAX_CREDENTIALS_LEN){
        return -2;
    }
    if(parameters->user_count < MAX_USERS){
        struct user * user_to_add = parameters->users + parameters->user_count;
        user_to_add->name = name;
        user_to_add->pass = pass;
        parameters->user_count++;
        return 0;
    }
    return -1;
}




static void
version(void) {
    fprintf(stderr, "Socksv5 Proxy version\n");
}

static void
usage(const char * name) {
    fprintf(stderr,
        "Guide: %s [OPTION]\n"
        "\n"
        "   -h                              Imprime gu??a para ejecutar el servidor.\n"
        "   -l <SOCKSv5 addr>               Direcci??n donde servir?? el servidor proxy SOCKSv5.\n"
        "   -L <monit addr>                 Direcci??n donde servir?? el servidor de monitoreo y control.\n"
        "   -p <SOCKSv5 port>               Puerto entrante para conexiones al servidor proxy SOCKSv5.\n"
        "   -P <monit port>                 Puerto entrante para conexiones al servidor de monitoreo y control.\n"
        "   -v                              Imprime versi??n del servidor proxy SOCKSv5.\n"
        "   -u <username>:<password>        Registra dicho usuario para luego conectarse al servidor.\n"
        "   -U <admin_name>:<admin_pass>    Registra al usuario admin para acceder al servidor de monitoreo.\n"
        "   -N                              Deshabilitas los disectores para sniffin POP3.\n"
        "\n",
        name);
    exit(1);
}

void 
parse_args(const int argc, char **argv) {

    parameters = calloc(1,sizeof(*parameters));
    parameters->socksv5_ipv4 = SOCKS_ADDR_IPV4;
    parameters->socksv5_ipv6 = SOCKS_ADDR_IPV6;
    parameters->socksv5_port = SOCKS_PORT;

    parameters->mng_ipv4 = ADMIN_ADDR_IPV4;
    parameters->mng_ipv6 = ADMIN_ADDR_IPV6;
    parameters->mng_port = ADMIN_PORT;

    parameters->disectors_enabled = DISECTORS_ENABLED;

    int c;
    parameters->user_count = 0;
    parameters->admin_count = 0;
    while(true) {
        c = getopt(argc, argv, "hl:L:Np:P:vu:U:a:");
        if (c == -1)
            break;

        switch (c) {
            case 'h':
                usage(argv[0]);
                break;
            case 'l':
                parameters->socksv5_ipv4 = optarg;
                break;
            case 'L':
                parameters->mng_ipv4 = optarg;
                break;
            case 'N':
                parameters->disectors_enabled = false;
                break;
            case 'p':
                parameters->socksv5_port = check_port(optarg);
                break;
            case 'P':
                parameters->mng_port = check_port(optarg);
                break;

            case 'a':
                if(parameters->admin_count >= MAX_ADMINS) {
                    fprintf(stderr, "Can't have more than %d admins.\n", MAX_ADMINS);
                    exit(1);
                } else {
                    user(optarg, parameters->admin + parameters->admin_count);
                    parameters->admin_count++;
                }
                break;

            case 'u':
                if(parameters->user_count >= MAX_USERS) {
                    fprintf(stderr, "Can't have more than %d users.\n", MAX_USERS);
                    exit(1);
                } else {
                    user(optarg, parameters->users + parameters->user_count);
                    parameters->user_count++;
                }
                break;
        
            case 'v':
                version();
                exit(0);
                break;
            default:
                fprintf(stderr, "unknown argument %d.\n", c);
                exit(1);
        }

    }
    if (optind < argc) {
        fprintf(stderr, "argument not accepted: ");
        while (optind < argc) {
            fprintf(stderr, "%s ", argv[optind++]);
        }
        fprintf(stderr, "\n");
        exit(1);
    }
}


bool authenticate_user(uint8_t * usr, uint8_t * password){
    for(unsigned int i = 0; i < parameters->user_count; i++) {
        const char * aux_name = parameters->users[i].name;
        const char * aux_pass = parameters->users[i].pass;
        if (strcmp((const char *) usr, aux_name) == 0 && strcmp((const char *) password, aux_pass) == 0)
            return true;
    }
    return false;
}

bool authenticate_admin(uint8_t * usr, uint8_t * password) {
    for(unsigned int i = 0; i < parameters->admin_count; i++) {
        const char* aux_name = parameters->admin[i].name;
        const char* aux_pass = parameters->admin[i].pass;
        if (strcmp((const char *) usr, aux_name) == 0 && strcmp((const char *) password, aux_pass) == 0)
            return true;
    }
    return false;
}



