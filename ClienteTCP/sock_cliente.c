#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define DIRECCION "127.0.0.1"
#define PUERTO 5000
#define BUFF_SIZE 1024

int main() {

	int unSocket;

	struct sockaddr_in socketInfo;
	//char buffer[BUFF_SIZE];

	printf("Conectando...\n");

// Crear un socket:
// AF_INET: Socket de internet IPv4
// SOCK_STREAM: Orientado a la conexion, TCP
// 0: Usar protocolo por defecto para AF_INET-SOCK_STREAM: Protocolo TCP/IPv4
	if ((unSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Error al crear socket");
		return EXIT_FAILURE;
	}

	socketInfo.sin_family = AF_INET;
	socketInfo.sin_addr.s_addr = inet_addr(DIRECCION);
	socketInfo.sin_port = htons(PUERTO);

// Conectar el socket con la direccion 'socketInfo'.
	if (connect(unSocket, (struct sockaddr*) &socketInfo, sizeof(socketInfo))
			!= 0) {
		perror("Error al conectar socket");
		return EXIT_FAILURE;
	}

	printf("Conectado!\n");

	//Prueba accept

	send(unSocket, "1", 10, 0);

	while (1) {

		//Estructura del paquete como t_paquete y se instancia una llamada Paquete
		//Type - Tamanio del Payload - Payload
		typedef struct t_paquete {
			int8_t type;
			int16_t payloadlength;
			char payload[1024];
		} Paquete;

		//Bautiza a Paquete como p
		//por conveniencia
		Paquete p;

		//Define el tipo de mensaje
		p.type = 1;

		//Recibe el contenido del mensaje que se quiere enviar (payload)
		scanf("%s", p.payload);

		//Calcula y asigna el tamanio del payload
		p.payloadlength = strlen(p.payload);

		//---------------------------------
		//Esto era el primer intento con string, lo dejo por ahora
		//char type[1];
		//char payloadlength[2];
		//char payload[1024];
		//char paquete[1+2+1024];
		//int length;
		//strcpy(type,"1");
		//scanf("%s", payload);
		//length = strlen(payload);
		//sprintf(payloadlength,"%d",length);
		//sprintf(test,"%d",type);
		//strcpy(paquete,type);
		//strcat(paquete,payloadlength);
		//strcat(paquete,payload);
		//----------------------------------

		// Enviar los datos leidos por teclado a traves del socket.
		if (send(unSocket, (struct t_paquete *)&p, sizeof(p), 0) >= 0) {
			printf("Datos enviados!\n");

			if (strcmp(p.payload, "fin") == 0) {

				printf("Cliente cerrado correctamente.\n");
				break;

			}

		} else {
			perror("Error al enviar datos. Server no encontrado.\n");
			break;
		}
	}

	close(unSocket);

	return EXIT_SUCCESS;

}
