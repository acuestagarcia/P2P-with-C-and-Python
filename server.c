// Importamos el header
#include "server.h"

//Definir mutex, variable condicional y variable global de sincronización 'busy'
pthread_mutex_t mutex2;
pthread_cond_t cond;
int busy;
UserList userList = {NULL, PTHREAD_MUTEX_INITIALIZER};
PublicationList publicationList = {NULL, PTHREAD_MUTEX_INITIALIZER};

// Función ejecutado por cada hilo para atender petición del cliente
void * SendResponse(void * sc){
    int already_sent = 0;
    int s_local;
    int ret;
    s_local = (* (int *) sc);
    busy = 0;
    ParsedMessage parsedMessage;
    char buffer[256];

    // Recibir la acción a realizar
    if ((ret = parseMessage(s_local, &parsedMessage)) != 0){
        perror("SERVIDOR: un hilo no recibió la acción a realizar");
        pthread_exit(&ret);
    }
    printf("s> OPERATION %s FROM %s\n", parsedMessage.action, parsedMessage.UserName);
    // Procesar la solicitud
    if (strcmp(parsedMessage.action, "REGISTER") == 0) {
        
        if (parsedMessage.UserName == NULL) {
            perror("SERVIDOR: Username faltantes para REGISTER");
            ret = 2; // Error en la comunicación
        } else {
            int is_registered = is_user_registered(parsedMessage.UserName); 
            if (is_registered == 1) {
                ret = 1; // Usuario ya registrado
            } else if (is_registered == 0 && register_user(parsedMessage.UserName) == 0) {
                ret = 0; // Registro exitoso
            } else {
                ret = 2; // Error en el registro
            }
        }
    } else if (strcmp(parsedMessage.action, "UNREGISTER") == 0) {

        
        if (parsedMessage.UserName == NULL) {
            perror("SERVIDOR: Username faltantes para UNREGISTER");
            ret = 2; // Error en la comunicación
        }else if (!is_user_registered(parsedMessage.UserName)) {
            ret = 1; // Usuario no existe
        } else if (unregister_user(parsedMessage.UserName) == 0) {
            ret = 0; // Baja exitosa
        } else {
            ret = 2; // Error al eliminar
        }
    } else if (strcmp(parsedMessage.action, "CONNECT") == 0) {
        
        if (parsedMessage.UserName == NULL) {
            perror("SERVIDOR: Username faltantes para CONNECT");
            ret = 3; // Error en la comunicación
        } else {
            // Leer el puerto del cliente
            ssize_t bytesRead;
            memset(buffer, 0, sizeof(buffer));
            bytesRead = readLine(s_local, buffer, sizeof(buffer));
            if (bytesRead <= 0) {
                perror("SERVIDOR: Error al leer el puerto del cliente");
                ret = 3; // Error en la comunicación
            } else {
                int client_port = atoi(buffer);
                if (client_port < 1024 || client_port > 49151) {
                    perror("SERVIDOR: Puerto inválido recibido del cliente");
                    ret = 3; // Error en la comunicación
                } else if (!is_user_registered(parsedMessage.UserName)) {
                    ret = 1; // Usuario no existe
                } else if (is_user_connected(parsedMessage.UserName)) {
                    ret = 2; // Usuario ya conectado
                } else if (register_connection(parsedMessage.UserName, client_port, s_local) == 0) {
                    ret = 0; // Conexión exitosa
                } else {
                    ret = 3; // Error al registrar la conexión
                }
            }
        } 
    } else if (strcmp(parsedMessage.action, "PUBLISH") == 0) {
        
        if (parsedMessage.UserName == NULL) {
            perror("SERVIDOR: Username faltantes para PUBLISH");
            ret = 4; // Error en la comunicación
        } else {
            if (parsedMessage.argument1 == NULL || parsedMessage.argument2 == NULL) {
                perror("SERVIDOR: Error al leer el nombre y descripción del archivo");
                ret = 4; // Error en la comunicación
            } else {
                char file_name[256];
                snprintf(file_name, sizeof(file_name), "%s", parsedMessage.argument1);
                // Leer la descripción
                char description[256];
                strncpy(description, parsedMessage.argument2, sizeof(description));
                // Verificar si el usuario existe
                if (!is_user_registered(parsedMessage.UserName)) {
                    ret = 1; // Usuario no existe
                } else if (!is_user_connected(parsedMessage.UserName)) {
                    ret = 2; // Usuario no conectado
                } else if (is_file_published(file_name)) {
                    ret = 3; // Archivo ya publicado
                } else if (register_publication(parsedMessage.UserName, file_name, description) == 0) {
                    ret = 0; // Publicación exitosa
                } else {
                    ret = 4; // Error al registrar la publicación
                }
            }
        }
    } else if (strcmp(parsedMessage.action, "DELETE") == 0) {
            
        if (parsedMessage.UserName == NULL) {
            perror("SERVIDOR: Username faltantes para DELETE");
            ret = 4; // Error en la comunicación
        } else {
            if (parsedMessage.argument1 == NULL) {
                perror("SERVIDOR: Error al leer el nombre del archivo");
                ret = 4; // Error en la comunicación
            } else {
                char file_name[256];
                snprintf(file_name, sizeof(file_name), "%s", parsedMessage.argument1);
                if (!is_user_registered(parsedMessage.UserName)) {
                    ret = 1; // Usuario no existe
                } else if (!is_user_connected(parsedMessage.UserName)) {
                    ret = 2; // Usuario no conectado
                } else if (!is_file_published(file_name)) {
                    ret = 3; // Archivo no publicado
                } else if (delete_publication(file_name) == 0) {
                    ret = 0; // Eliminación exitosa
                } else {
                    ret = 4; // Error al eliminar la publicación
                }
            }
        }
    } else if (strcmp(parsedMessage.action, "LIST_USERS") == 0) {
        
        if (parsedMessage.UserName == NULL) {
            perror("SERVIDOR: Username faltantes para LIST_USERS");
            ret = 3; // Error en la comunicación
        } else if (!is_user_registered(parsedMessage.UserName)) {
            ret = 1; // Usuario no existe
        } else if (!is_user_connected(parsedMessage.UserName)) {
            ret = 2; // Usuario no conectado
        } else {
            // Enviar el código de éxito (0)
            if (sendByte(s_local, 0) != 0) {
                perror("SERVIDOR: Error al enviar el código de éxito");
                ret = 3; // Error en la comunicación
                }
            already_sent = 1;
            char user_list[1024];
            memset(user_list, 0, sizeof(user_list));
            int num_users = get_ListUsers(user_list, sizeof(user_list));

            // Enviar el número de usuarios conectados
            snprintf(buffer, sizeof(buffer), "%d", num_users);
            if (sendMessage(s_local, buffer, strlen(buffer) + 1) != 0) {
                perror("SERVIDOR: Error al enviar el número de usuarios");
                ret = 3; // Error en la comunicación
            } else {
                // Dividir la lista de usuarios en tokens y enviar cada campo uno a uno
                char *user_info = strtok(user_list, "\n");
                while (user_info != NULL) {
                    // Dividir la información del usuario en nombre, IP y puerto
                    char username[256], ip_address[INET_ADDRSTRLEN];
                    int port;
                    sscanf(user_info, "%s %s %d", username, ip_address, &port);

                    // Enviar el nombre de usuario
                    
                    if (sendMessage(s_local, username, strlen(username) + 1) != 0) {
                        perror("SERVIDOR: Error al enviar el nombre de usuario");
                        ret = 3; 
                        break;
                    }

                    // Enviar la dirección IP
                    if (sendMessage(s_local, ip_address, strlen(ip_address) + 1) != 0) {
                        perror("SERVIDOR: Error al enviar la dirección IP");
                        ret = 3; 
                        break;
                    }

                    // Enviar el puerto
                    snprintf(buffer, sizeof(buffer), "%d", port);
                    if (sendMessage(s_local, buffer, strlen(buffer) + 1) != 0) {
                        perror("SERVIDOR: Error al enviar el puerto");
                        ret = 3; // Error en la comunicación
                        break;
                    }

                    // Obtener el siguiente usuario
                    user_info = strtok(NULL, "\n");
                }

                if (ret != 3) {
                    ret = 0; // Operación exitosa
                }
            }
        }
    } else if (strcmp(parsedMessage.action, "LIST_CONTENT") == 0){
        
    if (parsedMessage.UserName == NULL || parsedMessage.argument1 == NULL) {
        perror("SERVIDOR: Faltan argumentos para LIST_CONTENT");
        ret = 4; // Error en la comunicación
    } else if (is_user_registered(parsedMessage.UserName) == 0) {
        ret = 1; // Usuario que realiza la operación no existe
    } else if (is_user_connected(parsedMessage.UserName) == 0) {
        ret = 2; // Usuario que realiza la operación no está conectado
    } else if (!is_user_registered(parsedMessage.argument1)) {
        ret = 3; // Usuario cuyo contenido se quiere conocer no existe
    } else {
        // Obtener la lista de publicaciones del usuario objetivo
        char publication_list[1024];
        memset(publication_list, 0, sizeof(publication_list));
        int num_files = get_publications(parsedMessage.argument1, publication_list, sizeof(publication_list));

        // Enviar el código de éxito (0)
        if (sendByte(s_local, 0) != 0) {
            perror("SERVIDOR: Error al enviar el código de éxito");
            ret = 4; // Error en la comunicación
        } else {
            // Enviar el número de publicaciones
            snprintf(buffer, sizeof(buffer), "%d", num_files);
            if (sendMessage(s_local, buffer, strlen(buffer) + 1) != 0) {
                perror("SERVIDOR: Error al enviar el número de publicaciones");
                ret = 4; // Error en la comunicación
            } else {
                // Enviar la lista de publicaciones
                // Dividir la lista de publicaciones en tokens y enviar cada fichero uno a uno
                char *file_info = strtok(publication_list, "\n");
                while (file_info != NULL) {
                if (sendMessage(s_local, file_info, strlen(file_info) + 1) != 0) {
                    perror("SERVIDOR: Error al enviar la información del fichero");
                    ret = 4; // Error en la comunicación
                    break;
                }
                file_info = strtok(NULL, "\n"); // Obtener el siguiente fichero
                }

                if (ret != 4) {
                ret = 0; 
                    }
                }  
            }
        } 
    }else if (strcmp(parsedMessage.action, "DISCONNECT") == 0) {
            
        if (parsedMessage.UserName == NULL) {
            perror("SERVIDOR: Username faltantes para DISCONNECT");
            ret = 3; // Error en la comunicación
        } else if (!is_user_registered(parsedMessage.UserName)) {
            ret = 1; // Usuario no existe
        } else if (!is_user_connected(parsedMessage.UserName)) {
            ret = 2; // Usuario no conectado
        } else {
            // Desconectar al usuario
            if (unregister_connection(parsedMessage.UserName) == 0) {
                ret = 0; // Desconexión exitosa
            } else {
                ret = 3; // Error al desconectar
                }
            }
        } else {
        perror("SERVIDOR: No existe la acción requerida");
        ret = ERROR_COMMUNICATION;
            }

    // Enviar respuesta al cliente
    if (already_sent == 0){
        if (sendByte(s_local, ret) != 0) {
            perror("SERVIDOR: Error al enviar el resultado al cliente");
        }
    }
    // Liberar recursos
    freeParsedMessage(&parsedMessage);
    close(s_local);
    pthread_exit(&ret);
}


