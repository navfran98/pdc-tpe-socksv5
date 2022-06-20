#include <stdio.h>
#include <netdb.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>

// CONFIG
#define MAX_NUMBER_OF_PARAMETERS 2
#define ANS_BUFF_SIZE 10000
#define clrscr() printf("\033[1;1H\033[2J")
#define context(x) printf("\033[0;32m");printf("[ %s ]\n", x);printf("\033[0m");

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
#define SERVER_NO_RESPONSE -5
#define BAD_SINTAX -6
#define USR_OR_PWD_WRONG -7

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
void print_menu();
void obtener_metricas();
void error_handler(int n, char * msg, int err_num);
void auth_response_printer(uint8_t c);
void wait_to_continue();

// Este codigo correrlo en otra terminal, pasarle por entrada estandar:
//
// ./client <DONDE TENGO EL SERVER CORRIENDO> <PUERTO>
//
// Una vez que ingresamos el usr o la contrasenia elegir que hacer

int main( int argc, char ** argv ) {

    if( verify_parameters(argc, argv) == 1 ) {

        ////////////////////////////////////////////////////////////////////////
        //                                                                    //
        //                 CONFIGURACION DE SOCKETS DEL ADMIN                 //
        //                                                                    //
        ////////////////////////////////////////////////////////////////////////

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
        memset(&addr, 0, sizeof(struct sockaddr_in));
        //bzero(&addr, sizeof(struct sockaddr_in));
        addr.sin_family = AF_INET;
        // Aca deberiamos recorrer toda la lista de addresses del host
        // pero como lo vamos a usar solo para el localhost no hace falta :D
        memcpy(server->h_addr_list[0], &addr.sin_addr.s_addr, server->h_length);
        // Le pasamos el numero de puerto
        addr.sin_port = htons(port_number);
        // Nos conectamos con el servidor
        if( connect(socket_fd, (struct sockaddr * )&addr, sizeof(addr)) < 0 ) {
            fprintf(stderr, "Coudn't connect to server.");
            exit(IMPOSIBLE_TO_CONNECT);
        }

        ////////////////////////////////////////////////////////////////////////
        //                                                                    //
        //                 AUTENTICACION DEL ADMINISTRADOR                    //
        //                                                                    //
        ////////////////////////////////////////////////////////////////////////

        // Mandamos el greeting/auth del admin
        request_credentials();
        size_t index = 0;
        int n = 0; 
        size_t usr_len = strlen(usr), pass_len = strlen(pwd);
        // Alocamos espacio para: las strlen del usr y pass + la usr + el pass
        uint8_t * auth_msg = malloc(2 + usr_len + pass_len);

        // Adjuntamos las credenciales
        auth_msg[index++] = usr_len;
        for(size_t i = 0;i < usr_len;i++, index++) 
            auth_msg[index] = usr[i];
        auth_msg[index++] = pass_len;
        for(size_t i = 0;i < pass_len;i++, index++)
            auth_msg[index] = pwd[i];
        n = write(socket_fd, auth_msg, index);
        
        // Validamos la respuesta
        error_handler(n, "Error al enviar el mensaje de auth.\n", WRITE_TO_SERVER_FAILED);
        
        // Liberamos recursos
        free(auth_msg);

        ////////////////////////////////////////////////////////////////////////
        //                                                                    //
        //                  LEEMOS LA RTA AL AUTH DEL SERVER                  //
        //                                                                    //
        ////////////////////////////////////////////////////////////////////////

        // Leemos la respuesta al greeting del server
        uint8_t auth_ans[1] = {0};
        n = recv(socket_fd, auth_ans, 1, 0);

        error_handler(n, "El servidor no respondio.\n", SERVER_NO_RESPONSE);
        auth_response_printer(auth_ans[0]);

        ////////////////////////////////////////////////////////////////////////
        //                                                                    //
        //                   ANALIZAMOS EL COMANDO A ENVIAR                   //
        //                                                                    //
        ////////////////////////////////////////////////////////////////////////
    
        char response[1000] = {0};
        char buffsize[1000] = {0};
        char c;
        char * req;
        int get_out = 0;
        print_menu();
        while (get_out == 0) {
            c=getchar();
            index = 0;
            switch (c) {
                case '1':
                    n = write(socket_fd, "1" , 1);
                    error_handler(n, "Error al enviar el comando ingresado.\n", WRITE_TO_SERVER_FAILED);
                    n = recv(socket_fd, response, 1000, 0);
                    error_handler(n, "El servidor no responde.\n", SERVER_NO_RESPONSE);
                    clrscr();
                    context("Metricas");
                    printf("%s\n",response);
                    wait_to_continue();
                    break;
                case '2':
                    clrscr();
                    context("Creacion de usuario");
                    request_credentials();
                    req = malloc(3 + strlen(usr) + strlen(pwd) + 1);
                    req[index++] = '2';
                    req[index++] = ' ';
                    for(size_t i = 0; i < strlen(usr); i++, index++)
                        req[index] = usr[i];
                    req[index++] = ' ';
                    for(size_t i = 0; i < strlen(pwd); i++,index++)
                        req[index] = pwd[i];
                    req[index++]='\n';
                    n = write(socket_fd, req, index);
                    error_handler(n, "No se pudieron enviar las credenciales del usuario.\n", WRITE_TO_SERVER_FAILED);
                    n = recv(socket_fd, response, 1000, 0);
                    error_handler(n, "El servidor no confirmo la creacion del usuario.\n", SERVER_NO_RESPONSE);
                    printf("%s\n",response);
                    wait_to_continue();
                    free(req);
                    break;
                case '3':
                    clrscr();
                    n = write(socket_fd, "3", 1);
                    error_handler(n, "No se pudo enviar el comando.", WRITE_TO_SERVER_FAILED);
                    n = recv(socket_fd, response, 1000, 0);
                    error_handler(n, "El servidor no responde.\n", SERVER_NO_RESPONSE);
                    context("Listado de usuarios");
                    printf("%s",response);
                    wait_to_continue();
                    break;
                case '4':
                    clrscr();
                    context("Modificar buffer");
                    index = 0;
                    printf("Ingrese el tamaño nuevo (bytes): ");
                    scanf("%s", buffsize);
                    char * req = malloc(4 + strlen(buffsize));
                    req[index++] = '4';
                    req[index++] = ' ';
                    for(size_t i = 0; i < strlen(buffsize); i++)
                        req[index++] = buffsize[i];
                    req[index++] = '\n';
                    req[index] = '\0';
                    n = write(socket_fd, req, index);
                    error_handler(n, "No se pudo enviar el comando de cambio de buffer.", WRITE_TO_SERVER_FAILED);
                    n = recv(socket_fd, response, 1000, 0);
                    error_handler(n, "El servidor no confirmo el cambio.\n", SERVER_NO_RESPONSE);
                    printf("%s\n",response);
                    wait_to_continue();
                    free(req);
                    break;
                
                case '5':
                    get_out = 1;
                    break;
                default:
                    context("Opcion invalida");
                    break;
            }
            clrscr();
            print_menu();
        }
    } else {
        return 0;
    }
    return 1;
}

