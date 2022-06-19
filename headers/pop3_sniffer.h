#ifndef POP3SNIFF_H
#define POP3SNIFF_H

#include <stdint.h>
#include <stdbool.h>
#include "../headers/buffer.h"

#define CREDENTIALS_SIZE 255
#define POP3_BUFF 4096

enum pop3_sniffer_state
{
/*
** Busca la respuesta +OK
*/
    POP3_INIT,

/*
** Busca el comando 'USER'
*/
    POP3_USER_COMMAND,

/*
** Guarda el nombre de usuario
*/
    POP3_READ_USER,

/*
** Busca el comando 'PASS' 
*/    
    POP3_PASS_COMMAND,

/*
** Guarda la contraseña 
*/    
    POP3_READ_PASS,

/*
** Chequea si las credenciales son correctas. Si no lo son, se elimina lo que agarramos antes y 
** vamos al estado POP3_USER_COMMAND para empezar a sniffear de nuevo las nuevas credenciales
*/    
    POP3_CHECK_CREDS,

/*
** Si se encuentran credenciales correctas
*/    
    POP3_SNIFF_SUCCESSFUL,
/*
** Si no hay credenciales, se deja de parsear
*/    
    POP3_FINISH
};

struct pop3_sniffer{
    enum pop3_sniffer_state state;

    buffer buffer;
    uint8_t buff[POP3_BUFF];
    char user[CREDENTIALS_SIZE];
    char pass[CREDENTIALS_SIZE];

    bool is_parsing;

    uint8_t read;
    uint8_t remaining;
    uint8_t check_read;
    uint8_t check_remaining;
};

enum pop3_sniffer_state parse_pop3_sniffer(struct pop3_sniffer* sniffer, uint8_t ch);
void pop3_sniffer_init(struct pop3_sniffer * sniffer);
enum pop3_sniffer_state pop3_sniffer_consume(struct pop3_sniffer * sniffer);


#endif
