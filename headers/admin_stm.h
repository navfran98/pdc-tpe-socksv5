#ifndef PROTOS2022A_ADMINSTM
#define PROTOS2022A_ADMINSTM

#include "../headers/stm.h"
/*
Metodos que puede pedir el admin:
    - pedir metricas
    - cambiar tamaño buffer
    - agregar nuevo usuario (deberia verificar si puede o ya hay MAX)
    - eliminar el usuario (eliminar usuarios, verificar que no este online) (dsp ver si hace falta)

Estados:
    - Greeting read -> leo la greeting del usuario, q viene con usuario y contra para entrar
    - Greeting write -> le respondo si se pudo autenticar
    - Request read -> leo del cliente una request de algun metodo
    - Request write -> le devuelvo la respuesta al metodo que me pidio y si fue succesfull
    
    Luego me quedo escuchando otra request hasta que se desconecte o envie un comando para cerrar conexcion
*/

enum admin_state {
    ADMIN_AUTH,

    ADMIN_REQUEST,

    ADMIN_DONE, 

    ADMIN_ERROR
};

 const struct state_definition * admin_describe_states(void);

#endif