int main(int argc, char * argv[]) {

    if (argc != 3 || strcmp(argv[1], "-p") != 0) {
        printf("Uso: ./servidor -p <port>\n");
        goto cleanup_servidor;
    }

    
    char *endptr;
    long port = strtol(argv[2], &endptr, 10);
    if (*endptr != '\0' || port < 1024 || port > 49151) {
        perror("SERVIDOR: debe usar un puerto registrado\n");
        goto cleanup_servidor;
    }

    struct sockaddr_in server_addr, client_addr;
    socklen_t size;
    int ss, sc;

    // Crear socket del servidor
    if ((ss = socket (AF_INET, SOCK_STREAM, 0) ) < 0) { 
        perror("SERVIDOR: Error en el socket\n");
        goto cleanup_servidor;
    }
    int val = 1;
    if (setsockopt(ss, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) != 0){
        perror("SERVIDOR: Error al configurar el socket\n");
        goto cleanup_servidor;
    }

    // Asignar dirección IP y puerto
    memset((char *)&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(ss, (const struct sockaddr *) &server_addr, sizeof(server_addr)) != 0){
        perror("SERVIDOR: Error al asignar dirección al socket\n");
        goto cleanup_servidor;
    }
    // Escuchar conexiones entrantes
    if (listen(ss, SOMAXCONN) != 0){
        perror("SERVIDOR: Error al habilitar el socket para recibir conexiones\n");
        goto cleanup_servidor;
    }

    // Obtener la IP local
    struct sockaddr_in local_addr;
    socklen_t addr_len = sizeof(local_addr);
    if (getsockname(ss, (struct sockaddr *)&local_addr, &addr_len) == -1) {
        perror("SERVIDOR: Error al obtener la dirección local\n");
        close(ss);
        return -1;
    }
    char local_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &local_addr.sin_addr, local_ip, INET_ADDRSTRLEN);
    // Mostrar mensaje de inicio
    printf("s> init server %s:%ld\ns>\n", local_ip, port);
    fflush(stdout);
    // Inicializar variable global de control (busy)
    busy = 1;


    // Crear atributo de pthread DETACHED    
    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);   

    size = sizeof(client_addr);

    initializeUserList();

    // Bucle infinito para manejar las solicitudes
    while (1) {
        // Aceptar conexión del cliente
        if ((sc = accept(ss, (struct sockaddr *) &client_addr, &size)) < 0){
            perror("SERVIDOR: Error al tratar de aceptar conexión\n");
            goto cleanup_servidor;
        }

        //Crea hilo para manejar la conexión
        pthread_t thread_id;
        if (pthread_create(&thread_id, &thread_attr, SendResponse, (void*) &sc) != 0) {
            perror("SERVIDOR: Error al crear el hilo\n");
            goto cleanup_servidor;
        }

    }
    
    return 0;

    cleanup_servidor:
        // Cerrar el socket del servidor
        close(ss);
        goto cleanup_servidor;
        free_user_list();
        return -1; // Indicar error
}

