#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#include "../headers/logger.h"

#define TIMESTAMP_LEN 50
#define N(x) (sizeof(x)/sizeof((x)[0]))

static struct logger global_logger;


int logger_init() {

    global_logger.buffer_data_stdout = calloc(LOGGER_BUFFER_SIZE, sizeof(uint8_t));
    if(global_logger.buffer_data_stdout == NULL) {
        return -1;
    }
    buffer_init(&global_logger.buffer_stdout, LOGGER_BUFFER_SIZE, global_logger.buffer_data_stdout);

    global_logger.buffer_data_stderr = calloc(LOGGER_BUFFER_SIZE, sizeof(uint8_t));
    if(global_logger.buffer_data_stderr == NULL) {
        free(global_logger.buffer_data_stdout);
        return -1;
    }
    buffer_init(&global_logger.buffer_stderr, LOGGER_BUFFER_SIZE, global_logger.buffer_data_stderr);

    return 0;
}

struct logger * getLogger() {
    return &global_logger;
}



void logger_write(struct selector_key *key) {

    buffer *buff;

    if(key->fd == STDOUT_FILENO) {
        buff = &global_logger.buffer_stdout;
    } else if(key->fd == STDERR_FILENO) {
        buff = &global_logger.buffer_stderr;
    } else {
        exit(EXIT_FAILURE);
    }

    size_t nbytes;
    uint8_t * where_to_read = buffer_read_ptr(buff, &nbytes);
    int ret = write(key->fd, where_to_read, nbytes);
    if (ret <= 0) {
        return;
    }

    buffer_read_adv(buff, ret);
    if (!buffer_can_read(buff)) {
        selector_set_interest_key(key, OP_NOOP);
    }
}

void logger_destroy() {
    free(global_logger.buffer_data_stdout);
    free(global_logger.buffer_data_stderr);
}

void log_new_connection(char * msg){
    char time_str[TIMESTAMP_LEN] = {0};
    unsigned n = N(time_str);
    time_t now = 0;
    time(&now);

    strftime(time_str, n, "%FT %TZ", gmtime(&now));
    
    fprintf(stdout, "[%s] - %s\n", time_str, msg);
}

void log_new_request(char * request_string, char * msg){
    char time_str[TIMESTAMP_LEN] = {0};
    unsigned n = N(time_str);
    time_t now = 0;
    time(&now);
    
    strftime(time_str, n, "%FT %TZ", gmtime(&now));

    fprintf(stdout, "[%s] - Request: %s , %s\n", time_str, request_string, msg);
    
}

void log_new_response(char * request_string ,char * msg) {
    char time_str[TIMESTAMP_LEN] = {0};
    unsigned n = N(time_str);
    time_t now = 0;
    time(&now);

    strftime(time_str, n, "%FT %TZ", gmtime(&now));

    fprintf(stdout, "[%s] -  Request: %s, %s\n", time_str, request_string, msg);
}
