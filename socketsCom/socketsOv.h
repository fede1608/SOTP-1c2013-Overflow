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
		int mandarMensaje(int unSocket, int8_t tipo, int tamanio, void *buffer);
		//Recibe un mensaje el cual ya sabes lo que te va a llegar, recomiendo usar las otras funciones en 2 partes
		//ej
		//Posicion lifeSucks
		//recibirData(unSocket,unHeader,(void**)&lifeSucks);
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

#endif /* SOCKETSCOM_H_ */
