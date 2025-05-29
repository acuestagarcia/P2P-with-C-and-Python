#include "server.h"

// Definir funciones necesarias para decodificar mensajes 

// Función para recibir y separar acción y argumentos
int parseMessage(int socket, ParsedMessage *parsedMessage){
    char buffer[256];
    ssize_t bytesRead;

    // Leer la primera cadena (acción)
    bytesRead = readLine(socket, buffer, sizeof(buffer));
    if (bytesRead <= 0) {
        perror("Error al leer el mensaje desde el socket");
        return ERROR_COMMUNICATION;
    }

    // Validar que el mensaje tenga al menos un carácter para la acción
    if (bytesRead < 1 || !isprint(buffer[0])) {
        perror("Mensaje inválido: no contiene una acción válida");
        return ERROR_COMMUNICATION;
    }

    // Asignar la acción
    parsedMessage->action = strdup(buffer); // Copiar la acción
    //printf("UserName: %s\n", parsedMessage->UserName);
    if (parsedMessage->action == NULL) {
        perror("Error al asignar memoria para la acción");
        return ERROR_COMMUNICATION;
    }

    // Leer la fecha de servicio web
    bytesRead = readLine(socket, buffer, sizeof(buffer));
    if (bytesRead <= 0) {
        perror("Error al leer el mensaje desde el socket");
        return ERROR_COMMUNICATION;
    }

    // Validar que el mensaje tenga al menos un carácter para la fecha
    if (bytesRead < 1 || !isprint(buffer[0])) {
        perror("Mensaje inválido: no contiene una acción válida");
        return ERROR_COMMUNICATION;
    }

     // Asignar la fecha
     parsedMessage->fecha = strdup(buffer); // Copiar la fecha
     if (parsedMessage->fecha == NULL) {
         perror("Error al asignar memoria para la acción");
         return ERROR_COMMUNICATION;
     }


    // Leer el nombre de usuario
    bytesRead = readLine(socket, buffer, sizeof(buffer));
    if (bytesRead < 0) {
        perror("Error al leer los argumentos desde el socket");
        free(parsedMessage->action); // Liberar memoria en caso de error
        return ERROR_COMMUNICATION;
    }

    parsedMessage->UserName = strdup(buffer); // Copiar los argumentos
    if (parsedMessage->UserName == NULL) {
        perror("Error al asignar memoria para los argumentos");
        free(parsedMessage->action); // Liberar memoria en caso de error
        return ERROR_COMMUNICATION;
    }

    if (strcmp(parsedMessage->action, "REGISTER") == 0 || strcmp(parsedMessage->action, "UNREGISTER") == 0
    || strcmp(parsedMessage->action, "CONNECT") == 0 || strcmp(parsedMessage->action, "LIST_USERS") == 0 
    || strcmp(parsedMessage->action, "DISCONNECT") == 0) {
        parsedMessage->argument1 = NULL; // No hay argumentos adicionales
        parsedMessage->argument2 = NULL;
        return 0;
    }
   
   // Leer otros argumento para DELETE, PUBLISH y LIST_CONTENT
    bytesRead = readLine(socket, buffer, sizeof(buffer));
    if (bytesRead < 0) {
        perror("Error al leer los argumentos desde el socket");
        free(parsedMessage->action); // Liberar memoria en caso de error
        free(parsedMessage->UserName); 
        return ERROR_COMMUNICATION;
    }
    parsedMessage->argument1 = strdup(buffer); // Copiar los argumentos
    if (parsedMessage->argument1 == NULL) {
        perror("Error al asignar memoria para los argumentos");
        free(parsedMessage->action); // Liberar memoria en caso de error
        free(parsedMessage->UserName);
        return ERROR_COMMUNICATION;
    }

    if (strcmp(parsedMessage->action, "DELETE") == 0 || strcmp(parsedMessage->action, "LIST_CONTENT") == 0) {
        parsedMessage->argument2 = NULL; // No hay argumentos adicionales
        return 0;
    } 

    // Leer otros argumento para PUBLISH
    
    bytesRead = readLine(socket, buffer, sizeof(buffer));
    if (bytesRead < 0) {
        perror("Error al leer los argumentos desde el socket");
        free(parsedMessage->action); // Liberar memoria en caso de error
        free(parsedMessage->UserName);
        free(parsedMessage->argument1); 
        return ERROR_COMMUNICATION;
    }
    parsedMessage->argument2 = strdup(buffer); // Copiar los argumentos


    if (parsedMessage->argument2 == NULL) {
        perror("Error al asignar memoria para los argumentos");
        free(parsedMessage->action); // Liberar memoria en caso de error
        free(parsedMessage->UserName);
        free(parsedMessage->argument1);
        return ERROR_COMMUNICATION;
    }

        return 0;
}


