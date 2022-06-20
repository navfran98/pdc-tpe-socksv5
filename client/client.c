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

// print metricas -> 1
// agregar usuario -> 2 [usuario] [contraseña]
// list users -> 3 
// change buffer size --> 4 [buffsize en bytes]

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
// Este codigo correrlo en otra terminal, pasarle por entrada estandar:
//
// ./client <DONDE TENGO EL SERVER CORRIENDO> <PUERTO>
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

        // Mandamos el greeting/auth del admin
        request_credentials();
        size_t index = 0;
        int n = 0, usr_len = strlen(usr), pass_len = strlen(pwd);
        // las 2 strlen del usr y pass + la usr y el pass
        uint8_t * auth_msg = malloc(2 + usr_len + pass_len);
        auth_msg[index++] = usr_len;
        for(int i = 0;i < usr_len;i++) {
            auth_msg[index] = usr[i];
            index++;
        }
        auth_msg[index++] = pass_len;
        for(int i = 0;i < pass_len;i++) {
            auth_msg[index] = pwd[i];
            index++;
        }
        //printf("%s", auth_msg);
        n = write(socket_fd, auth_msg, index);
        if( n < 0 ) {
            fprintf(stderr, "Coudn't write to server.");
            exit(WRITE_TO_SERVER_FAILED);
        }else{
            printf("Le mando al proxy la auth siendo el admin\n");
        }
        free(auth_msg);
        // Leemos la respuesta al greeting del server
        uint8_t auth_ans[1];
        bzero(auth_ans, 1);
        n = recv(socket_fd, auth_ans, 1, 0);
        if(n < 0){
            fprintf(stderr, "Coudn't receive from server.");
            exit(WRITE_TO_SERVER_FAILED);
        }else{
            switch (auth_ans[0]){
            case 0:
                printf("ENTRASTE MOSTRO\n");
                break;
            case 1:
                printf("TE LA DISTE CONTRA LA PUERTA\n");
                break;
            case 2:
                printf("TODO MAL CON VOS PETE\n");
                break;
            default:
                break;
            }
        }
        if(auth_ans[0] == 0){
            char response[1000] = {0};
            char buffsize[1000] = {0};
            char c;
            //  char * k = malloc(2 * sizeof(char));
            //  sprintf(k, "1");
            char * req;
            print_menu();
            int get_out = 0;
            while (get_out == 0) {
                c=getchar();
                if(c != '\n'){
                    if (getchar() == '\n'){
                        index = 0;
                        switch (c) {
                            case '1':
                                n = write(socket_fd, "1" , 1);
                                if(n == 0){
                                    printf("NO ENVIE UN CARAJO\n");
                                }
                                if( n < 0 ) {
                                    printf("ERROR\n");
                                    fprintf(stderr, "Couldn't write to server 1.");
                                    exit(WRITE_TO_SERVER_FAILED);
                                }else{
                                    printf("Pidiendo metricas...\n");
                                }
                                n = recv(socket_fd, response, 1000, 0);
                                if(n < 0){
                                    fprintf(stderr, "Couldn't receive from server.");
                                    exit(WRITE_TO_SERVER_FAILED);
                                }else{
                                    printf("%s\n",response);
                                }
                                break;
                            case '2':
                                printf("Creando nuevo usuario, ingrese:\n");
                                request_credentials();
                                req = malloc(3 + strlen(usr) + strlen(pwd) + 1);
                                req[index++] = '2';
                                req[index++] = ' ';
                                for(int i = 0; i < strlen(usr); i++) {
                                    req[index] = usr[i];
                                    index++;
                                }
                                req[index++] = ' ';
                                for(int i = 0; i < strlen(pwd); i++) {
                                    req[index] = pwd[i];
                                    index++;
                                }
                                req[index++]='\n';
                                n = write(socket_fd, req, index);

                                n = recv(socket_fd, response, 1000, 0);
                                if(n < 0){
                                    fprintf(stderr, "Couldn't receive from server.");
                                    exit(WRITE_TO_SERVER_FAILED);
                                }else{
                                    printf("%s\n",response);
                                }
                                free(req);
                                break;
                            case '3':
                                n = write(socket_fd, "3", 1);
                                if( n < 0 ) {
                                    fprintf(stderr, "Couldn't write to server 3.");
                                    exit(WRITE_TO_SERVER_FAILED);
                                }else{
                                    printf("Listando usuarios...\n");
                                }
                                n = recv(socket_fd, response, 1000, 0);
                                if(n < 0){
                                    fprintf(stderr, "Couldn't receive from server.");
                                    exit(WRITE_TO_SERVER_FAILED);
                                }else{
                                    printf("%s\n",response);
                                }
                                break;
                            case '4':
                                printf("Nuevo tamanio de buffer:");
                                index = 0;
                                scanf("%s", buffsize);
                                char * req = malloc(4 + strlen(buffsize));
                                req[index++] = '4';
                                req[index++] = ' ';
                                for(ssize_t i = 0; i < strlen(buffsize); i++){
                                    req[index++] = buffsize[i];
                                }
                                req[index++] = '\n';
                                req[index] = '\0';
                                n = write(socket_fd, req, index);
                                n = recv(socket_fd, response, 1000, 0);
                                if(n < 0){
                                    fprintf(stderr, "Couldn't receive from server.");
                                    exit(WRITE_TO_SERVER_FAILED);
                                }else{
                                    printf("%s\n",response);
                                }
                                free(req);
                                break;
                            
                            case '5':
                                get_out = 1;
                                break;
                        }
                    } else {
                        while(getchar() != '\n');
                        printf("Invalid command\n");
                    }
                }
            }
        } else if( auth_ans[0] == 1 ) {
            fprintf(stderr, "Wrong credentials.");
        } else if( auth_ans[0] == 2 ){
            fprintf(stderr, "Bad sintax.");
        } else {
            fprintf(stderr, "Bug!! -->  Se recibio: %u.", auth_ans[0]);
        }
    } else {
        return 0;
    }
    return 1;
}

void print_menu() {
    printf("Bienvenido al menu de administrador, que desea hacer...\n");
    printf("\t 1. Obtener metricas.\n");
    printf("\t 2. Agregar un usuario.\n");
    printf("\t 3. Listar usuarios del proxy.\n");
    printf("\t 4. Cambiar buffer del sistema.\n");
    printf("\t 5. Salir.\n");
    printf("Ingrese alguna opcion: \n");
}

void request_credentials() {
    printf("Usuario: ");
    scanf("%s", usr);
    printf("Contrasenia: ");
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

    return 1;

}

void obtener_metricas(int fd) {

}
