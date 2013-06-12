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
		} Header;

		//Se inicializa el Header, buffer y mensaje
		Header h;
		void* buffer;
		char mensaje[1024];

		//Recibe el contenido del mensaje que se quiere enviar (payload)
		//AVISO: No tengo idea por qué si mandás un string con espacio, lo manda como 2 paquetes distintos
		printf("Escriba un mensaje\n");
		scanf("%s", mensaje);

		//Define el tipo de mensaje
		//Test
		h.type = 1;

		//Calcula y asigna el tamanio del payload
		h.payloadlength = strlen(mensaje);

		//Aloja la memoria necesaria para el payload
		buffer = malloc(h.payloadlength);
		strcpy(buffer,mensaje);

		// Envia el header al servidor
		if (send(unSocket, &h, sizeof(Header), 0) >= 0) {
			printf("Header enviado!\n");

			// Envia el payload al servidor
			if(send(unSocket, buffer, h.payloadlength, 0) >= 0){

				//Chequea si el payload enviado contiene la palabra "fin"
				//si es asi, sale del while y cierra la conexion
				if (strcmp(buffer, "fin") == 0) {

					printf("Cliente cerrado correctamente.\n");
					break;
				}

				//No se que hace pero funciona
				memset(buffer,0,h.payloadlength);

				//Recibe la respuesta (header) del servidor
				if(recv(unSocket, &h, sizeof(Header), 0)>=0){

					printf("Header recibido!.\n");

					//Aloja memoria para el siguiente mensaje (payload)
					buffer = malloc(h.payloadlength);

					//Test: chequea si el tipo de mensaje es 2
					if (h.type == 2)
						printf("Mensaje de Se pudrio Todo!\n");

					//Recibe la respuesta (payload) del servidor
					if(recv(unSocket, buffer, h.payloadlength, 0) >= 0){
						fwrite(buffer, 1, h.payloadlength, stdout);
						printf("\n");
						fflush(stdout);
						free(buffer);
					}
					else {
						printf("Error al recibir payload.\n");
						free(buffer);
						break;
					}

				}
				else {
					printf("Error al recibir header.\n");
					free(buffer);
					break;
				}

			} else {
				perror("Error al enviar payload. Server no encontrado.\n");
				free(buffer);
				break;
			}

		} else {
			perror("Error al enviar header. Server no encontrado.\n");
			free(buffer);
			break;
		}

	}

	close(unSocket);

	return EXIT_SUCCESS;

}
