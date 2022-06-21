# PDC-TPE-SOCKSv5

## Installation Guide
---

The project has a Makefile in order to run the compilation. To compile the code you need to be in the project's root folder and run the following command:

```bash
make all
```

This command will generate inside the root folder, the two executable of the project:

>**socks5d** -> Executable to run the SOCKSv5 Proxy Server

>**socks5_clt** -> Executable to run a client that connects with the Monitoring Server

## Usage Guide
---
### SOCKSv5 Proxy Server

1. To run the server the following command must be use:

```bash
./socks5d <parameters>
```

2. The parameters that can be used when running the server are:

    **```-h```** -> Imprime guía para ejecutar el servidor.

    **```-l SOCKSv5_addr```** -> Dirección donde servirá el servidor proxy SOCKSv5.

    **```-L monit_addr```** -> Dirección donde servirá el servidor de monitoreo y control.

    **```-p SOCKSv5_port```** -> Puerto entrante para conexiones al servidor proxy SOCKSv5.

    **```-P monit_port```** -> Puerto entrante para conexiones al servidor de monitoreo y control.

    **```-v```** -> Imprime versión del servidor proxy SOCKSv5.
    
    **```-u <username>:<password>```** -> Registra dicho usuario para luego conectarse al servidor.

    **```-U <admin_name>:<admin_pass>```** -> Registra al usuario admin para acceder al servidor de monitoreo.

    **```-N```** -> Deshabilitas los disectores para sniffin POP3.

---
### Control and Monitoring Server

1. To run the server the following command must be use:

    ```bash
    ./socks5_clt <proxy_server_ip> <proxy_server_port>
    ```

2. After running the executable file you must authenticate with the admin credentials, by writing the following command:
    ```
    <admin_name>:<admin_pass>\n
    ```
3. After a succesfull authentication you can select from a list of methods which you want to run. The format for each method is the following:
    ```
    Get server metrics -> 1\n
    Add new user -> 2 <username> <password>\n
    List all users -> 3\n
    Change buff size -> 4 <new_buff_size>\n
    ```

