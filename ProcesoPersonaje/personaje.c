/*
 * personaje.c
 *
 *  Created on: 29/04/2013
 *      Author: utnso
 */
#include "config.h"
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
	//obtener recurso de config
			char** obj;
			char* val;
			int veclong;
			int llego;
			Header h;

			obj=config_get_array_value(config,"obj[Nivel1]");
			val=config_get_string_value(config,"obj[Nivel1]");
			charPer=config_get_string_value(config,"simbolo")[0];
			veclong=lengthVecConfig(val);
			pos.x=1;
			pos.y=1;
		//TODO implementar for de niveles, conexion solicitud de ip:puerto al orquestator y cierre de esa conex

			//conectar con nivel, y planificador

			int unSocket;

			unSocket = quieroUnPutoSocketAndando("127.0.0.1",5000);



			char* charbuf;
			charbuf=malloc(1);
			*charbuf=charPer;

			if (mandarMensaje(unSocket,0 , 1,charbuf)) {

				printf("Llego el OK al nivel\n");

				if(recibirMensaje(unSocket, (void**)&charbuf)>=0) {

					printf("Llego el OK del nivel char %c\n",*charbuf);

				}

				else
					printf("No llego al cliente la confirmacion del nivel (handshake)\n");

			} else {

				printf("No llego al nivel la confirmacion del personaje (handshake)\n");

			}

int ii;
for(ii=0;ii<veclong;ii++)printf("recurso %c",*obj[ii]);



void* buffer;
recActual=*obj[0];
buffer= &recActual;
printf("pos %d %d \n",pos.x,pos.y);
if (mandarMensaje(unSocket,1, sizeof(char),buffer)) {

	printf("Llego el header de la posicion a recurso al nivel\n");


	printf("pos %d %d \n",pos.x,pos.y);
		printf("Llego el recurso actual necesario al nivel\n");
		Header unHeader;
		printf("pos %d %d \n",pos.x,pos.y);
		if (recibirHeader(unSocket,&unHeader)) {

			printf("pos %d %d %d %d\n",pos.x,pos.y,unHeader.payloadlength,unHeader.type);
			Posicion lifeSucks;
			recibirData(unSocket,unHeader,(void**)&lifeSucks);
			rec=lifeSucks;
			printf("pos %d %d \n",pos.x,pos.y);
			printf("Llego %c header respuesta del nivel: %d %d\n",recActual,rec.x,rec.y);
//							rec = (Posicion)*buffer;
			printf("Llego la posicion del recurso solicitado del nivel\n");



		}
}


			for(ii=0;ii<veclong;ii++){

				recActual=*obj[ii];

	//solicitar posicion recurso Recurso: recactual


				//esperar posicion del recurso posicion : rec


			llego=1;

			while(llego){

				//TODO esperar mensaje de movPermitido para continuar

					printf("pos %d %d \n",pos.x,pos.y);

					if (pos.x!=rec.x) {
						(rec.x-pos.x)>0?pos.x++:pos.x--;
					}
					else if (pos.y!=rec.y) {
						(rec.y-pos.y)>0?pos.y++:pos.y--;
					} else {


							//Solicitar instancia del recActual
							//esperar OK

						h.type = 3;
						h.payloadlength = 1;

						if(ii+1==veclong) {
							mandarMensaje(unSocket,4 , sizeof(char),&recActual);
							exit(0); //todo settear variable de escape de nivel terminado
						}
						llego=0;
						recActual=*obj[ii+1];
						buffer = &recActual;

						if (mandarMensaje(unSocket,(int8_t)3 , sizeof(char),buffer)) {


							printf("Llego el header de peticion de recurso %c al nivel\n",recActual);
							printf("Llego el buffer del recurso necesario al nivel\n");
							Header unHeader;
							Posicion lifeSucks;
							if (recibirHeader(unSocket,&unHeader)) {
								printf("Llego header%d %d respuesta del nivel\n", unHeader.payloadlength,unHeader.type);
								recibirData(unSocket,unHeader,(void**)&lifeSucks);
								rec=lifeSucks;
								printf("Llego la confirmacion del recurso del nivel %d  %d\n",rec.x,rec.y);
								}

							}

						}





				//aviso posicion pos al nivel

						h.type = 2;
						buffer=&pos;
						char* sth;
						if (mandarMensaje(unSocket,2 , sizeof(Posicion),buffer)) {
							printf("Llego el header de la posicion del personaje %d %d\n",pos.x,pos.y);
							if (recibirMensaje(unSocket, (void**)&sth)>=0) {
								printf("Llego header respuesta: %c del nivel\n",*sth);
//							rec = *buffer;
						}
					}
					sleep(1);
				}//fin while(llego)
			}//fin for
		sleep(3);

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
