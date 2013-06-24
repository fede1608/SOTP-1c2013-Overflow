/*
 * nivel.c
 *
 *  Created on: 29/04/2013
 *      Author: utnso
 */
#include "config.h"
#include "nivel.h"
#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "socketsOv.h"

static int rows,cols;

typedef struct t_posicion {
	int8_t x;
	int8_t y;
} Posicion;

int main(void){
	void* buffer;
//	t_config* config=config_create("config.txt");
	nivel_gui_inicializar();
	nivel_gui_get_area_nivel(&rows, &cols);

//listenear
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
		socketInfo.sin_addr.s_addr = INADDR_ANY; //Notar que aca no se usa inet_addr()
		socketInfo.sin_port = htons(5000);

	// Vincular el socket con una direccion de red almacenada en 'socketInfo'.
		if (bind(socketEscucha, (struct sockaddr*) &socketInfo, sizeof(socketInfo))
				!= 0) {

			perror("Error al bindear socket escucha");
			return EXIT_FAILURE;
		}

	// Escuchar nuevas conexiones entrantes.
		if (listen(socketEscucha, 1) != 0) {
			perror("Error al poner a escuchar socket");
			return EXIT_FAILURE;
		}
		printf("Escuchando conexiones entrantes.\n");

			// Aceptar una nueva conexion entrante. Se genera un nuevo socket con la nueva conexion.
			// La función accept es bloqueante, no sigue la ejecución hasta que se reciba algo
			if ((socketNuevaConexion = accept(socketEscucha, NULL, 0)) < 0) {

				perror("Error al aceptar conexion entrante");
				return EXIT_FAILURE;
			}
			char* rec;
			if(recibirMensaje(socketNuevaConexion, (void**)&rec)>=0) {
								printf("Llego el Personaje %c del nivel",*rec);
								if (mandarMensaje(socketNuevaConexion,0 , 1,rec)) {

												printf("a");
								}
			}


//	Conectar con el personaje y obtener el simbolo

	ITEM_NIVEL * pj1=malloc(sizeof(ITEM_NIVEL));
	pj1->id=*rec;
	pj1->item_type=PERSONAJE_ITEM_TYPE;
	pj1->posx=0;
	pj1->posy=0;
	pj1->quantity=1;
	pj1->next=malloc(sizeof(ITEM_NIVEL));
	ITEM_NIVEL * flor=pj1->next;
	flor->id='F';
		flor->item_type=RECURSO_ITEM_TYPE;
		flor->posx=10;
		flor->posy=14;
		flor->quantity=7;
		flor->next=malloc(sizeof(ITEM_NIVEL));

	ITEM_NIVEL * hongo=flor->next;
		hongo->id='H';
		hongo->item_type=RECURSO_ITEM_TYPE;
		hongo->posx=17;
		hongo->posy=6;
		hongo->quantity=2;
		hongo->next=NULL;


		//esperar solicitud de recurso y enviar posicion del mismo
		Posicion posRec;
		if(recibirMensaje(socketNuevaConexion, (void**)&rec)>=0) {
			posRec.x=10;posRec.y=14;
			if (flor->id==*rec) {posRec.x=flor->posx;posRec.y=flor->posy;}
			if (hongo->id==*rec) {posRec.x=hongo->posx;posRec.y=hongo->posy;}

			buffer=&posRec;
			if (mandarMensaje(socketNuevaConexion,1 , sizeof(Posicion),buffer)) {
			printf("pos rec");
			}
		}

		//FIN


//		ITEM_NIVEL * elemento=flor;

		while(1){

			//esperar pos del personaje y escribirla (preguntar si el tipo de mensaje es pos sino disminuir recurso y darle nueva posicion del otro recurso)
			Header headMen;
			Posicion posPer;
			recibirHeader(socketNuevaConexion, &headMen);
			if(headMen.type==3) {
						char* caracter;
						caracter= malloc(1);
						recibirData(socketNuevaConexion, headMen, (void**)caracter);
						rec=caracter;
						printf("char %c",*rec);
						flor->quantity--;
						if(*rec=='F')posRec.x=flor->posx;posRec.y=flor->posy;
						if(*rec=='H')posRec.x=hongo->posx;posRec.y=hongo->posy;
						if (mandarMensaje(socketNuevaConexion,3 , sizeof(Posicion),&posRec)) {

						}
					}
			else{
				//tipo2
				char* sth;
				recibirData(socketNuevaConexion, headMen, (void**)&posPer);
				pj1->posx=posPer.x;
				pj1->posy=posPer.y;
				printf("px: %d py: %d rx: %d ry:%d",posPer.x,posPer.y,posRec.x,posRec.y);
				sth=malloc(1);
				sth[0]='k';
				if (mandarMensaje(socketNuevaConexion,4 , sizeof(char),(void*)sth)) {
				printf("resp pos %c %d %d \n",flor->id,flor->posx,flor->posy);
				}


			}

//			pj1->posx=0;//settear nueva pos
//			pj1->posy=0; //settear nueva pos

			nivel_gui_dibujar(pj1);
			sleep(1);

		}

		sleep(3);
nivel_gui_terminar();
return 0;
}


