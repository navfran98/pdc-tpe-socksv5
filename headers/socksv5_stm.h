#ifndef PROTOS2022A_SOCKSTM
#define PROTOS2022A_SOCKSTM


#include "../headers/stm.h"

// Estados
// 1. Client Greeting
// 2. Server greeting analysis (si la VER != 5 chau, si nos piden otra cosa que no sea U/P chau)
//  . Method selection msg (en nuestro caso le decimos que usamos U/P)
// 3. Client auth request msg (nos envian las credenciales de usr)
// 4. Analisamos si es correcta el usr y la contra
//  . Server greeting response (OK ==> se conecta / NOK ==> chau conexion)

/////////////////////////////////////////////////////////////////////////
//                           GREETING_READ                             //
/////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////
//                           GREETING_WRITE                            //
/////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////
//                             AUTH_READ                               //
/////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////
//                             AUTH_WRITE                              //
/////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////
//                                DONE                                 //
/////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////
//                         ERROR_GLOBAL_STATE                          //
/////////////////////////////////////////////////////////////////////////


enum socksv5_global_state {

    ///////////////////////////////////////////////////////////////////
    // Se lee el saludo que le manda el cliente al servidor, se en-  //
    // cuentra formado por:                                          //
    // +---------+---------+---------+ VER = version del protocolo   //
    // |   VER   | NMETHOD | METHODS | NME = lenght de METHODS       //
    // +---------+---------+---------+ MET = metodos de auth pedidos //
    ///////////////////////////////////////////////////////////////////
    GREETING_READ = 0,
    
    ///////////////////////////////////////////////////////////////////////
    // Una vez leido lo que nos mando el cliente le respondemos con que  //
    // metodo de los solicitados por el vamos a trabajar, en caso de que //
    // no soportemos ninguno devolvemos 0xFF y EL CLIENTE se debera des- //
    // conectar. Formato:                                                //
    // +---------+---------+ VER = misma que antes.                      //
    // |   VER   |  METHOD | MET = metodo seleccionado o ninguno.        //
    // +---------+---------+                                             //
    ///////////////////////////////////////////////////////////////////////
    GREETING_WRITE,

    ////////////////////////////////////////////////////////////////////////
    // Saltear los 2 siguientes estados si no se va a usar autenticacion. //
    ////////////////////////////////////////////////////////////////////////
 
    ////////////////////////////////////////////////////////////////////////
    // Una vez que el usuario selecciona autenticacion con usuario y con- //
    // trasenia, se las manda al proxy para que haga las correspondientes //
    // validaciones. Formato:                                             //
    //         +---------+---------+---------+---------+---------+        //
    //         |   VER   |   ULN   |   USR   |   PLN   |   PWD   |        //
    //         +---------+---------+---------+---------+---------+        //
    // Donde:                                                             //
    //   - VER = version de la negociacion, es 0x01.                      //
    //   - ULN = longitud del nombre de usuario.                          //
    //   - USR = nombre de usuario.                                       //
    //   - PLN = longitud de la contrasenia.                              //
    //   - PWD = contrasenia.                                             //
    ////////////////////////////////////////////////////////////////////////
    AUTH_READ,

    ////////////////////////////////////////////////////////////////////////
    // El servidor recibe el usuario y la contrasenia del cliente, las va //
    // lida y le devuelve una respuesta al usuario. Formato:              //
    // +---------+---------+ VER = misma que antes.                       //
    // |   VER   |  STATUS | STA = 0x00 validado / cualquier valor error  //
    // +---------+---------+       y EL CLIENTE se debera desconectar.    //
    ////////////////////////////////////////////////////////////////////////
    AUTH_WRITE,

    /////////////////////////////////////////////////////////////////////////
    // A partir de aca el cliente esta formalmente conectado, entonces nos //
    // va a mandar una request que tiene el siguiente formato:             //
    //    +---------+---------+---------+---------+---------+----------+   //
    //    |   VER   |   CMD   |   RSV   |   ATYP  | DST.ADD | DST.PORT |   //
    //    +---------+---------+---------+---------+---------+----------+   //
    // Donde:                                                              //
    //   - VER  = version del protocolo, 0x05.                             //
    //   - CMD  = comando a realizar, CONNECT (0x01), BIND (0x02) o        //
    //            UDP  ASSOCIATE (0x03).                                   //
    //   - RSV  = reservado.                                               //
    //   - ATYP = tipo de addr, 0x01(IPv4), 0x03(name) o 0x04(IPv6).       //
    //   - D.AD = direccion IP (4 o 6) o domain name.                      //
    //   - D.PT = puerto al cual conectarse.                               //
    // Segun el tipo de request se deberan responder 1 o mas responses. Si //
    // elegimos pasarle un domain name el perimer octeto es el largo del   //
    // domain name.                                                        //
    // Los campos marcados como RSV deben ser seteados en 0x00. Esto lo de //
    // cia en la parte de responses asumo que aca es igual pq no lo usamos //
    /////////////////////////////////////////////////////////////////////////
    REQUEST_READ,

    /////////////////////////////////////////////////////////////////////////
    // El servidor le responde la request al cliente con un mensaje que ti //
    // ene el siguiente formato:                                           //
    //    +---------+---------+---------+---------+---------+----------+   //
    //    |   VER   |   REP   |   RSV   |   ATYP  | BND.ADD | BND.PORT |   //
    //    +---------+---------+---------+---------+---------+----------+   //
    // Donde:                                                              //
    //   - VER  = version del protocolo, 0x05.                             //
    //   - CMD  = tipo de response, 0 exito, 1 server error, 2 connection  //
    //            not allowed by ruleset, 3 network unreachable, 4 host    //
    //            unreachable, 5 connection refueses, 6 TTL expired, 7 co  //
    //            mmand not suported, 8 address type not suported, de aca  //
    //            en adelante pueden ser errores nuestros (de 9 a FF).     //
    //   - RSV  = reservado.                                               //
    //   - ATYP = tipo de addr, 0x01(IPv4), 0x03(name) o 0x04(IPv6).       //
    //   - B.AD = server bound address.                                    //
    //   - B.PT = server bound port                                        //
    // Los campos marcados como RSV deben ser seteados en 0x00. En el caso //
    // de que la respuesta del servidor no sea 0x00 (exito) el servidor le //
    // manda la response al cliente y EL SERVIDOR lo desconecta. Si el cli //
    // ente nos manda una reques de tipo CONNECT o BIND hay que prepararno //
    // para empezar a recibir datos por parte del cliente.                 //
    /////////////////////////////////////////////////////////////////////////
    REQUEST_WRITE,

    ORIGIN_CONNECT,

    RESPONSE_READ,

    RESPONSE_WRITE,

    
    DONE,
    ERROR_GLOBAL_STATE
};

const struct state_definition * socksv5_describe_states(void);


#endif
