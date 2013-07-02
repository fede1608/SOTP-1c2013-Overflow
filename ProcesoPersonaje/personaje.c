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
int vidas;
char recActual;
char charPer;
int lengthVecConfig(char * value);

int main(void){
	t_config* config=config_create("config.txt");
	//obtener recurso de config
			char** obj,niveles;
			char* val,aux;
			char objNivel[14]="obj[Nivel1]";
			char corchete[2]="]";
			int veclong, llego, cantNiv,numNiv, c,ii;
			Header h;
			niveles=config_get_array_value(config,"planDeNiveles");
			vidas=config_get_int_value(config,"vidas");
			charPer=config_get_string_value(config,"simbolo")[0];
			cantNiv=config_keys_amount(config)-5/*cant de keys fijas*/;

		for(c=0;c<cantNiv;c++){ //for each nivel

			memcpy(&(objNivel[4]),niveles[c],strlen(niveles[c]));
			memcpy(&(objNivel[4+strlen(niveles[c])]),corchete,2);
			numNiv=(int)strtol(niveles[c][5], &aux, 10);

			obj=config_get_array_value(config,objNivel);
			val=config_get_string_value(config,objNivel);
			veclong=lengthVecConfig(val);
			pos.x=1;
			pos.y=1;
		//TODO conexion solicitud de ip:puerto al orquestator y cierre de esa conex

			//conectar con nivel, y planificador
			int unSocket;
			unSocket = quieroUnPutoSocketAndando("127.0.0.1",5000);

			//esperar Primer movPermitido
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


//for(ii=0;ii<veclong;ii++)printf("recurso %c",*obj[ii]);

void* buffer;
recActual=*obj[0];
buffer= &recActual;

if (mandarMensaje(unSocket,1, sizeof(char),buffer)) {
	printf("Llego el header de la posicion a recurso al nivel\n");
	printf("Llego el recurso actual necesario al nivel\n");
	Header unHeader;
	if (recibirHeader(unSocket,&unHeader)) {
		printf("pos %d %d %d %d\n",pos.x,pos.y,unHeader.payloadlength,unHeader.type);
		Posicion lifeSucks;
		recibirData(unSocket,unHeader,(void**)&lifeSucks);
		rec=lifeSucks;
		printf("Llego %c header respuesta del nivel: %d %d\n",recActual,rec.x,rec.y);
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
		}

		//aviso posicion pos al nivel
		h.type = 2;
		buffer=&pos;
		char* sth;

		if (mandarMensaje(unSocket,2 , sizeof(Posicion),buffer)) {
			printf("Llego el header de la posicion del personaje %d %d\n",pos.x,pos.y);
			if (recibirMensaje(unSocket, (void**)&sth)>=0) {
				printf("Llego header respuesta: %c del nivel\n",*sth);
			}
		}

		if((pos.y==rec.y)&(pos.x==rec.x)) {
			//Solicitar instancia del recActual
			//esperar OK
			h.type = 3;
			h.payloadlength = 1;
			llego=0;
			buffer = &recActual;

			if (mandarMensaje(unSocket,(int8_t)3 , sizeof(char),buffer)) {
				printf("Llego el header de peticion de recurso %c al nivel\n",recActual);
				printf("Llego el buffer del recurso necesario al nivel\n");
				Header unHeader;
				Posicion lifeSucks;

				if (recibirHeader(unSocket,&unHeader)) {
					printf("Llego header%d %d respuesta del nivel\n", unHeader.payloadlength,unHeader.type);
					recibirData(unSocket,unHeader,(void**)&lifeSucks);
					//rec=lifeSucks;
					printf("Llego la confirmacion del recurso del nivel %d  %d\n",rec.x,rec.y);
				}
			}

			if(ii+1==veclong) {
				//cerrar conexion con el nivel
				mandarMensaje(unSocket,4 , sizeof(char),&recActual);
				//exit(0); //todo settear variable de escape de nivel terminado
			}else {
				recActual=*obj[ii+1];
				buffer=&recActual;

				if (mandarMensaje(unSocket,1, sizeof(char),buffer)) {
					printf("Llego el header de la posicion a recurso al nivel\n");
					printf("Llego el recurso actual necesario al nivel\n");
					Header unHeader;

					if (recibirHeader(unSocket,&unHeader)) {
						printf("pos %d %d %d %d\n",pos.x,pos.y,unHeader.payloadlength,unHeader.type);
						Posicion lifeSucks;
						recibirData(unSocket,unHeader,(void**)&lifeSucks);
						rec=lifeSucks;
						printf("Llego %c header respuesta del nivel: %d %d\n",recActual,rec.x,rec.y);
						printf("Llego la posicion del recurso solicitado del nivel\n");
					}

				}

			}

		}
		//esperar MovPermitido
		sleep(1);//todo sacar
	}//fin while(llego)
}//fin for each recurso

}//fin for each nivel
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
