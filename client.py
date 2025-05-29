from enum import Enum
from pathlib import Path
import argparse
import protocol
import socket
import threading
import os
import zeep # Para Servicio Web SOAP

class client :

    # ******************** TYPES *********************
    # *
    # * @brief Return codes for the protocol methods
    class RC(Enum) :
        OK = 0
        ERROR = 1
        USER_ERROR = 2

    # ****************** ATTRIBUTES ******************
    _server = None
    _port = -1

    _user = None
    _running = False
    _listener = None
    _thread = None

    # ******************** METHODS *******************

    @staticmethod
    def _validate_field(field: str) -> bool:
        return (field is not None) and isinstance(field, str) and 0 < len(field.encode()) <= protocol.MAX_LEN

    @staticmethod
    def get_datetime():
        # Consumir el servicio web para obtener la fecha y hora
        wsdl = 'http://127.0.0.1:5000/?wsdl'
        client = zeep.Client(wsdl=wsdl)
        return client.service.get_datetime()


    @staticmethod
    def  register(user) :
        if client._validate_field(user):
            datetime_str = client.get_datetime()  # Obtener la fecha y hora  
            msg = protocol.register(client._server, client._port, datetime_str, user)
            print("c> " + msg)
        else:
            settings = protocol.SETTINGS['register']
            print("c> " + settings[settings['default']])
   
    @staticmethod
    def  unregister(user) :
        if client._validate_field(user):  
            datetime_str = client.get_datetime()  # Obtener la fecha y hora  
            msg = protocol.unregister(client._server, client._port, datetime_str, user)
            print("c> " + msg)
        else:
            settings = protocol.SETTINGS['unregister']
            print("c> " + settings[settings['default']])

    @staticmethod
    def _handle_p2p_connection(connection):
        try:
            # recibir "GET_FILE"
            command = protocol.recv_str(connection)
            if command == "GET_FILE":
                # recibir path absoluto de uno de mis ficheros
                file_path_str = protocol.recv_str(connection)
                file_path = Path(file_path_str)

                if not file_path.is_absolute():
                    # Path no absoluto, error. Mandamos 2
                    connection.sendall(bytes([2])) 
                elif not file_path.exists() or not file_path.is_file():
                    connection.sendall(bytes([1])) # mandar 1 si el file no existe
                else:
                    try:
                        connection.sendall(bytes([0])) # mandar 0 si tengo el get_file y el fichero
                        
                        file_size = os.path.getsize(file_path)
                        # mandar tamaño en bytes del fichero como una cadena
                        protocol.send_str(connection, str(file_size))

                        # abrir fichero y mandar bloques periódicamente. with open se encargará de cerrarlo más tarde
                        with open(file_path, 'rb') as f:
                            while True:
                                chunk = f.read(protocol.MAX_FILE_SIZE)
                                if not chunk:
                                    break # Fin del fichero
                                connection.sendall(chunk)
                    except Exception:
                        # Error al abrir el archivo. Mandamos 2
                        connection.sendall(bytes([2]))
            else:
                # Comando no esperado, mandamos 2
                connection.sendall(bytes([2]))
        except (socket.error, ValueError, ConnectionError, OSError, TimeoutError, UnicodeError) as e:
            # Error no esperado, mandamos 2
            connection.sendall(bytes([2]))
        finally:
            # una vez la transferencia está hecha o ha habido algún tipo de error, cerramos la comunicación con el cliente
            connection.close()
    
    @staticmethod
    def _p2p_server_loop(sock):
        while client._running:
            try:
                # connection es un nuevo socket que se usa para transmitir datos con el otro cliente
                # client_address es una tupla con la dirección ip y el puerto del cliente
                connection, client_address = sock.accept()
                # creamos un thread daemon para gestionar cada petición
                threading.Thread(target=client._handle_p2p_connection, args=(connection,), daemon=True).start()                    
            except socket.timeout:
                # cada segundo revisa de nuevo el flag _running
                continue
            except OSError:
                # listener.close() en disconnect() lanza OSError. debemos salir del bucle
                break
    
    @staticmethod
    def  connect(user) :
        if client._validate_field(user):
            if (client._user is not None) and (client._user != user):
                client.disconnect(client._user)
            try:
                # Creamos el socket de servidor
                client._listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                client._listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                # El listener revisará periódicamente si debe ser desactivado
                client._listener.settimeout(1)

                # Configuramos este socket con la IP host local y puerto 0 → el SO elige un puerto libre
                client._listener.bind(('0.0.0.0', 0))

                # Recuperamos el puerto asignado
                _, chosen_port = client._listener.getsockname()

                # Empieza a escuchar. Permitimos que haya hasta 5 clientes en cola si ya hay uno conectado
                client._listener.listen(5)
                
                # por defecto, el hilo ejecutará
                client._running = True
                # creamos el hilo
                client._thread = threading.Thread(target=client._p2p_server_loop, args=(client._listener,))
                client._thread.start()
                datetime_str = client.get_datetime()  # Obtener la fecha y hora
                msg = protocol.connect(client._server, client._port, datetime_str, user, chosen_port)
                print("c> " + msg)
                if msg == "CONNECT OK":
                    # en caso de una conexión exitosa, cambiamos el nombre de usuario actualmente conectado
                    client._user = user
                else:
                    raise Exception
            except Exception:
                # si la conexión falla, hay que deshacer todos los cambios
                if client._listener:
                    client._running = False
                    client._listener.close()
                    client._listener = None
                if client._thread:
                    client._thread.join()
                    client._thread = None

        else:
            settings = protocol.SETTINGS['connect']
            print("c> " + settings[settings['default']])


    @staticmethod
    def  disconnect(user) :
        # toda desconexión implica borrar el nombre del usuario conectado actualmente
        client._user = None
        if client._validate_field(user):  
            if client._listener and client._thread:
                # señalo al thread que debe parar su ejecución
                client._running = False
                # cierro el socket de escucha, lo que fuerza un OS error en el thread
                client._listener.close()
                client._listener = None
                # espero a que el hilo termine
                client._thread.join()
                client._thread = None
            datetime_str = client.get_datetime()  
            msg = protocol.disconnect(client._server, client._port, datetime_str, user)
            print("c> " + msg)
        else:
            settings = protocol.SETTINGS['disconnect']
            print("c> " + settings[settings['default']])

    @staticmethod
    def  publish(fileName,  description) :
        if (client._validate_field(fileName) and (Path(fileName).is_absolute()) and 
            client._validate_field(description)
        ):  
            datetime_str = client.get_datetime()
            msg = protocol.publish(client._server, client._port, datetime_str, client._user, fileName, description)
            print("c> " + msg)
        else:
            settings = protocol.SETTINGS['publish']
            print("c> " + settings[settings['default']])

    @staticmethod
    def  delete(fileName) :
        if (client._validate_field(fileName) and (Path(fileName).is_absolute())):
            datetime_str = client.get_datetime()            
            msg = protocol.delete(client._server, client._port, datetime_str, client._user, fileName)
            print("c> " + msg)
        else:
            settings = protocol.SETTINGS['delete']
            print("c> " + settings[settings['default']])
    


    @staticmethod
    def  listusers() :
        datetime_str = client.get_datetime()
        msg = protocol.list_users(client._server, client._port, datetime_str, client._user)
        print("c> " + msg)

    @staticmethod
    def  listcontent(user) :
        datetime_str = client.get_datetime()
        msg = protocol.list_content(client._server, client._port, datetime_str, client._user, user)
        print("c> " + msg)
    
    @staticmethod
    def _get_remote_user_address(target_user_name):
        datetime_str = client.get_datetime()
        # función helper para obtener la ip y puerto de un usuario
        users_response = protocol.list_users(client._server, client._port, datetime_str, client._user)
        resp = users_response.strip()

        if not resp or '\n' not in resp:
            return None, None  # respuesta mal formada o vacía

        first_line, *rest = resp.splitlines()

        if first_line.upper().startswith("LIST_USERS OK"):
            for line_data in rest:
                if not line_data.strip():
                    continue
                parts = line_data.split()
                if len(parts) >= 3 and parts[0] == target_user_name:
                    return parts[1], parts[2]  # IP, Puerto
            return None, None  # el usuario no estaba en la lista
        else:
            return None, None  # list_users ha fallado

    @staticmethod
    def  getfile(user,  remote_FileName,  local_FileName) :
        settings = protocol.SETTINGS['get_file']
        if (client._validate_field(user) and 
            client._validate_field(remote_FileName) and (Path(remote_FileName).is_absolute()) and 
            client._validate_field(local_FileName) and (Path(local_FileName).is_absolute())
        ):  

            # obtenemos la IP y el puerto del user
            remote_user_ip, remote_user_port = client._get_remote_user_address(user)

            if remote_user_ip is None or remote_user_port is None:
                # Error al obtener la dirección del usuario (no encontrado, error en list_users, etc.)
                print("c> " + settings[settings['default']])
                return
            

            code, sock = protocol.communicate_with_server(remote_user_ip, int(remote_user_port), ["GET_FILE", remote_FileName], settings['default'])        
            msg = settings.get(code, settings[settings['default']])

            # si el código recibido es 0, esperamos recibir la lista de usuarios
            if (code == 0) and (sock is not None):
                try:
                    # with asegura cerrar el socket después de salir de él
                    with sock as sock_conn:
                        number_str = protocol.recv_str(sock_conn)
                        try:
                            number = int(number_str)
                        except ValueError:
                            # el servidor ha retornado un valor no integer
                            print("c> " + settings[settings['default']])
                            return
                        
                        # creamos o abrimos el fichero. además, with open se asegurará de cerrarlo más tarde
                        with open(local_FileName, 'wb') as f:
                            # leer contenido del fichero remoto desde sock_conn
                            bytes_remaining = number
                            while bytes_remaining > 0:
                                chunk_size = min(protocol.MAX_FILE_SIZE, bytes_remaining)
                                data = protocol.recv_bytes(sock_conn, chunk_size)
                                if not data:
                                    # Conexión cerrada prematuramente
                                    print("c> " + settings[settings['default']])
                                    # tratamos de borrar el archivo incompleto
                                    try:
                                        os.remove(local_FileName)
                                    except OSError:
                                        pass
                                    return
                                f.write(data)
                                bytes_remaining -= len(data)
                    
                # si hay cualquier tipo de error en el cliente, se devuelve el valor predeterminado de error
                except (socket.error, ValueError, ConnectionError, OSError, TimeoutError, UnicodeError) as e:
                    print("c> " + settings[settings['default']])
                    return
            else:
                # communicate with server falló
                # msg ya contiene el valor apropiado
                # solo tenemos que asegurarnos de cerrar el socket
                if sock:
                    sock.close()

            print(msg)
            return
        
        else:
            print("c> " + settings[settings['default']])
            return


    # *
    # **
    # * @brief Command interpreter for the client. It calls the protocol functions.
    @staticmethod
    def shell():

        while (True) :
            try :
                command = input("c> ")
                line = command.split(" ")
                if (len(line) > 0):

                    line[0] = line[0].upper()

                    if (line[0]=="REGISTER") :
                        if (len(line) == 2) :
                            client.register(line[1])
                        else :
                            print("Syntax error. Usage: REGISTER <userName>")

                    elif(line[0]=="UNREGISTER") :
                        if (len(line) == 2) :
                            client.unregister(line[1])
                        else :
                            print("Syntax error. Usage: UNREGISTER <userName>")

                    elif(line[0]=="CONNECT") :
                        if (len(line) == 2) :
                            client.connect(line[1])
                        else :
                            print("Syntax error. Usage: CONNECT <userName>")
                    
                    elif(line[0]=="PUBLISH") :
                        if (len(line) >= 3) :
                            #  Remove first two words
                            description = ' '.join(line[2:])
                            client.publish(line[1], description)
                        else :
                            print("Syntax error. Usage: PUBLISH <fileName> <description>")

                    elif(line[0]=="DELETE") :
                        if (len(line) == 2) :
                            client.delete(line[1])
                        else :
                            print("Syntax error. Usage: DELETE <fileName>")

                    elif(line[0]=="LIST_USERS") :
                        if (len(line) == 1) :
                            client.listusers()
                        else :
                            print("Syntax error. Use: LIST_USERS")

                    elif(line[0]=="LIST_CONTENT") :
                        if (len(line) == 2) :
                            client.listcontent(line[1])
                        else :
                            print("Syntax error. Usage: LIST_CONTENT <userName>")

                    elif(line[0]=="DISCONNECT") :
                        if (len(line) == 2) :
                            client.disconnect(line[1])
                        else :
                            print("Syntax error. Usage: DISCONNECT <userName>")

                    elif(line[0]=="GET_FILE") :
                        if (len(line) == 4) :
                            client.getfile(line[1], line[2], line[3])
                        else :
                            print("Syntax error. Usage: GET_FILE <userName> <remote_fileName> <local_fileName>")

                    elif(line[0]=="QUIT") :
                        if (len(line) == 1) :
                            if (client._user is not None):
                                client.disconnect(client._user)
                            break
                        else :
                            print("Syntax error. Use: QUIT")
                    else :
                        print("Error: command " + line[0] + " not valid.")
            except Exception as e:
                print("Exception: " + str(e))

    # *
    # * @brief Prints program usage
    @staticmethod
    def usage() :
        print("Usage: python3 client.py -s <server> -p <port>")


    # *
    # * @brief Parses program execution arguments
    @staticmethod
    def  parseArguments(argv) :
        parser = argparse.ArgumentParser()
        parser.add_argument('-s', type=str, required=True, help='Server IP')
        parser.add_argument('-p', type=int, required=True, help='Server Port')
        args = parser.parse_args()

        if (args.s is None):
            parser.error("Usage: python3 client.py -s <server> -p <port>")
            return False

        if ((args.p < 1024) or (args.p > 65535)):
            parser.error("Error: Port must be in the range 1024 <= port <= 65535");
            return False
        
        client._server = args.s
        client._port = args.p

        return True


    # ******************** MAIN *********************
    @staticmethod
    def main(argv) :
        if (not client.parseArguments(argv)) :
            client.usage()
            return

        client.shell()
        print("+++ FINISHED +++")
    

if __name__=="__main__":
    client.main([])