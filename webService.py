from spyne import Application, ServiceBase, rpc, Unicode
from spyne.protocol.soap import Soap11
from spyne.server.wsgi import WsgiApplication
from datetime import datetime

class DateTimeService(ServiceBase):
    @rpc(_returns=Unicode)
    def get_datetime(ctx):
        # Obtener la fecha y hora actual
        current_datetime = datetime.now().strftime("%d/%m/%Y %H:%M:%S")
        return current_datetime

# Configurar la aplicación SOAP
application = Application(
    [DateTimeService],
    tns='http://datetime.com',
    in_protocol=Soap11(validator='lxml'),
    out_protocol=Soap11()
)

# Configurar el servidor WSGI
wsgi_app = WsgiApplication(application)

if __name__ == '__main__':
    import logging

    from wsgiref.simple_server import make_server

    logging.basicConfig(level=logging.INFO)
    logging.getLogger('spyne.protocol.xml').setLevel(logging.INFO)

    logging.info("Servicio SOAP ejecutándose en http://127.0.0.1:5000")
    logging.info("WSDL disponible en: http://localhost:5000/?wsdl")

    server = make_server('127.0.0.1', 5000, wsgi_app)
    server.serve_forever()