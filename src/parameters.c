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
     char *end     = 0;
    const long sl = strtol(s, &end, 10);

    if (end == s|| '\0' != *end
    || ((LONG_MIN == sl || LONG_MAX == sl) && ERANGE == errno)
    || sl < 0 || sl > USHRT_MAX) {
        fprintf(stderr, "port should in in the range of 1-65536: %s\n", s);
        exit(1);
        return 1;
    }
    return (unsigned short)sl;
}


static void
version(void) {
    fprintf(stderr, "socks5v version\n");
}

static void
usage(const char *progname) {
    fprintf(stderr,
        "Usage: %s [OPTION]...\n"
        "\n"
        "   -h               Imprime la ayuda y termina.\n"
        "   -l <SOCKS addr>  Dirección donde servirá el proxy SOCKS.\n"
        "   -L <conf  addr>  Dirección donde servirá el servicio de management.\n"
        "   -p <SOCKS port>  Puerto entrante conexiones SOCKS.\n"
        "   -P <conf port>   Puerto entrante conexiones configuracion\n"
        "   -v               Imprime información sobre la versión versión y termina.\n"
        "\n",
        progname);
    exit(1);
}

void 
parse_args(const int argc, char **argv) {

    parameters = calloc(1,sizeof(*parameters));
    parameters->socksv5_ipv4 = SOCKS_ADDR_IPV4;
    parameters->socksv5_ipv6 = SOCKS_ADDR_IPV6;
    parameters->socksv5_port = SOCKS_PORT;

    parameters->mng_ipv4 = MANAGER_ADDR_IPV4;
    parameters->mng_ipv6 = MANAGER_ADDR_IPV6;
    parameters->mng_port = MANAGER_PORT;

    parameters->disectors_enabled = DISECTORS_ENABLED;

    int c;

    while (true) {
        c = getopt(argc, argv, "hl:L:Np:P:v");
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


