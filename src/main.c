#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "../headers/logger.h"
#include "../headers/socket_errors.h"
#include "../headers/selector.h"
#include "../headers/parameters.h"
#include "../headers/socksv5_server.h"

static bool done = false;

static void set_std_non_blocking();
static int register_all_fds(int fd1, int fd2, int fd3, int fd4, fd_selector s);

static enum socket_errors create_ipv4_passive_socket(int * ret_socket);
static enum socket_errors create_ipv6_passive_socket(int * ret_socket);
static enum socket_errors create_ipv4_manager_passive_socket(int * ret_socket);
static enum socket_errors create_ipv6_manager_passive_socket(int * ret_socket);

// Definimos los handlers para el socket pasivo
static const struct fd_handler socksv5_passive_handler = {
    .handle_read       = socksv5_passive_accept,
    .handle_write      = NULL,  
    .handle_close      = NULL, 
};

// Definimos los handlers para el socket pasivo de nuestro protocolo
static const struct fd_handler manager_passive_handler = {
    .handle_read       = NULL /*admin_passive_accept*/,
    .handle_write      = NULL,
    .handle_close      = NULL, 
};

static void
sigterm_handler(const int signal) {
    printf("Received Signal: %d - Closing\n", signal);
    done = true;
}

int main(const int argc, const char ** argv){
    parse_args(argc, (char **) argv); //TODO: fijarse de tener globalizado parameters

    close(STDIN_FILENO);
    
    set_std_non_blocking();

    selector_status ss = SELECTOR_SUCCESS;
    fd_selector selector = NULL;

    const char * err_msg = NULL;

    int ipv4_passive_socket = -1, ipv6_passive_socket = -1;
    int ipv4_manager_passive_socket = -1, ipv6_manager_passive_socket = -1;
    bool one_passive_socket_failed = false;
    bool one_manager_passive_socket_failed = false;

    //creo socket ipv4
    enum socket_errors error = create_ipv4_passive_socket( &ipv4_passive_socket);
    if(error != socket_no_fail) {
        printf("%d\n", error);
        printf("Error when creating IPV4 socket\n");
        one_passive_socket_failed = true;
    }

    //creo socket ipv6
    error = create_ipv6_passive_socket( &ipv6_passive_socket);
    if(error != socket_no_fail) {
        if(one_passive_socket_failed) {
            printf("Error both pasive sockets failed - Closing\n");
            err_msg = socket_error_description[error];
            goto finally;
        } else {
            printf("Error when creating IPV6 socket\n");
        }
    }

    error = create_ipv4_manager_passive_socket( &ipv4_manager_passive_socket);
    if(error != socket_no_fail) {
        printf("Error when creating admin IPV4 socket\n");
        one_manager_passive_socket_failed = true;
    }
    error = create_ipv6_manager_passive_socket( &ipv6_manager_passive_socket);
    if(error != socket_no_fail) {
        if(one_manager_passive_socket_failed) {
            printf("Error both admin pasive sockets failed - Could not start admin server\n");
            err_msg = socket_error_description[error];
        } else {
            printf("Error when creating admin IPV6 socket\n");
        }
    }


    if(logger_init() == -1) {
        printf("Error when creating logger\n");
    }
    
    signal(SIGTERM, sigterm_handler);
    signal(SIGINT,  sigterm_handler);

    const struct selector_init conf = {
        .signal = SIGALRM,
        .select_timeout = {
            .tv_sec  = 2,
            .tv_nsec = 0,
        },
    };

    if(0 != selector_init(&conf)) {
        err_msg = "Error when initializing selector";
        goto finally;
    }

    selector = selector_new(1024);
    if(selector == NULL) {
        err_msg = "Error when creating selector";
        goto finally;
    }

    ss = register_all_fds(ipv4_passive_socket, ipv6_passive_socket, ipv4_manager_passive_socket, ipv6_manager_passive_socket, selector);

    if(ss != SELECTOR_SUCCESS) {
        err_msg = "Error when registering fd";
        goto finally;
    }

    printf("Running SocksV5 server...\n");

    for(;!done;) {  
        err_msg = NULL;
        ss = selector_select(selector);
        if(ss != SELECTOR_SUCCESS) {
            err_msg = "Serving";
            goto finally;
        }
    }
    if(err_msg == NULL) {
        err_msg = "Closing";
    }

   
finally:
    ;
    int ret = 0;
    if(ss != SELECTOR_SUCCESS) {
        fprintf(stderr, "%s: %s\n", (err_msg == NULL) ? "": err_msg, ss == SELECTOR_IO ? strerror(errno) : selector_error(ss));
        ret = 2;
    } else if(err_msg) {
        perror(err_msg);
        ret = 1;
    }
    if(selector != NULL) {
        selector_destroy(selector);
    }
    selector_close();

    if(ipv4_passive_socket >= 0) {
        close(ipv4_passive_socket);
    }
    if(ipv6_passive_socket >= 0) {
        close(ipv6_passive_socket);
    }
    if(ipv4_manager_passive_socket >= 0) {
        close(ipv4_manager_passive_socket);
    }
    if(ipv6_manager_passive_socket >= 0) {
        close(ipv6_manager_passive_socket);
    }

    destroy_socksv5_pool(); 
    logger_destroy();
    // free(parameters->pass);
    // free(parameters->user);

    return ret;
}