void print_menu() {
    context("Sesion iniciada");
    printf("Bienvenido al menu de administrador, que desea hacer...\n");
    printf("  1. Obtener metricas.\n");
    printf("  2. Agregar un usuario.\n");
    printf("  3. Listar usuarios del proxy.\n");
    printf("  4. Cambiar buffer del sistema.\n");
    printf("  5. Salir.\n");
    printf("Ingrese alguna opcion: \n");
}

void wait_to_continue() {
    getchar();
    printf("Presione cualquier tecla para continuar.\n");
    getchar();
}

void error_handler(int n, char * msg, int err_num) {
    if( n <= 0 ) {
        printf("%s", msg);
        exit(err_num);
    }
}

void auth_response_printer(uint8_t c) {
    switch (c){
    case 0:
        clrscr();
        break;
    case 1:
        printf("Usuario o contraseña invalidos\n");
        exit(USR_OR_PWD_WRONG);
        break;
    case 2:
        printf("Mensaje mal formado\n");
        exit(BAD_SINTAX);
        break;
    default:
        break;
    }
}

void request_credentials() {
    printf("Usuario: ");
    scanf("%s", usr);
    getchar();
    printf("Contraseña: ");
    scanf("%s", pwd);
    getchar();
}

int verify_parameters(int argc, char ** argv) {

    if( argc != (MAX_NUMBER_OF_PARAMETERS + 1) ) {
        printf("Invalid number of arguments, given %d requiered %d.\n", argc-1, MAX_NUMBER_OF_PARAMETERS);
        return INVALID_NUMBER_OF_PARAMETERS;
    }

    // Nos guardamos los parametros que nos pasaron.
    strcpy(host, argv[1]);
    port_number = atoi(argv[2]);

    return 1;

}
