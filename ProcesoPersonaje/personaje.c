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
#include <unistd.h>
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

typedef struct t_infoConxNivPlan {
	char ipNivel[16];
	int portNivel;
	char ipPlanificador[16];
	int portPlanificador;
} ConxNivPlan;

typedef struct t_msjPersonaje {
	int solicitaRecurso;
	int bloqueado;
	int finNivel;
} MensajePersonaje;

//Var Globales;
Posicion pos;
Posicion rec;
int vidas;
char recActual;
char charPer;
int lengthVecConfig(char * value);
int seMurio=0;
t_config* config;

//El manejador de señales
void manejador (int sig){
	switch (sig){
	case SIGINT:
	case SIGTERM:
		//Se debe notificar al nivel el motivo de la muerte y liberar recursos
		if (vidas>0){
			vidas--;
			printf("Has perdido una vida.\n Vidas restantes: %d\n",vidas);
			seMurio=1;
		}
		else
		{
			vidas=config_get_int_value(config,"vidas");
			seMurio=2;
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
	config=config_create("config.txt");

	//Se inicializan las variables para el logueo
	t_log_level detail = LOG_LEVEL_TRACE;
	t_log * log = log_create("LogPersonaje.log","Personaje",false,detail);

	//obtener recurso de config
		char** obj;
		char** niveles;
		char** ipPuertoOrq;
		char* val;
		char* aux;
		char* aux1;
		char* nivelActual;
		char objNivel[20]="obj[Nivel1]";
		char corchete[2]="]";
		int veclong, llego, cantNiv,c,ii,puertoOrq;
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

//			numNiv=(int)strtol(niveles[c]+5, &aux, 10);
			nivelActual=niveles[c];
			obj=config_get_array_value(config,objNivel);
			val=config_get_string_value(config,objNivel);
			veclong=lengthVecConfig(val);
			pos.x=0;
			pos.y=1;

		ConxNivPlan ipNivelPlanif;
		ipNivelPlanif.portNivel=0;
		while(ipNivelPlanif.portNivel==0){//checkea q el nivel haya llegado al planif y sino entra en un ciclo hasta que entre
		//conexion solicitud de ip:puerto al orquestator y cierre de esa conex
			int unSocketOrq;
			unSocketOrq = quieroUnPutoSocketAndando(ipPuertoOrq[0],puertoOrq);
			char* auxC;
			auxC=malloc(sizeof(char));
			*auxC=charPer;
			//handshake Orquestador-Personaje
			if (mandarMensaje(unSocketOrq ,0 , 1,auxC)) {
				if(recibirMensaje(unSocketOrq,(void**)&auxC)>=0) {
					printf("msj recibido from handshake %c\n",*auxC);
				}
			}

			Header unHeader;
//			int* intaux;
//			intaux=malloc(sizeof(int));
//			*intaux=numNiv;

			//esperar solicitud de info nivel/Planif
//			mandarMensaje(unSocketOrq,1,sizeof(int),intaux);
			mandarMensaje(unSocketOrq,1,strlen(nivelActual)+1,nivelActual);
			printf("%s",nivelActual);
			log_debug(log,"nivelActual: %s",nivelActual);
			if(recibirHeader(unSocketOrq,&unHeader)){
				if(recibirData(unSocketOrq,unHeader,(void**)&ipNivelPlanif)){
				//Obtener info de ip & port
					printf("%s %d %s %d\n",ipNivelPlanif.ipNivel,ipNivelPlanif.portNivel,ipNivelPlanif.ipPlanificador,ipNivelPlanif.portPlanificador);
				}
			}

			close(unSocketOrq);
			usleep(1*1000000);
	}
			//conectar con Planificador
			int unSocketPlanif;
			unSocketPlanif = quieroUnPutoSocketAndando(ipPuertoOrq[0],ipNivelPlanif.portPlanificador);
			log_info(log,"Se creo un nuevo socket con el Planificador. Direccion: %s // Puerto: %d // Socket: %d",ipPuertoOrq[0],ipNivelPlanif.portPlanificador,unSocketPlanif);
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
			unSocket = quieroUnPutoSocketAndando(ipNivelPlanif.ipNivel,ipNivelPlanif.portNivel);
			log_info(log,"Se creo un nuevo socket. Direccion: %s // Puerto: %d // Socket: %d",ipNivelPlanif.ipNivel,ipNivelPlanif.portNivel,unSocket);
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
		Header unHeader;
		// esperar mensaje de movPermitido para continuar
		char* charAux;

		printf("Esperando permiso de moviemiento...");
		if(recibirHeader(unSocketPlanif,&unHeader)>0){
		if(unHeader.type==8)
			recibirData(unSocketPlanif,unHeader,(void**)&charAux);
		printf("Permiso data Recibido");
		}

		MensajePersonaje respAlPlanf;
		respAlPlanf.bloqueado=0;
		respAlPlanf.solicitaRecurso=0;
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
					int * respRec;
					respRec=malloc(sizeof(int));
					recibirData(unSocket,unHeader,(void**)respRec);
					printf("Llego respuesta %d del nivel\n", *respRec);
					log_info(log,"Llego respuesta %d del nivel",*respRec);
					//rec=lifeSucks;
					respAlPlanf.solicitaRecurso=1;
					if(*respRec){
						respAlPlanf.bloqueado=0;
						printf("Se entrego una instancia del Recurso %c\n",recActual);
						log_info(log,"Se entrego una instancia del Recurso %c",recActual);
					}else{
						respAlPlanf.bloqueado=1;
						printf("No se entrego una instancia del Recurso %c\n",recActual);
						log_info(log,"No se entrego una instancia del Recurso %c",recActual);
					}
					free(respRec);

				}
			}

			if((ii+1==veclong)) {
				//cerrar conexion con el nivel
				mandarMensaje(unSocket,4 , sizeof(char),&recActual);
				respAlPlanf.finNivel=1;
				if(seMurio) {c--;ii=veclong;llego=0;seMurio=0;} //reiniciar el mismo nivel
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
		if(seMurio>0) {
		//cerrar conexion con el nivel
		mandarMensaje(unSocket,4 , sizeof(char),&recActual);
		respAlPlanf.finNivel=1;
		c--;
		if(seMurio==2) c=-1;
		llego=0;
		printf("Se murio %d %d %d\n",ii,c,llego);
		ii=veclong;
		printf("Se murio %d\n",ii);
		seMurio=0;
		}
		mandarMensaje(unSocketPlanif,8,sizeof(MensajePersonaje),&respAlPlanf);
		printf("Se envio respuesta de turno cloncluido al Planificador\n");
	}//fin while(llego)
	printf("Fin for\n");
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