static int register_all_fds(int fd1, int fd2, int fd3, int fd4, fd_selector s) {

    bool fd1_failed = true, fd2_failed = true;
    bool fd3_failed = true, fd4_failed = true;

    int ss;

    if(fd1 != -1) {
        ss = selector_register(s, fd1, &socksv5_passive_handler, OP_READ, NULL);
        if(ss == SELECTOR_SUCCESS)
            fd1_failed = false;
    }
    if(fd2 != -1) {
        ss = selector_register(s, fd2, &socksv5_passive_handler, OP_READ, NULL);
        if(ss == SELECTOR_SUCCESS)
            fd2_failed = false;
    }
    if(fd3 != -1) {
        ss = selector_register(s, fd3, &manager_passive_handler, OP_READ, NULL);
        if(ss == SELECTOR_SUCCESS)
            fd3_failed = false;
    }
    if(fd4 != -1) {
        ss = selector_register(s, fd4, &manager_passive_handler, OP_READ, NULL);
        if(ss == SELECTOR_SUCCESS)
            fd4_failed = false;
    }

    // ss = selector_register(s, STDOUT_FILENO, &logger_handler, OP_NOOP, NULL);
    // if(ss == SELECTOR_SUCCESS)
    //     stdout_failed = false;

    // ss = selector_register(s, STDERR_FILENO, &logger_handler, OP_NOOP, NULL);
    // if(ss == SELECTOR_SUCCESS)
    //     stderr_failed = false;

    if( (fd1_failed && fd2_failed) || (fd3_failed && fd4_failed)) {
        return -1;
    }

    return SELECTOR_SUCCESS;
}

static enum socket_errors create_ipv4_passive_socket(int *ret_socket) {

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    if(inet_pton(AF_INET, parameters->socksv5_ipv4, &addr.sin_addr) == 0) {
        return socket_inet_pton_error;
    }
    addr.sin_port = htons(parameters->socksv5_port);

    *ret_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(*ret_socket < 0) {
        return socket_error;
    }

    setsockopt(*ret_socket, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));

    if(bind(*ret_socket, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
        return socket_bind_error;
    }

    if(listen(*ret_socket, 20) < 0) {
        return socket_listen_error;
    }

    if(selector_fd_set_nio(*ret_socket) == -1) {
        return socket_selector_fd_set_nio_error;
    }

    return socket_no_fail;
}

static enum socket_errors create_ipv6_passive_socket(int *ret_socket) {

    struct sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    if (inet_pton(AF_INET6, parameters->socksv5_ipv6, &addr.sin6_addr) == 0) {
        return socket_inet_pton_error;
    }
    addr.sin6_port = htons(parameters->socksv5_port);

    *ret_socket = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (*ret_socket < 0) {
        return socket_error;
    }

    setsockopt(*ret_socket, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));

    if (setsockopt(*ret_socket, IPPROTO_IPV6, IPV6_V6ONLY, &(int){ 1 }, sizeof(int)) < 0) {
        return socket_setsockopt_error;
    }

    if(bind(*ret_socket, (struct sockaddr*) &addr, (socklen_t)sizeof(addr)) < 0) {
        return socket_bind_error;
    }

    if(listen(*ret_socket, 20) < 0) {
        return socket_listen_error;
    }

    if(selector_fd_set_nio(*ret_socket) == -1) {
        return socket_selector_fd_set_nio_error;
    }

    return socket_no_fail;
}

static enum socket_errors create_ipv4_manager_passive_socket(int *ret_socket) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    if(inet_pton(AF_INET, parameters->mng_ipv4, &addr.sin_addr) == 0) {
        return socket_inet_pton_error;
    }
    addr.sin_port = htons(parameters->mng_port);

    *ret_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(*ret_socket < 0) {
        return socket_error;
    }

    setsockopt(*ret_socket, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));

    if(bind(*ret_socket, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
        return socket_bind_error;
    }

    if(listen(*ret_socket, 20) < 0) {
        return socket_listen_error;
    }

    if(selector_fd_set_nio(*ret_socket) == -1) {
        return socket_selector_fd_set_nio_error;
    }

    return socket_no_fail;
}

static enum socket_errors create_ipv6_manager_passive_socket(int *ret_socket) {
    struct sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    if (inet_pton(AF_INET6, parameters->mng_ipv6, &addr.sin6_addr) == 0) {
        return socket_inet_pton_error;
    }
    addr.sin6_port = htons(parameters->mng_port);

    *ret_socket = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (*ret_socket < 0) {
        return socket_error;
    }

    setsockopt(*ret_socket, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));

    if (setsockopt(*ret_socket, IPPROTO_IPV6, IPV6_V6ONLY, &(int){ 1 }, sizeof(int)) < 0) {
        return socket_setsockopt_error;
    }

    if(bind(*ret_socket, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
        return socket_bind_error;
    }

    if(listen(*ret_socket, 20) < 0) {
        return socket_listen_error;
    }

    if(selector_fd_set_nio(*ret_socket) == -1) {
        return socket_selector_fd_set_nio_error;
    }

    return socket_no_fail;
}

static void set_std_non_blocking() {
    if(selector_fd_set_nio(STDOUT_FILENO) == -1) {
        printf("Error when making STDOUT non-blocking - Closing\n");
        exit(EXIT_FAILURE);
    }
    if(selector_fd_set_nio(STDERR_FILENO) == -1) {
        printf("Error when making STDERR non-blocking - Closing\n");
        exit(EXIT_FAILURE);
    }
}
