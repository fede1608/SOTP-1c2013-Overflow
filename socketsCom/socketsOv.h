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


		int mandarMensaje(int unSocket, int8_t tipo, int tamanio, void *buffer);
		int recibirMensaje(int unSocket, void** buffer) ;
		int recibirHeader(int unSocket, Header* header);
		int recibirData(int unSocket, Header header, void** buffer);
		int recv_variable(int socketReceptor, void* buffer);
		int quieroUnPutoSocketAndando(char* direccion, int puerto);

#endif /* SOCKETSCOM_H_ */
