#ifndef PROTOS2022A_LOGGER
#define PROTOS2022A_LOGGER

#include "selector.h"

//Loggeamos una nueva conexion
void log_new_connection(char * msg, struct selector_key * key);

#endif
