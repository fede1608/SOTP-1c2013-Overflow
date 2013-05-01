/*
 * recv_variable.c
 *
 *  Created on: Oct 26, 2012
 *      Author: marke
 */

#include <stdlib.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdint.h>

typedef struct {
	int8_t type;
	int16_t length;
} header_t;

int recv_variable(int socketReceptor, void* buffer) {

	header_t header;
	int bytesRecibidos;

// Primero: Recibir el header para saber cuando ocupa el payload.
	if (recv(socketReceptor, &header, sizeof(header), MSG_WAITALL) <= 0)
		return -1;

// Segundo: Alocar memoria suficiente para el payload.
	buffer = malloc(header.length);

// Tercero: Recibir el payload.
	if((bytesRecibidos = recv(socketReceptor, buffer, header.length, MSG_WAITALL)) < 0){
		free(buffer);
		return -1;
	}

	return bytesRecibidos;

}
