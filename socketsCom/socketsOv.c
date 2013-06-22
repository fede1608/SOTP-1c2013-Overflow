#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "socketsOv.h"




		//Mande un mensaje a un socket determinado
		int mandarMensaje(int unSocket, int tipo, int tamanio, void *buffer) {

			Header header;

			//Que el tamanio lo mande

			header.type = tipo;
			header.payloadlength = tamanio;

			if (send(unSocket, &header, sizeof(Header), 0) >= 0){
				if (send(unSocket, buffer, header.payloadlength, 0) >= 0) {
					return 1;
				}

			}

			return 0;
		}

		//Recibe un mensaje del servidor - Version Lucas
		int recibirMensaje(int unSocket, void** buffer) {

			Header header;

			if((recv(unSocket, &header, sizeof(Header), 0))>=0) {
				*buffer = malloc (header.payloadlength);
				if ((recv(unSocket, buffer, header.payloadlength, 0) >= 0)) {
					return 1;
				}
			}
			return 0;

		}

		//Recibe un mensaje del servidor - Version SO
		int recv_variable(int socketReceptor, void* buffer) {

			Header header;
			int bytesRecibidos;

		// Primero: Recibir el header para saber cuando ocupa el payload.
			if (recv(socketReceptor, &header, sizeof(header), 0) <= 0)
				return -1;

		// Segundo: Alocar memoria suficiente para el payload.
			buffer = malloc(header.payloadlength);

		// Tercero: Recibir el payload.
			if((bytesRecibidos = recv(socketReceptor, buffer, header.payloadlength, 0)) < 0){
				free(buffer);
				return -1;
			}

			return bytesRecibidos;

		}

		//Crea un socket
		int quieroUnPutoSocketAndando(char* direccion, int puerto) {

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
			socketInfo.sin_addr.s_addr = inet_addr(direccion);
			socketInfo.sin_port = htons(puerto);

			// Conectar el socket con la direccion 'socketInfo'.
			if (connect(unSocket, (struct sockaddr*) &socketInfo, sizeof(socketInfo))
					!= 0) {
				perror("Error al conectar socket");
				return 0;
			}

			printf("Conectado!\n");
			return unSocket;
		}
