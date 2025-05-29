# Sistema P2P de Intercambio de Archivos

Este repositorio contiene un sistema distribuido cliente-servidor para la asignatura **Sistemas Distribuidos** del curso 2024–2025 en la Universidad Carlos III de Madrid (UC3M). El sistema permite el **registro, conexión, publicación, búsqueda y descarga de archivos** entre usuarios de una red P2P, gestionado por un servidor central multihilo y complementado por servicios web.

## Arquitectura

El sistema está compuesto por:

- **Servidor multihilo (C)**  
  Gestiona usuarios y archivos publicados, sincroniza peticiones concurrentes mediante hilos (`pthread`) y mutexes.

- **Cliente interactivo (Python)**  
  Permite al usuario interactuar con el sistema: registrarse, conectarse, publicar, listar y descargar archivos.

- **Web service**  
  Servicio REST que devuelve la fecha actual del sistema. Se integra como parte del protocolo de comunicación para cada acción del cliente.

## Funcionalidades principales

- `REGISTER / UNREGISTER` – Alta y baja de usuarios.
- `CONNECT / DISCONNECT` – Gestión de conexiones.
- `PUBLISH / DELETE` – Publicación y eliminación de archivos.
- `LIST_USERS` – Listado de usuarios conectados.
- `LIST_CONTENT` – Consulta de archivos publicados por un usuario.
- Integración con web service para añadir metadatos a cada operación.

## Estructura del proyecto

```
├── server.c              # Servidor principal multihilo
├── server.h              # Definiciones y estructuras compartidas
├── socketFunctions.c     # Funciones auxiliares de red y parsing
├── client.py             # Cliente interactivo
├── protocol.py           # Protocolo de comunicación cliente-servidor
├── webService.py         # Servicio web REST
├── Makefile              # Compilación del servidor
```

## Ejecución rápida

Primero debemos establecer el servidor.
Para ello, ejecute en una misma sesión de la terminal los siguientes comandos:
1. make
2. ./server -p <puerto del servidor>

Por otro lado, ya sea en la misma máquina que el servidor u otra, podemos crear
tantos clientes como queramos. 
Con ese fin, primero debemos ejecutar el servicio web en una sesión de la terminal:
3. python3 webService.py

En otra sesión de la terminal dentro de la misma máquina que el servidor web,
creamos un cliente de la siguiente manera: 
4. python3 client.py -s <ip del servidor> -p <puerto del servidor>