// Función para liberar la memoria de ParsedMessage
void freeParsedMessage(ParsedMessage *parsedMessage) {
    if (parsedMessage->action != NULL) {
        free(parsedMessage->action);
        parsedMessage->action = NULL;
    }
    if (parsedMessage->UserName != NULL) {
        free(parsedMessage->UserName);
        parsedMessage->UserName = NULL;
    }
    if (parsedMessage->argument1 != NULL) {
        free(parsedMessage->argument1);
        parsedMessage->argument1 = NULL;
    }
    if (parsedMessage->argument2 != NULL) {
        free(parsedMessage->argument2);
        parsedMessage->argument2 = NULL;
    }
}

void initializeUserList() {
    userList.head = NULL;
    pthread_mutex_init(&userList.mutex, NULL);
}


// Función para verificar si un usuario ya está registrado
int is_user_registered(const char *username) {
    pthread_mutex_lock(&userList.mutex);
    UserNode *current = userList.head;
    while (current != NULL) {
        if (strcmp(current->username, username) == 0) {
            pthread_mutex_unlock(&userList.mutex);
            return 1; // Usuario encontrado
        }
        current = current->next;
    }
    pthread_mutex_unlock(&userList.mutex);
    return 0; // Usuario no encontrado
}

// Función para registrar un usuario
int register_user(const char *username) {

    // Crear un nuevo nodo
    UserNode *newNode = (UserNode *)malloc(sizeof(UserNode));
    if (newNode == NULL) {
        perror("Error al asignar memoria para el nuevo usuario");
        return -1; // Error de memoria
    }
    strncpy(newNode->username, username, sizeof(newNode->username) - 1);
    newNode->username[sizeof(newNode->username) - 1] = '\0';
    newNode->next = NULL;

    // Agregar el nodo a la lista
    pthread_mutex_lock(&userList.mutex);
    newNode->next = userList.head;
    userList.head = newNode;
    pthread_mutex_unlock(&userList.mutex);

    return 0; // Registro exitoso
}

int unregister_user(const char *username) {
    pthread_mutex_lock(&userList.mutex);

    UserNode *current = userList.head;
    UserNode *previous = NULL;

    while (current != NULL) {
        if (strcmp(current->username, username) == 0) {
            // Eliminar el nodo
            if (previous == NULL) {
                userList.head = current->next; // Eliminar el primer nodo
            } else {
                previous->next = current->next; // Saltar el nodo actual
            }
            free(current);
            pthread_mutex_unlock(&userList.mutex);
            return 0; // Baja exitosa
        }
        previous = current;
        current = current->next;
    }

    pthread_mutex_unlock(&userList.mutex);
    return -1; // Usuario no encontrado
}

// Liberar la memoria de la lista de usuarios
void free_user_list() {
    pthread_mutex_lock(&userList.mutex);

    UserNode *current = userList.head;
    while (current != NULL) {
        UserNode *temp = current;
        current = current->next;
        free(temp);
    }
    userList.head = NULL;

    pthread_mutex_unlock(&userList.mutex);
    pthread_mutex_destroy(&userList.mutex);
}

int is_user_connected(const char *username) {
    pthread_mutex_lock(&userList.mutex);

    UserNode *current = userList.head;
    while (current != NULL) {
        if (strcmp(current->username, username) == 0 && current->connected) {
            pthread_mutex_unlock(&userList.mutex);
            return 1; // Usuario conectado
        }
        current = current->next;
    }

    pthread_mutex_unlock(&userList.mutex);
    return 0; // Usuario no conectado
}

int register_connection(const char *username, int port, int client_socket) {
    pthread_mutex_lock(&userList.mutex);

    UserNode *current = userList.head;
    while (current != NULL) {
        if (strcmp(current->username, username) == 0) {
            current->connected = 1; // Marcar como conectado
            current->port = port;   // Guardar el puerto
            // Obtener la dirección IP del cliente
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            if (getpeername(client_socket, (struct sockaddr *)&client_addr, &addr_len) == 0) {
                current->ip = client_addr.sin_addr; // Guardar la IP
            } else {
                perror("Error al obtener la dirección IP del cliente");
            }

            pthread_mutex_unlock(&userList.mutex);
            return 0; // Conexión exitosa
        }
        current = current->next;
    }

    pthread_mutex_unlock(&userList.mutex);
    return -1; // Usuario no encontrado
}

int is_file_published(const char *file_name) {
    pthread_mutex_lock(&publicationList.mutex);

    PublicationNode *current = publicationList.head;
    while (current != NULL) {
        if (strcmp(current->file_name, file_name) == 0) {
            pthread_mutex_unlock(&publicationList.mutex);
            return 1; // Archivo ya publicado
        }
        current = current->next;
    }

    pthread_mutex_unlock(&publicationList.mutex);
    return 0; // Archivo no publicado
}

int register_publication(const char *username, const char *file_name, const char *description) {
    pthread_mutex_lock(&publicationList.mutex);

    PublicationNode *new_node = malloc(sizeof(PublicationNode));
    if (new_node == NULL) {
        pthread_mutex_unlock(&publicationList.mutex);
        return -1; // Error al asignar memoria
    }

    strncpy(new_node->file_name, file_name, sizeof(new_node->file_name));
    strncpy(new_node->description, description, sizeof(new_node->description));
    strncpy(new_node->username, username, sizeof(new_node->username));
    new_node->next = publicationList.head;
    publicationList.head = new_node;

    pthread_mutex_unlock(&publicationList.mutex);
    return 0; // Publicación registrada
}

