
# P2P File Sharing System

This repository contains a distributed client-server system developed for the **Distributed Systems** course (2024–2025) at Universidad Carlos III de Madrid (UC3M). The system allows **user registration, connection, file publishing, searching, and downloading** among users in a P2P network, managed by a multithreaded central server and complemented by web services.

## Architecture

The system consists of:

- **Multithreaded server (C)**  
  Manages users and published files, handling concurrent requests using threads (`pthread`) and mutexes.

- **Interactive client (Python)**  
  Allows the user to interact with the system: register, connect, publish, list, and download files.

- **Web service**  
  A REST service that returns the current system date. It is integrated as part of the communication protocol for each client action.

## Main Features

- `REGISTER / UNREGISTER` – User registration and removal.
- `CONNECT / DISCONNECT` – Connection management.
- `PUBLISH / DELETE` – File publication and deletion.
- `LIST_USERS` – List of connected users.
- `LIST_CONTENT` – Query of files published by a user.
- Integration with a web service to add metadata to each operation.

## Project Structure

```
├── server.c              # Main multithreaded server
├── server.h              # Shared definitions and structures
├── socketFunctions.c     # Network and parsing helper functions
├── client.py             # Interactive client
├── protocol.py           # Client-server communication protocol
├── webService.py         # REST web service
├── Makefile              # Server compilation
```

## Quick Start

First, we must set up the server.
To do so, run the following commands in the same terminal session:

1. make  
2. ./server -p <server port>

On the other hand, either on the same machine as the server or on another,
we can create as many clients as we want.

To do this, first run the web service in a terminal session:

3. python3 webService.py

Then, in another terminal session on the same machine as the web server,
create a client as follows:

4. python3 client.py -s <server IP> -p <server port>
