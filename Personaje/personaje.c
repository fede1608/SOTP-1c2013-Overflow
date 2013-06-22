/*
 * personaje.c
 *
 *  Created on: 29/04/2013
 *      Author: utnso
 */
#include "config.h"
#include "nivel.h"
#include <string.h>
#include "string.h"
#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <sys/ioctl.h>
#include "socketsOv.h"


typedef struct t_posicion {
	int8_t x;
	int8_t y;
} Posicion;

//static int rows,cols;
Posicion pos;
Posicion rec;
char recActual;
char charPer;
int lengthVecConfig(char * value);

int main(void){
	t_config* config=config_create("config.txt");
	//obtener recurso
			char** obj;
			char* val;
			int veclong;
			int llego;
			obj=config_get_array_value(config,"obj[Nivel1]");
			val=config_get_string_value(config,"obj[Nivel1]");
			charPer=config_get_string_value(config,"simbolo")[0];
			veclong=lengthVecConfig(val);

	//conectar con nivel

			int unSocket;

			unSocket = quieroUnPutoSocketAndando("127.0.0.1",5000);

			Header h;
			h.type = 0;
			h.payloadlength = 1;
			char* jo;
			*jo=charPer;
			if (mandarMensaje(unSocket,0 , 1,jo)) {

				printf("Llego el OK al nivel\n");

				if(recibirMensaje(unSocket, (void**)&jo)) {

					printf("Llego el OK del nivel\n");

				}

				else
					printf("No llego al cliente la confirmacion del nivel (handshake)\n");

			} else {

				printf("No llego al nivel la confirmacion del personaje (handshake)\n");

			}

int ii;
			for(ii=0;ii<veclong;ii++){

				recActual=*obj[ii];

	//solicitar posicion recurso Recurso: recactual

				h.type = 1;
				h.payloadlength = sizeof(recActual);
				void* buffer;
				buffer= &recActual;

				if (mandarMensaje(unSocket,1 , sizeof(recActual),buffer)) {

					printf("Llego el header de la posicion a recurso al nivel\n");



						printf("Llego el recurso actual necesario al nivel\n");

						if (recibirMensaje(unSocket, (void**)&rec)) {

							printf("Llego header respuesta del nivel\n");
//							rec = (Posicion)*buffer;
								printf("Llego la posicion del recurso solicitado del nivel\n");



						}



				}

				//esperar posicion del recurso posicion : rec


				llego=1;

				while(llego){


			//if ((char)*obj[now]==elemento->id) { //strcmp devuelve 0 si es true por eso hay q negarlo, vaya uno a saber porque
				if (pos.x!=rec.x) {
					(pos.x-rec.x)>0?pos.x++:pos.x--;

				}else if (pos.y!=rec.y) {
					(rec.y-pos.y)>0?pos.y++:pos.y--;

				}else {
					//solicitar
					llego=0;

//


				//Solicitar instancia del recActual
					//esperar OK

					h.type = 3;
					h.payloadlength = 1;
					buffer = &recActual;
					int * algo;
					if (mandarMensaje(unSocket,3 , sizeof(recActual),buffer)) {


						printf("Llego el header de peticion de recurso al nivel\n");
						printf("Llego el buffer del recurso necesario al nivel\n");
						if (recibirMensaje(unSocket, (void**)&algo)) {
//							if (recv(unSocket, &h, sizeof(Header), 0) >= 0) {

								printf("Llego header respuesta del nivel\n");

//								if (recv(unSocket,(int)** &buffer, h.payloadlength, 0) >= 0) {

									llego = *algo;
									printf("Llego la confirmacion del recurso del nivel\n");

								}

							}

				}





				//aviso posicion pos al nivel

				h.type = 2;
				buffer=&pos;
				if (mandarMensaje(unSocket,2 , sizeof(Posicion),buffer)) {
//				if (send(unSocket, &h, sizeof(Header), 0) >= 0) {

					printf("Llego el header de la posicion a recurso al nivel\n");

//					if (send(unSocket, buffer, strlen(buffer), 0) >= 0) {

						printf("Llego el recurso actual necesario al nivel\n");
						if (recibirMensaje(unSocket, (void**)&rec)) {
//						if (recv(unSocket, &h, sizeof(Header), 0) >= 0) {

							printf("Llego header respuesta del nivel\n");

//								if (recv(unSocket,(Posicion)* &buffer, h.payloadlength, 0) >= 0) {

//									rec = *buffer;
									printf("Llego la posicion del recurso solicitado del nivel\n");

//								}

							}

//						}

					}


		sleep(1);
		}
}
		sleep(3);
//solicitar cerrar conexxion
return 0;
}

int lengthVecConfig(char * value){
	int cont=1;
	int i;
	for (i=1;i< strlen(value);i++) {
		if (string_starts_with(string_substring_from(value,i),","))
			cont++;
	}

return cont;
}