int delete_publication(const char *file_name) {
    pthread_mutex_lock(&publicationList.mutex);

    PublicationNode *current = publicationList.head;
    PublicationNode *previous = NULL;

    while (current != NULL) {
        if (strcmp(current->file_name, file_name) == 0) {
            // Eliminar el nodo
            if (previous == NULL) {
                publicationList.head = current->next; // Eliminar el primer nodo
            } else {
                previous->next = current->next; // Saltar el nodo actual
            }
            free(current);
            pthread_mutex_unlock(&publicationList.mutex);
            return 0; // Eliminación exitosa
        }
        previous = current;
        current = current->next;
    }

    pthread_mutex_unlock(&publicationList.mutex);
    return -1; // Archivo no encontrado
}


int get_ListUsers(char *buffer, size_t buffer_size) {
    pthread_mutex_lock(&userList.mutex);

    UserNode *current = userList.head;
    int count = 0;
    size_t offset = 0;

    while (current != NULL) {
        if (current->connected) {
            // Información del usuario
            char *ip_str = inet_ntoa(current->ip); // Convert IP to string
            int written = snprintf(buffer + offset, buffer_size - offset, "%s %s %d\n",
                                    current->username, ip_str, current->port);
            if (written < 0 || written >= buffer_size - offset) {
                break; // Evitar desbordamiento del buffer
            }
            offset += written;
            count++;
        }
        current = current->next;
    }

    pthread_mutex_unlock(&userList.mutex);
    return count; // Retornar el número de usuarios conectados
}

int get_publications(const char *username, char *buffer, size_t buffer_size) {
    pthread_mutex_lock(&publicationList.mutex);

    PublicationNode *current = publicationList.head;
    size_t offset = 0;
    int count = 0;

    // Recorrer la lista de publicaciones
    while (current != NULL) {
        if (strcmp(current->username, username) == 0) {
            char publication_info[256];
            snprintf(publication_info, sizeof(publication_info), "%.*s\n", (int)(sizeof(publication_info) - 2), current->file_name);

            // Verificar que el buffer no se desborde
            if (offset + strlen(publication_info) < buffer_size) {
                strcat(buffer, publication_info);
                offset += strlen(publication_info);
                count++;
            } else {
                break; // Detener si el buffer está lleno
            }
        }
        current = current->next;
    }

    pthread_mutex_unlock(&publicationList.mutex);
    return count; // Retornar el número de publicaciones
}

int unregister_connection(const char *username) {
    pthread_mutex_lock(&userList.mutex);

    UserNode *current = userList.head;
    while (current != NULL) {
        if (strcmp(current->username, username) == 0) {
            current->connected = 0; // Marcar como desconectado
            pthread_mutex_unlock(&userList.mutex);
            return 0; // Desconexión exitosa
        }
        current = current->next;
    }

    pthread_mutex_unlock(&userList.mutex);
    return -1; // Usuario no encontrado
}

int sendByte(int socket, char byte) {
    int bytesSent = write(socket, &byte, 1);
    if (bytesSent < 0) {
        perror("Error al enviar el byte");
        close(socket);
        return -2; // Error al enviar
    }
    return 0; // Envío exitoso
}

// Función para enviar cadenas
int sendMessage(int socket, char *buffer, int len) {
    int totalSent = 0; // Total de bytes enviados
    int bytesSent;

    while (totalSent < len) {
        bytesSent = write(socket, buffer + totalSent, len - totalSent);
        if (bytesSent < 0) {
            perror("Error al enviar la petición");
            close(socket);
            return -2; // Error al enviar
        }
        totalSent += bytesSent;
    }

    return 0; // Envío exitoso
}

// Función para leer una línea desde un socket
ssize_t readLine(int socket, char * buffer, size_t n){
    if (n <= 0 || buffer == NULL) {
        perror("Error al leer el mensaje");
        return -2;
    }

    // Número de bytes leídos por el último fetch
    ssize_t numRead;   
    // Total de bytes leídos hasta el momento
    size_t totRead = 0;   
    char * buf = buffer;
    char ch;

    for (;;) {
        // Leer un byte del socket
        numRead = read(socket, &ch, 1);
        if (numRead == -1) {
            // reiniciar read al ser interrumpido
            if (errno == EINTR) continue;
            else{
                //Cualquier otro error
                close(socket);
                perror("Error al leer el mensaje");
                return -2;                  
            }
        } else if (numRead == 0) { //EOF
            // ningún byte leído, lo que implica que el archivo está vacío
            if (totRead == 0)          
                return 0;
            else
                break;
        } else { 
            // numRead debe ser 1 si hemos llegado hasta aquí
            if (ch == '\n')
                break;
            if (ch == '\0')
                break;
            // descartar n-1 bytes
            if (totRead < n - 1) {     
                totRead++;
                *buf++ = ch;
            }
        }
    }

    *buf = '\0';
    return totRead;
}