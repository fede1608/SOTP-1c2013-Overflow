#ifndef SOCKETSCOM_H_
#define SOCKETSCOM_H_

#include <sys/types.h>
//Estructura del paquete como t_paquete y se instancia una llamada Paquete
		//Type - Tamanio del Payload
		//Generico
typedef struct t_paquete {
			int8_t type;
			int16_t payloadlength;
		} Header;


		//envia un mensaje se envia el puntero al string o char, o struct ej:
		//char recActual;
		//buffer= &recActual;
		//if (mandarMensaje(unSocket,1 , sizeof(recActual),buffer))
		//o char* caracter; mandarMensaje(unSocket,1 , sizeof(recActual),caracter)
		//Enviar String (mandar tamaño del string +1 para incluir el \0)
		//mandarMensaje(unSocketOrq,1,strlen(nivelActual)+1,nivelActual);
		int mandarMensaje(int unSocket, int8_t tipo, int tamanio, void *buffer);
		//Recibe un mensaje el cual ya sabes lo que te va a llegar, recomiendo usar las otras funciones en 2 partes
		//ej (Recomendable al recibir char o un string)
		//char* nivelDelPersonaje;
		//if(recibirMensaje(socketNuevaConexion, (void**) &nivelDelPersonaje)>=0) {
		int recibirMensaje(int unSocket, void** buffer) ;
		//recibe un header
		//ej
		//Header unHeader;
		//recibirHeader(unSocket,&unHeader)
		int recibirHeader(int unSocket, Header* header);
//		recibe Data enviado el header anteriormente recibido, esta y la funcion anterior son complementarias
		//ej
		//Recibir un string o char
		//char * rec;
		//rec=malloc(sizeof(char));
		//recibirData(socketNuevaConexion, headMen, (void**)rec);
//
		//Recibir un struct
		//Posicion lifeSucks;
		//recibirData(unSocket,unHeader,(void**)&lifeSucks);
		int recibirData(int unSocket, Header header, void** buffer);
		//este no se usa
		int recv_variable(int socketReceptor, void* buffer);
		//devuelve un int que es un socket
		int quieroUnPutoSocketAndando(char* direccion, int puerto);

		//------Funciones extendidas de la librería------

		//Se envía un puerto (int) y devuelvo un socket (int) escuchando en ese puerto
		//Ejemplo:
		//  int socketEscucha = quieroUnPutoSocketDeEscucha(5515);
		//	if (listen(socketEscucha, 10) != 0) {
		//		perror("Error al poner a escuchar socket");
		//		return EXIT_FAILURE; }
		//Nota: El socket de escucha generado se asocia a todas las interfaces
		//      (IPs privadas del host) para ese puerto
		int quieroUnPutoSocketDeEscucha(int puerto);

		//Función para uso interno de la librería.
		//Permite simplificar y reutilizar código entre quieroUnPutoSocketAndando(...);
		//y quieroUnPutoSocketDeEscucha(...);
		//Devuelve un socket genérico y que libera sus recursos inmediatamente cuando
		//recibe un close(socketGenerado);
		int solicitarSocketAlSO();

		//Función para uso interno de la librería.
		//Permite simplificar y reutilizar código entre quieroUnPutoSocketAndando(...);
		//y quieroUnPutoSocketDeEscucha(...);
		//Se le envía una dirección y un puerto y los deja configurados en un struct
		//para ser utilizados en las funciones de sockets
		struct sockaddr_in especificarSocketInfo(char* direccion, int puerto);



		//

#endif /* SOCKETSCOM_H_ */
