/*
 * personaje.c
 *
 *  Created on: 29/04/2013
 *      Author: utnso
 */

#include <string.h>
#include "string.h"
#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <sys/ioctl.h>
#include "socketsOv.h"
#include "config.h"
#include "log.h"
#include <signal.h> //libreria de señales
#include <sys/types.h>

typedef struct t_posicion {
	int8_t x;
	int8_t y;
} Posicion;

typedef struct t_infoNP {
		char ipN[16];
		int portN;
		char ipP[16];
		int portP;
	} IPNivPlan;
typedef struct t_msjPer {
	int pidioRec;
	int blocked;
	int finNivel;
} MsjPer;

//static int rows,cols;
Posicion pos;
Posicion rec;
int vidas;
char recActual;
char charPer;
int lengthVecConfig(char * value);

//El manejador de señales hay que ponerlo ACA ARRIBA.
void manejador (int sig){
	switch (sig){
	case SIGINT:
	case SIGTERM:
		//Se debe notificar al nivel el motivo de la muerte y liberar recursos
		if (vidas>0){
			vidas--;
			printf("Has perdido una vida.\n Vidas restantes: %d\n",vidas);
		}
		else
		{
			vidas++;
			//Reiniciar el plan de niveles
		}
		break;
	case SIGUSR1:
		vidas++;
		break;
}
}


