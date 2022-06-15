#include <stdio.h>
#include <netdb.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>

// CONFIG
#define MAX_NUMBER_OF_PARAMETERS 3
#define ANS_BUFF_SIZE 10000

// PROTOCOL CODES
#define VALID_USR 0x00
#define INVALID_USR 0x01
#define VERSION_NOT_SUPPORTED 0x02
#define INTERNAL_ERROR 0x03
#define PROTOCOL_VERSION 0x05
#define AUTH_METHOD_SUPPORTED 0x02
#define NO_AUTH_METHOD_SUPPORTED 0x00

// ERRORS
#define INVALID_AUTH_OPTION -1
#define INVALID_NUMBER_OF_PARAMETERS 0
#define HOST_NOTFOUND -1
#define ERROR_CREATING_SOCKET -2
#define IMPOSIBLE_TO_CONNECT -3
#define WRITE_TO_SERVER_FAILED -4

// PARAMETERS
char usr  [255] = {0};
char pwd  [255] = {0};
char host [255] = {0};
int port_number = 0;
int auth_required = 0;

// METHODS
int verify_parameters(int argc, char ** argv);
void request_credentials();
void send_greeting_to_server();

// Este codigo correrlo en otra terminal, pasarle por entrada estandar:
//
// ./client <DONDE TENGO EL SERVER CORRIENDO> <PUERTO> <CON (1) O SIN AUTH (0)>
//
// Una vez que ingresamos el usr o la contrasenia elegir que hacer

int main( int argc, char ** argv ) {

    if( verify_parameters(argc, argv) == 1 ) {


        // Pedimos la informacion del servidor
        struct hostent * server = gethostbyname(host);
        if( server == NULL ) {
            fprintf(stderr, "Host not found.");
            exit(HOST_NOTFOUND);
        }

        // Configuracion pre-conexion
        int socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if( socket_fd < 0 ) {
            fprintf(stderr, "Coudn't create socket.");
            exit(ERROR_CREATING_SOCKET);
        }

        // Configuracion pre-conexion
        struct sockaddr_in addr;

        // Seteamos en 0 la estructura
        bzero(&addr, sizeof(struct sockaddr_in));
        addr.sin_family = AF_INET;
        // Aca deberiamos recorrer toda la lista de addresses del host
        // pero como lo vamos a usar solo para el localhost no hace falta :D
        bcopy(server->h_addr_list[0], &addr.sin_addr.s_addr, server->h_length);
        // Le pasamos el numero de puerto
        addr.sin_port = htons(port_number);

        // Nos conectamos con el servidor
        if( connect(socket_fd, (struct sockaddr * )&addr, sizeof(addr)) < 0 ) {
            fprintf(stderr, "Coudn't connect to server.");
            exit(IMPOSIBLE_TO_CONNECT);
        }

        // Mandamos el greeting
        size_t index = 0;
        int n = 0;
        uint8_t * greeting_msg = malloc(4); // VERSION - 0x02 - 0x00 0x02
        greeting_msg[index++] = PROTOCOL_VERSION;
        greeting_msg[index++] = 0x02;
        greeting_msg[index++] = NO_AUTH_METHOD_SUPPORTED;
        greeting_msg[index++] = AUTH_METHOD_SUPPORTED;
        n = write(socket_fd, greeting_msg, index);
        if( n < 0 ) {
            fprintf(stderr, "Coudn't write to server.");
            exit(WRITE_TO_SERVER_FAILED);
        }else{
            printf("Le mando al proxy el hello\n");
        }

        // Leemos la respuesta al greeting del server
        uint8_t greeting_ans[2];
        bzero(greeting_ans, 2);
        n = recv(socket_fd, greeting_ans, 2, 0);
        if(n < 0){
            fprintf(stderr, "Coudn't receive from server.");
            exit(WRITE_TO_SERVER_FAILED);
        }else{
            printf("Recibí del servidor: %u, %u\n",greeting_ans[0], greeting_ans[1]);

        }
        if( greeting_ans[1] == 2 ){

            // Mandamos las credenciales
            request_credentials();
            index = 0;
            uint8_t * auth_msg = malloc( 3 + sizeof(usr) + sizeof(pwd) );
            auth_msg[index++] = 0x01;
            auth_msg[index++] = strlen(usr);
            for(; index < strlen(usr)+2; index++)
                auth_msg[index] = usr[index-2];
            auth_msg[index++] = strlen(pwd);
            for(; index < strlen(pwd)+3+strlen(usr); index++)
                auth_msg[index] = pwd[index-3-strlen(usr)];
            n = write(socket_fd, auth_msg, index);
            if( n < 0 ) {
                fprintf(stderr, "Coudn't write to server.");
                exit(WRITE_TO_SERVER_FAILED);
            }


        } else {

        }
    } else {
        return 0;
    }

    return 1;

}

void request_credentials() {
    printf("Username: ");
    scanf("%s", usr);
    printf("Password: ");
    scanf("%s", pwd);
}

int verify_parameters(int argc, char ** argv) {

    if( argc != (MAX_NUMBER_OF_PARAMETERS + 1) ) {
        printf("Invalid number of arguments, given %d requiered %d.\n", argc-1, MAX_NUMBER_OF_PARAMETERS);
        return INVALID_NUMBER_OF_PARAMETERS;
    }

    // Nos guardamos los parametros que nos pasaron.
    strcpy(host, argv[1]);
    port_number = atoi(argv[2]);
    auth_required = argv[3][0] - '0';

    if( auth_required > 2 ) {
        printf("Invalid auth option, must be either 0 or 1, given %d.\n", auth_required);
        return INVALID_AUTH_OPTION;
    }

    return 1;

}