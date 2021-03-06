#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define DIRECCION INADDR_ANY   //INADDR_ANY representa la direccion de cualquier
							   //interfaz conectada con la computadora
#define PUERTO 5000
#define BUFF_SIZE 1024

void atenderCliente (int socketNuevaConexion){

	//Define la estructura del paquete
	typedef struct t_paquete {
		int8_t type;
		int16_t payloadlength;
	} Header;

	Header h;
	void* buffer;

	while (1) {

		int nbytesRecibidos;

			// Recibe header
			if(recv(socketNuevaConexion, &h, sizeof(Header),0) > 0){
				printf("Header del cliente recibido\n");
				buffer = malloc(h.payloadlength);

			// Recibe payload hasta la cantidad indicada por el header y lo guarda en buffer
				if ((nbytesRecibidos = recv(socketNuevaConexion, buffer, h.payloadlength, 0) )
						> 0) {
					printf("Mensaje recibido: ");
					fwrite(buffer, 1, h.payloadlength, stdout);
					printf("\n");
					printf("Tamanio del buffer %d bytes!\n", nbytesRecibidos);
					fflush(stdout);

					//Verifica que si el payload contiene la palabra "fin" para cerrar la conexion
					if (memcmp(buffer, "fin", nbytesRecibidos) == 0) {
						printf("Conexion con cliente cerrada correctamente.\n");
						break;
					}

					// Define el tipo de mensaje
					// Test
					h.type = 1;

					// Esto es un test, si en el buffer esta la palabra "Pudrio",
					// el tipo de mensaje de respuesta sera 2
					if (strcmp(buffer, "Pudrio") == 0) {
						h.type = 2;
						printf("Mensaje de rechazo cargado correctamente.\n");
					}

					//No se que hace esto, pero anda
					memset(buffer,0,h.payloadlength);

					//Arma el nuevo header y buffer para enviar
					h.payloadlength = strlen("Respuesta del servidor");
					buffer = malloc(sizeof(h.payloadlength));
					strcpy(buffer,"Respuesta del servidor");

					//Envia header al cliente
					if (send(socketNuevaConexion, &h, sizeof(Header), 0) >= 0){

						printf("Header enviado\n");

						//Envia buffer (payload) al cliente
						if (send(socketNuevaConexion, buffer, h.payloadlength, 0) >= 0)
							printf("Respuesta enviada\n");

						else
							printf("Fallo el envio del payload\n");

					}

					else
						printf("Fallo el envio del Header\n");

					//TODO: No se por que tira error esto
					//free(buffer);

				} else {
					perror("Error al recibir datos");
					break;
				}

			} else {
				perror("Error al recibir header");
				break;
			}
		}

	close(socketNuevaConexion);
	return;
}

int main(void) {

	int socketEscucha, socketNuevaConexion;

	struct sockaddr_in socketInfo;
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

		//Creación del thread que atiende al cliente. Se crean tantos threads como nuevas conexiones haya
		pthread_t thr1;

		//Se crea el thread y le indicamos que use la función antederCliente con el socket de la nueva conexión
		//como parámetro
		pthread_create(&thr1, NULL, atenderCliente, socketNuevaConexion);

		//Si sólo se quiere que el servidor atienda una sóla conexión, descomentar
		//lo que viene a continuación:
		//pthread_join(thr1, NULL);
		//pthread_detach(thr1);

	}

	close(socketEscucha);
	return EXIT_SUCCESS;
}
