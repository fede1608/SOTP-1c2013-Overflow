#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define DIRECCION INADDR_ANY   //INADDR_ANY representa la direccion de cualquier
							   //interfaz conectada con la computadora
#define PUERTO 5000
#define BUFF_SIZE 1024

int main() {

	int socketEscucha, socketNuevaConexion;
	int nbytesRecibidos;

	struct sockaddr_in socketInfo;
	char buffer[BUFF_SIZE];
	int optval = 1;

	// Crear un socket:
	// AF_INET: Socket de internet IPv4
	// SOCK_STREAM: Orientado a la conexion, TCP
	// 0: Usar protocolo por defecto para AF_INET-SOCK_STREAM: Protocolo TCP/IPv4
	if ((socketEscucha = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Error al crear el socket");
		return EXIT_FAILURE;
	}

	// Hacer que el SO libere el puerto inmediatamente luego de cerrar el socket.
	setsockopt(socketEscucha, SOL_SOCKET, SO_REUSEADDR, &optval,
			sizeof(optval));

	socketInfo.sin_family = AF_INET;
	socketInfo.sin_addr.s_addr = DIRECCION; //Notar que aca no se usa inet_addr()
	socketInfo.sin_port = htons(PUERTO);

// Vincular el socket con una direccion de red almacenada en 'socketInfo'.
	if (bind(socketEscucha, (struct sockaddr*) &socketInfo, sizeof(socketInfo))
			!= 0) {

		perror("Error al bindear socket escucha");
		return EXIT_FAILURE;
	}

// Escuchar nuevas conexiones entrantes.
	if (listen(socketEscucha, 10) != 0) {

		perror("Error al poner a escuchar socket");
		return EXIT_FAILURE;

	}

	//Este ciclo while sirve para que el servidor pueda volver a aceptar conexiones entrantes cuando se termine la conexión
	//con el cliente que está atendiendo.
	while(1){


		printf("Escuchando conexiones entrantes.\n");

		// Aceptar una nueva conexion entrante. Se genera un nuevo socket con la nueva conexion.
		// La función accept es bloqueante, no sigue la ejecución hasta que se reciba algo
		if ((socketNuevaConexion = accept(socketEscucha, NULL, 0)) < 0) {

			perror("Error al aceptar conexion entrante");
			return EXIT_FAILURE;

		}

		//TODO: En este lugar comenzará un nuevo thread, pasándole de alguna forma la variable socketNuevaConexion

		//La conexión sólo se establece al recibir el primer mensaje. Si no aparece esto es porque no llegó

		printf("Conexión establecida wachoooooooo.\n");

		//TODO: Convertir este ciclo en una función, así puede ser ejecutada por el thread
		while (1) {

			// Recibir hasta BUFF_SIZE datos y almacenarlos en 'buffer'.
			if ((nbytesRecibidos = recv(socketNuevaConexion, buffer, BUFF_SIZE, 0) )
					> 0) {
				printf("Mensaje recibido: ");
				fwrite(buffer, 1, nbytesRecibidos, stdout);
				printf("\n");
				printf("Tamanio del buffer %d bytes!\n", nbytesRecibidos);
				fflush(stdout);

				if (memcmp(buffer, "fin", nbytesRecibidos) == 0) {

					printf("Server cerrado correctamente.\n");
					break;

				}

			} else {
				perror("Error al recibir datos");
				break;
			}
		}

		close(socketNuevaConexion);
	}
	close(socketEscucha);
	return EXIT_SUCCESS;
}
