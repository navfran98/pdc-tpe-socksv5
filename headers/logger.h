#ifndef PROTOS2022A_LOGGER
#define PROTOS2022A_LOGGER

#include "buffer.h"
#include "selector.h"

#define LOGGER_BUFFER_SIZE 4096 * 100



struct logger {
    buffer  buffer_stdout;   
    buffer  buffer_stderr;   
    uint8_t *buffer_data_stdout;
    uint8_t *buffer_data_stderr;
};

void log_new_connection(char * msg);
void log_new_request(char * request_string, char * msg);
void log_new_response(char * request_string ,char * msg);

int logger_init();

/** destruye el logger */
void logger_destroy();


#endif
