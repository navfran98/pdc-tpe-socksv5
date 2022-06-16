#ifndef PROTOS2022A_SOCKETERRORS
#define PROTOS2022A_SOCKETERRORS

enum socket_errors {
    socket_error,
    socket_bind_error,
    socket_listen_error,
    socket_inet_pton_error,
    socket_selector_fd_set_nio_error,
    socket_setsockopt_error,
    socket_no_fail,
};

static const char * socket_error_description[] = {
        "Error in socket()",
        "Error in bind()",
        "Error in listen()",
        "Error in inet_pton()",
        "Error in selector_fd_set_nio()",
        "Error in setsockopt()",
};

#endif
