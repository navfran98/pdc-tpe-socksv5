#include <stdio.h>
#include <netdb.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdbool.h>

// CONFIG
#define MAX_NUMBER_OF_PARAMETERS 2
#define ANS_BUFF_SIZE 10000
#define clrscr() printf("\033[1;1H\033[2J")
#define context(x) printf("\033[0;32m");printf("[ %s ]\n", x);printf("\033[0m");
#define CREDENTIALS_LEN 100
#define REQUEST_LEN 255

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
char usr  [CREDENTIALS_LEN] = {0};
char pwd  [CREDENTIALS_LEN] = {0};
char host [CREDENTIALS_LEN] = {0};
int port_number = 0;
int auth_required = 0;

// METHODS
int verify_parameters(int argc, char ** argv);
void send_greeting_to_server();
void print_menu();
void obtener_metricas();
void error_handler(int n, char * msg, int err_num);
void auth_response_printer(uint8_t c);

// Este codigo correrlo en otra terminal, pasarle por entrada estandar:
//
// ./client <DONDE TENGO EL SERVER CORRIENDO> <PUERTO>
//
// Una vez que ingresamos el usr o la contrasenia elegir que hacer

int splitstr(char * str,int len, char delim, char * str1, char * str2){
    int flag = 0;
    int j = 0;
    for(int i=0; i < len; i++){
        if(flag == 0){
            if(str[i] == delim)
                flag++;
            else
                str1[i] = str[i];
        }else{
            str2[j++] = str[i];
        } 
    }
    return flag; 
}

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
        memcpy( &addr.sin_addr.s_addr, server->h_addr_list[0],server->h_length);
        // Le pasamos el numero de puerto
        addr.sin_port = htons(port_number);
        // Nos conectamos con el servidor
        if( connect(socket_fd, (struct sockaddr * )&addr, sizeof(addr)) < 0 ) {
            fprintf(stderr, "Couldn't connect to server.\n");
            exit(IMPOSIBLE_TO_CONNECT);
        }

        ////////////////////////////////////////////////////////////////////////
        //                                                                    //
        //                 AUTENTICACION DEL ADMINISTRADOR                    //
        //                                                                    //
        ////////////////////////////////////////////////////////////////////////
        printf("Ingresa las credenciales de usuario -> [username]:[password]\n");
        int auth_index = 0;
        char cc;
        char * auth_input = malloc(CREDENTIALS_LEN + 2);
        while ((cc = getchar()) != '\n') {
            if(auth_index >= CREDENTIALS_LEN){
                fprintf(stderr, "Credentials must have less than 100 characters\n");
                exit(1);
            }
            else {
                auth_input[auth_index++] = cc;
            }
        }
        auth_input[auth_index++] = '\n';
        auth_input[auth_index] = '\0';

        splitstr(auth_input, strlen(auth_input), ':',usr, pwd);

        char * credentials = malloc(CREDENTIALS_LEN);
        int to_send_index = 0;
        credentials[to_send_index++] = strlen(usr);
        if(strlen(usr) == 0 || strlen(pwd) == 0){
            fprintf(stderr, "Invalidad credentials format\n");
            exit(1);
        }
        for(size_t i = 0;i<strlen(usr); i++, to_send_index++){
            credentials[to_send_index]=usr[i];
        }
        credentials[to_send_index++] = strlen(pwd)-1;
        for(size_t i=0;i<strlen(pwd); i++, to_send_index++){
            credentials[to_send_index]=pwd[i];
        }

        int n = write(socket_fd, credentials, to_send_index);
        // Validamos la respuesta
        error_handler(n, "Error al enviar las credenciales.\n", WRITE_TO_SERVER_FAILED);

        ////////////////////////////////////////////////////////////////////////
        //                                                                    //
        //                  LEEMOS LA RTA AL AUTH DEL SERVER                  //
        //                                                                    //
        ////////////////////////////////////////////////////////////////////////

        // Leemos la respuesta al greeting del server
        uint8_t auth_ans[1] = {0};
        n = recv(socket_fd, auth_ans, 1, 0);

        error_handler(n, "El servidor no envio una respuesta.\n", SERVER_NO_RESPONSE);
        auth_response_printer(auth_ans[0]);
        
        // Liberamos recursos
        free(credentials);
        free(auth_input);

        ////////////////////////////////////////////////////////////////////////
        //                                                                    //
        //                   ANALIZAMOS EL COMANDO A ENVIAR                   //
        //                                                                    //
        ////////////////////////////////////////////////////////////////////////
    
        char response[1000] = {0};
        char c;
        char * req;
        int get_out = 0;
        print_menu();
        req = malloc(REQUEST_LEN + 2);
        while (get_out == 0) {
            bool first = true;
            int index = 0;
            while (get_out != 1 && (c = getchar()) != '\n') {
                if(c == '5' && first == true ){
                    if((c = getchar()) == '\n'){
                        get_out = 1;
                    }
                }
                else if(index >= REQUEST_LEN){
                    fprintf(stderr, "The request must have less than 255 characters\n");
                    exit(1);
                }else{
                    req[index++] = c;
                }
                first = false;
            }
            if(get_out != 1){
                req[index++] = '\n';
                int nn = write(socket_fd, req, index);
                error_handler(nn, "No se pudo enviar la request.\n", WRITE_TO_SERVER_FAILED);
                nn = recv(socket_fd, response, 1000, 0);
                error_handler(nn, "El servidor no envio una respuesta.\n", SERVER_NO_RESPONSE);
                printf("%s\n",response);
            }
        }
        free(req);
    }
    return 1;
}

void print_menu() {
    context("Sesion iniciada");
    printf("Bienvenido al servidor de control y monitoreo\n");
    printf("  Obtener metricas -> 1\n");
    printf("  Agregar un usuario -> 2 [user] [pass]\n");
    printf("  Listar usuarios del proxy -> 3\n");
    printf("  Cambiar buffer del sistema -> 4 [buff_size]\n");
    printf("  Salir -> 5\n");
    printf("Ingrese alguna opcion: \n");
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