int main(void){
	//Señales
	signal(SIGTERM,manejador);
	signal(SIGINT,manejador);
	signal(SIGUSR1,manejador);
	t_config* config=config_create("config.txt");

	//Se inicializan las variables para el logueo
	t_log_level detail = LOG_LEVEL_INFO;
	t_log * log = log_create("LogPersonaje.log","Personaje",false,detail);

	//obtener recurso de config
		char** obj;
		char** niveles;
		char** ipPuertoOrq;
		char* val;
		char* aux;
		char* aux1;
		char objNivel[14]="obj[Nivel1]";
		char corchete[2]="]";
		int veclong, llego, cantNiv,numNiv, c,ii,puertoOrq;
		Header h;

		niveles=config_get_array_value(config,"planDeNiveles");
		vidas=config_get_int_value(config,"vidas");
		charPer=config_get_string_value(config,"simbolo")[0];
		cantNiv=config_keys_amount(config)-5/*cant de keys fijas*/;
		log_info(log,"Se obtuvieron los recursos del config");
		aux1=config_get_string_value(config,"orquestador");
		ipPuertoOrq=string_split(aux1, ":");
		puertoOrq=(int)strtol(ipPuertoOrq[1], &aux, 10);


	for(c=0;c<cantNiv;c++){ //for each nivel

			memcpy(objNivel+4,niveles[c],strlen(niveles[c]));
			memcpy(objNivel+4+strlen(niveles[c]),corchete,2);
			numNiv=(int)strtol(niveles[c]+5, &aux, 10);
			obj=config_get_array_value(config,objNivel);
			val=config_get_string_value(config,objNivel);
			veclong=lengthVecConfig(val);
			pos.x=0;
			pos.y=1;

		//TODO conexion solicitud de ip:puerto al orquestator y cierre de esa conex
			int unSocketOrq;
			unSocketOrq = quieroUnPutoSocketAndando(ipPuertoOrq[0],puertoOrq);
			char* auxC;
			auxC=malloc(sizeof(char));
			*auxC=charPer;
			if (mandarMensaje(unSocketOrq ,0 , 1,auxC)) {
				if(recibirMensaje(unSocketOrq,(void**)&auxC)>=0) {
					printf("msj recibido from handshake %c\n",*auxC);
				}
			}

			Header unHeader;
			int* intaux;
			intaux=malloc(sizeof(int));
			*intaux=numNiv;
			IPNivPlan ipNivelPlanif;
			//esperar solicitud de info nivel/Planif
			mandarMensaje(unSocketOrq,1,sizeof(int),intaux);
			if(recibirHeader(unSocketOrq,&unHeader)){
				if(recibirData(unSocketOrq,unHeader,(void**)&ipNivelPlanif)){
				//Obtener info de ip & port
					printf("%s %d %s %d\n",ipNivelPlanif.ipN,ipNivelPlanif.portN,ipNivelPlanif.ipP,ipNivelPlanif.portP);
				}
			}

			close(unSocketOrq);


			//conectar con Planificador
			int unSocketPlanif;
			unSocketPlanif = quieroUnPutoSocketAndando(ipNivelPlanif.ipP,ipNivelPlanif.portP);
			log_info(log,"Se creo un nuevo socket con el Planificador. Direccion: %s // Puerto: %d // Socket: %d",ipNivelPlanif.ipP,ipNivelPlanif.portP,unSocketPlanif);
			char* charbuf;
			charbuf=malloc(1);
			*charbuf=charPer;
			//handshake
			if (mandarMensaje(unSocketPlanif,0 , 1,charbuf)) {
				log_info(log,"Se envío el Handshake al planificador");
				if(recibirMensaje(unSocketPlanif, (void**)&charbuf)>=0) {
					log_info(log,"Llego el Handshake del Planificador: %c",*charbuf);
				}
				else {
					log_error(log,"No llego al cliente la confirmacion del Planificador (handshake)");
				}
			} else {
				log_error(log,"No llego al planif la confirmacion del personaje (handshake)");
			}


			//conectar con nivel
			int unSocket;
			unSocket = quieroUnPutoSocketAndando(ipNivelPlanif.ipN,ipNivelPlanif.portN);
			log_info(log,"Se creo un nuevo socket. Direccion: %s // Puerto: %d // Socket: %d",ipNivelPlanif.ipN,ipNivelPlanif.portN,unSocket);
			*charbuf=charPer;

			if (mandarMensaje(unSocket,0 , 1,charbuf)) {
				printf("Llego el OK al nivel\n");
				log_info(log,"Llego el OK al nivel");
				if(recibirMensaje(unSocket, (void**)&charbuf)>=0) {
					printf("Llego el OK del nivel char %c\n",*charbuf);
					log_info(log,"Llego el OK del nivel char %c",*charbuf);
				}
				else {
					printf("No llego al cliente la confirmacion del nivel (handshake)\n");
					log_error(log,"No llego al cliente la confirmacion del nivel (handshake)");
				}
			} else {
				printf("No llego al nivel la confirmacion del personaje (handshake)\n");
				log_error(log,"No llego al nivel la confirmacion del personaje (handshake)");
			}


//for(ii=0;ii<veclong;ii++)printf("recurso %c",*obj[ii]);

void* buffer;
recActual=*obj[0];
buffer= &recActual;

if (mandarMensaje(unSocket,1, sizeof(char),buffer)) {
	printf("Llego el header de la posicion a recurso al nivel\n");
	log_info(log,"Llego el header de la posicion a recurso al nivel");
	printf("Llego el recurso actual necesario al nivel\n");
	log_info(log,"Llego el recurso actual necesario al nivel");
	Header unHeader;
	if (recibirHeader(unSocket,&unHeader)) {
		printf("pos %d %d %d %d\n",pos.x,pos.y,unHeader.payloadlength,unHeader.type);
		log_info(log,"pos %d %d %d %d",pos.x,pos.y,unHeader.payloadlength,unHeader.type);
		Posicion lifeSucks;
		recibirData(unSocket,unHeader,(void**)&lifeSucks);
		rec=lifeSucks;
		printf("Llego %c header respuesta del nivel: %d %d\n",recActual,rec.x,rec.y);
		log_info(log,"Llego %c header respuesta del nivel: %d %d",recActual,rec.x,rec.y);
		printf("Llego la posicion del recurso solicitado del nivel\n");
		log_info(log,"Llego el header de la posicion a recurso al nivel");
	}
}


for(ii=0;ii<veclong;ii++){
	recActual=*obj[ii];
	//solicitar posicion recurso Recurso: recactual
	//esperar posicion del recurso posicion : rec
	llego=1;

	while(llego){

		// esperar mensaje de movPermitido para continuar
		char* charAux;
		recibirHeader(unSocketPlanif,&unHeader);
		if(unHeader.type==8) recibirData(unSocketPlanif,unHeader,(void**)&charAux);
		MsjPer respAlPlanf;
		respAlPlanf.blocked=0;
		respAlPlanf.pidioRec=0;
		respAlPlanf.finNivel=0;
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
			log_info(log,"Llego el header de la posicion del personaje %d %d",pos.x,pos.y);
			if (recibirMensaje(unSocket, (void**)&sth)>=0) {
				printf("Llego header respuesta: %c del nivel\n",*sth);
				log_info(log,"Llego header respuesta: %c del nivel",*sth);
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
				log_info(log,"Llego el header de peticion de recurso %c al nivel",recActual);
				printf("Llego el buffer del recurso necesario al nivel\n");
				log_info(log,"Llego el buffer del recurso necesario al nivel");
				Header unHeader;
				Posicion lifeSucks;

				if (recibirHeader(unSocket,&unHeader)) {
					printf("Llego header%d %d respuesta del nivel\n", unHeader.payloadlength,unHeader.type);
					log_info(log,"Llego header%d %d respuesta del nivel", unHeader.payloadlength,unHeader.type);
					recibirData(unSocket,unHeader,(void**)&lifeSucks);
					//rec=lifeSucks;
					respAlPlanf.pidioRec=1;
					printf("Llego la confirmacion del recurso del nivel %d  %d\n",rec.x,rec.y);
					log_info(log,"Llego la confirmacion del recurso del nivel %d  %d",rec.x,rec.y);

				}
			}

			if(ii+1==veclong) {
				//cerrar conexion con el nivel
				mandarMensaje(unSocket,4 , sizeof(char),&recActual);
				respAlPlanf.finNivel=1;
				//exit(0);
			}else {
				recActual=*obj[ii+1];
				buffer=&recActual;

				if (mandarMensaje(unSocket,1, sizeof(char),buffer)) {
					printf("Llego el header de la posicion a recurso al nivel\n");
					log_info(log,"Llego el header de la posicion a recurso al nivel");
					printf("Llego el recurso actual necesario al nivel\n");
					log_info(log,"Llego el recurso actual necesario al nivel");
					Header unHeader;

					if (recibirHeader(unSocket,&unHeader)) {
						printf("pos %d %d %d %d\n",pos.x,pos.y,unHeader.payloadlength,unHeader.type);
						log_info(log,"pos %d %d %d %d",pos.x,pos.y,unHeader.payloadlength,unHeader.type);
						Posicion lifeSucks;
						recibirData(unSocket,unHeader,(void**)&lifeSucks);
						rec=lifeSucks;
						printf("Llego %c header respuesta del nivel: %d \n",recActual,rec.x,rec.y);
						log_info(log,"Llego %c header respuesta del nivel: %d",recActual,rec.x,rec.y);
						printf("Llego la posicion del recurso solicitado del nivel\n");
						log_info(log,"Llego la posicion del recurso solicitado del nivel");
					}

				}

			}

		}
		//mandar mensaje de resp al Planif
		mandarMensaje(unSocketPlanif,8,sizeof(MsjPer),&respAlPlanf);
//		sleep(1);//todo sacar
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


