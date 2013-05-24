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

	while (1) {

		//Estructura del paquete como t_paquete y se instancia una llamada Paquete
		//Type - Tamanio del Payload - Payload
		typedef struct t_paquete {
			int8_t type;
			int16_t payloadlength;
			char payload[1024];
		} Paquete;

		//Crea una variable y un puntero tipo Paquete
		Paquete p;
		Paquete *buffer;

		//Define el tipo de mensaje
		p.type = 1;

		//Recibe el contenido del mensaje que se quiere enviar (payload)
		scanf("%s", p.payload);

		//Calcula y asigna el tamanio del payload
		p.payloadlength = strlen(p.payload);

		//Asigna la direccion de memoria de p al puntero buffer
		buffer = &p;

		// Envia la dirreccion de memoria del buffer con el tamanio a unSocket
		//TODO: colocar tamanio del paquete y que se muestre bien en el servidor
		if (send(unSocket, buffer, 500, 0) >= 0) {
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
