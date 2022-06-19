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

//Iniciamos el logger
int logger_init();

//Loggeamos una nueva conexion
void log_new_connection(char * msg, struct selector_key * key);

//Loggeamos los usuarios y contraseñas obtenidas con el pop3 sniff
void log_sniff(char * request_string, char * msg); // Para ver que se está mandando, acá se logea el sniff.

/** destruye el logger */
void logger_destroy();


#endif
