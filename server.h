#ifndef SERVER_H
#define SERVER_H

//Definir las librerías que serán utilizadas
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>
#include <limits.h>
#include <float.h>
#include <ctype.h>

// Estructura para almacenar la acción y los argumentos
typedef struct {
    char *action;
    char *UserName; 
    char *fecha;
    char *argument1; // Para DELETE, PUBLISH y LIST_CONTENT
    char *argument2; // Solo para PUBLISH
} ParsedMessage;

// Estructura para almacenar usuario registrados
typedef struct UserNode {
    char username[256];           // Nombre del usuario
    int connected;                // Estado de conexión (0 = desconectado, 1 = conectado)
    struct in_addr ip;            // Dirección IP del cliente
    int port;                     // Puerto del cliente
    struct UserNode *next;        // Puntero al siguiente nodo
} UserNode;

// Lista de usuarios
typedef struct {
    UserNode *head;               // Puntero al primer nodo
    pthread_mutex_t mutex;        // Mutex para proteger la lista
} UserList;

// Declarar la lista global de usuarios
extern UserList userList;

typedef struct PublicationNode {
    char file_name[256];
    char description[256];
    char username[256];
    struct PublicationNode *next;
} PublicationNode;

typedef struct {
    PublicationNode *head;
    pthread_mutex_t mutex;
} PublicationList;

extern PublicationList publicationList;

// Declaraciones de funciones
void initializeUserList();
int is_user_registered(const char *username);
int register_user(const char *username);
int unregister_user(const char *username);
void free_user_list();
int is_user_connected(const char *username);
int register_connection(const char *username, int port, int client_socket);
int is_file_published(const char *file_name);
int register_publication(const char *username, const char *file_name, const char *description);
int delete_publication(const char *file_name);
int get_publications(const char *username, char *buffer, size_t buffer_size);
int unregister_connection(const char *username);
int sendByte(int socket, char byte);
int sendMessage(int socket, char *buffer, int len);
int get_ListUsers(char *buffer, size_t buffer_size);
ssize_t readLine(int socket, char *buffer, size_t n);
int parseMessage(int socket, ParsedMessage *parsedMessage);
void freeParsedMessage(ParsedMessage *parsedMessage);

//Definir constantes globales que se van a utilizar con frecuencia
#define ERROR_TUPLAS -1
#define ERROR_COMMUNICATION -2

#endif // SERVER_H