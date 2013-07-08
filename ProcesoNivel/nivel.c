/*
 * nivel.c
 *
 *  Created on: 29/04/2013
 *      Author: utnso
 */
#include "config.h"
#include "log.h"
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
#include <socketsOv.h>
#include <pthread.h>
#include "tad_items.h" //aca ya se incluye el #include "nivel.h"

static int rows,cols;

typedef struct t_posicion {
	int8_t x;
	int8_t y;
} Posicion;

typedef struct t_datap {
int socket;
ITEM_NIVEL * nodo;
} DataP;

ITEM_NIVEL * listaItems = NULL;

t_log * logger;

void handler(DataP* dataPer);
int listenear(void);
void sacarInfoCaja(char * caja, char* id, int* x , int* y, int* cant);
//1) El proceso nivel crea 1 lista (global)
//que tiene personajes e items

//2) Listenea personajes para conectarse
//cuando conectan:
//-Crea el thread correspondiente
//-Le pide el simbolo que usa
//-Con el simbolo, lo agrega a la lista de personajes

//3) El personaje le pide proximo recurso
//Nivel busca en la lista de recursos
//y le pasa la posicion del recurso

//4) El personaje avisa a nivel que se mueve
//Nivel va actualizando la posicion
//y chequea que este dentro de los margenes (rows,cols)

//5) El personaje esta en el recurso y solicita instancia
//Verifica que el personaje este en ese lugar
//Nivel busca en la lista de recursos si tiene
//instancia de ese recurso. Si la tiene, se la da
//y actualiza las instancias disponibles de ese recurso
//si no tiene, le avisa que no tiene

//6) El personaje se desconecta (por muerte o porque completa nivel)
//Nivel notifica a Orquestador instancias a liberar
//Nivel libera instancias que tenia el personaje
//Lo saca de la lista

//7) Chequeo de interbloqueo
//thread adicional que cada cierto tiempo (configurable)
//chequee que un personaje este bloqueado (lo veremos despues esto)
int main(void){
	//	Leer el config y cargar los recursos
	//	CrearCaja(&listaItems,'F',10,14,7);
	//	CrearCaja(&listaItems,'H',17,6,3);
	t_config* configNivel = config_create("config.txt");
	char *varstr;
	varstr=malloc(8);
	strcpy(varstr,"Caja");
	char pal[4];
	int cantKeys = config_keys_amount(configNivel)-4;
	int x;
	for (x=1;x<=cantKeys;x++)
		{
			sprintf(pal, "%d", x);
			strcpy(varstr+4,pal);
			if(config_has_property(configNivel,varstr))
			{
				char *stringCompleto = config_get_string_value(configNivel,varstr);
				char id;
				int posx, posy, instancias;
				sacarInfoCaja(stringCompleto, &id, &posx, &posy, &instancias);
				CrearCaja(&listaItems, id, posx, posy, instancias);
			}
		}

	//Crea archivo log
	//Si existe lo abre, sino, lo crea
	//Con trace va a poder loguear t-o-d-o
	t_log_level detail = LOG_LEVEL_TRACE;

	//LogNivel = Nombre de archivo log
	//ProcesoNivel = Nombre del proceso
	//false = Que no aparezca en pantalla los logs
	//detail = Detalle con el que se va a loguear (TRACE,INFO,DEBUG,etc)
	logger = log_create("LogNivel.log","ProcesoNivel",false,detail);

//	t_config* config=config_create("config.txt");
	nivel_gui_inicializar();
	nivel_gui_get_area_nivel(&rows, &cols);

//listenear

	pthread_t threadListener;
	pthread_create(&threadListener, NULL, listenear, NULL);






		while(1){

			nivel_gui_dibujar(listaItems);
			sleep(1);

		}

		sleep(3);
nivel_gui_terminar();
return 0;
}


//handler de cada personaje recibe un struct con el socket y el puntero a su nodo
void handler(DataP* dataPer)
{
void* buffer;
int resp;
Posicion posAux;
char* carAux;
carAux=malloc(1);
Header unHeader;
ITEM_NIVEL* nodoAux;
log_info(logger,"Llego el socket %d (Thread)", dataPer->socket);

while(1){

	// todos los returns son para handlear el tema de desconexion del cliente sin aviso
	if(!recibirHeader(dataPer->socket,&unHeader)) {close(dataPer->socket);return;}
	log_debug(logger,"Llego Msj tipo %d payload %d", unHeader.type,unHeader.payloadlength);
	switch(unHeader.type){
	case 0:
		if(!recibirData(dataPer->socket,unHeader, (void**)carAux)) {close(dataPer->socket);return;}
		//TODO: borrar
		//printf("Llego el Personaje %c al nivel",*carAux);
		log_info(logger,"Llego el Personaje %c al nivel(Thread)", *carAux);
		if (mandarMensaje(dataPer->socket,0 , 1,carAux))
		{
			printf("a");
		}else {close(dataPer->socket);return;}
			break;
	case 1:
		resp=recibirData(dataPer->socket,unHeader, (void**)carAux);
		if(!resp) {close(dataPer->socket);return;}
		log_debug(logger,"Resp del recData: %d",resp);
		//TODO Borrar
		//printf("El Personaje %c solicita la Posición del Recurso %c\n",dataPer.nodo->id,*carAux);
		log_info(logger,"El Personaje %c solicita la Posición del Recurso %c\n",dataPer->nodo->id,*carAux);
		nodoAux=obtenerRecurso(listaItems, *carAux);
		posAux.x=nodoAux->posx;
		posAux.y=nodoAux->posy;
		buffer=&posAux;
		if(mandarMensaje(dataPer->socket,1 , sizeof(Posicion),buffer)){
			//TODO Borrar
			//printf("Se mando la pos(%d,%d) del Rec %c al Personaje %c\n",posAux.x,posAux.y,*carAux,dataPer.nodo->id);
			log_info(logger,"Se mando la pos(%d,%d) del Rec %c al Personaje %c\n",posAux.x,posAux.y,*carAux,dataPer->nodo->id);
		}else {close(dataPer->socket);return;}
		break;
	case 2:
		if(!recibirData(dataPer->socket, unHeader, (void**)&posAux)) {close(dataPer->socket);return;}
		dataPer->nodo->posx=posAux.x;
		dataPer->nodo->posy=posAux.y;
		//TODO Borrar
		//printf("Se recibió la posición(%d,%d) del Personaje %c\n",posAux.x,posAux.y,dataPer.nodo->id);
		log_info(logger,"Se recibió la posición(%d,%d) del Personaje %c\n",posAux.x,posAux.y,dataPer->nodo->id);
		carAux=malloc(1);
		carAux[0]='K';
		if (mandarMensaje(dataPer->socket,4 , sizeof(char),(void*)carAux)) {
			//TODO Borrar
			//printf("Se le aviso al Personaje %c que llego bien su Posición",dataPer.nodo->id);
			log_info(logger,"Se le aviso al Personaje %c que llego bien su Posición",dataPer->nodo->id);
		}else {close(dataPer->socket);return;}
		break;
	case 3:
		if(!recibirData(dataPer->socket, unHeader, (void**)carAux)) {close(dataPer->socket);return;}
		//TODO Borrar
		//printf("El Personaje %c solicita una Instancia del Recurso %c\n",dataPer.nodo->id,*carAux);
		log_info(logger,"El Personaje %c solicita una Instancia del Recurso %c\n",dataPer->nodo->id,*carAux);
		if(restarRecurso(listaItems, *carAux)>0)
		{
			//TODO Borrar
			//printf("Hay instancias del recurso %c y se le dio una al Personaje %c\n",*carAux,dataPer.nodo->id);
			log_info(logger,"Hay instancias del recurso %c y se le dio una al Personaje %c\n",*carAux,dataPer->nodo->id);
			nodoAux=obtenerRecurso(listaItems, *carAux);
			posAux.x=nodoAux->posx;
			posAux.y=nodoAux->posy;
			buffer=&posAux;
			if(mandarMensaje(dataPer->socket,1 , sizeof(Posicion),buffer)){
				//TODO Borrar
				//printf("Se mando la pos(%d,%d) del Rec %c\n",posAux.x,posAux.y,*carAux);
				log_info(logger,"Se mando la pos(%d,%d) del Rec %c\n",posAux.x,posAux.y,*carAux);
			}else {close(dataPer->socket);return;}

		}else{
			//TODO Borrar
			//printf("No hay instancias del recurso %c y no se le dio una al Personaje %c",*carAux,dataPer.nodo->id);
			log_info(logger,"No hay instancias del recurso %c y no se le dio una al Personaje %c",*carAux,dataPer->nodo->id);
			posAux.x=dataPer->nodo->posx;
			posAux.y=dataPer->nodo->posy;
			buffer=&posAux;
			if(mandarMensaje(dataPer->socket,1 , sizeof(Posicion),buffer)){
				//TODO Borrar
				//printf("Se mando la posActual(%d,%d) del Personaje %c al Personaje %c\n",posAux.x,posAux.y,dataPer.nodo->id,dataPer.nodo->id);
				log_info(logger,"Se mando la posActual(%d,%d) del Personaje %c al Personaje %c\n",posAux.x,posAux.y,dataPer->nodo->id,dataPer->nodo->id);
			} else {close(dataPer->socket);return;}
		}
		break;
	case 4:
		//todo sincronizar
		log_info(logger,"El Personaje %c solicita salir del nivel.",(dataPer->nodo)->id);
		BorrarItem(listaItems,(dataPer->nodo)->id);
		log_info(logger,"El personaje salio del Nivel.");
		close(dataPer->socket);
		log_debug(logger,"Se cerro el Socket: %d",dataPer->socket);
		return;
		break;
	default:
			break;
	}

}

}


int listenear(void){

    int socketEscucha,socketNuevaConexion;
    socketEscucha=quieroUnPutoSocketDeEscucha(5000);

            while (1){
            	// Escuchar nuevas conexiones entrantes.
            if (listen(socketEscucha, 1) != 0) {

                log_error(logger,"Error al bindear socket escucha");
                return EXIT_FAILURE;
            }

            	log_info(logger,"Escuchando conexiones entrantes");

                // Aceptar una nueva conexion entrante. Se genera un nuevo socket con la nueva conexion.
                // La funci贸n accept es bloqueante, no sigue la ejecuci贸n hasta que se reciba algo
                if ((socketNuevaConexion = accept(socketEscucha, NULL, 0)) < 0) {

                	log_error(logger,"Error al aceptar conexion entrante");
                    return EXIT_FAILURE;
                }

                //Handshake en el que recibe la letra del personaje
                char* rec;
                if(recibirMensaje(socketNuevaConexion, (void**)&rec)>=0) {

                	log_info(logger,"Llego el Personaje %c del nivel",*rec);

                    if (mandarMensaje(socketNuevaConexion,0 , 1,rec)) {
                    	log_info(logger,"Mando mensaje al personaje %c",*rec);

                    }

                    DataP* personaje;
                    personaje=malloc(sizeof(DataP));

                    personaje->socket = socketNuevaConexion;

                    //Agrega personaje a la lista y devuelve nodo
                    personaje->nodo = CrearPersonaje(&listaItems, *rec, 1 ,1);
                    log_info(logger,"Mando el socket %d (Thread)", personaje->socket);
                    //TODO Fede vos sabes que hacer
                    pthread_t threadPersonaje;

                    pthread_create(&threadPersonaje, NULL, handler, (void *)personaje);

                }



            }
    return 1;
}

void sacarInfoCaja(char * caja, char* id, int* x , int* y, int* cant)
{
	char ** vecStr;
	char* aux;
	vecStr=string_split(caja, ",");
	*id=*vecStr[1];
	*x=(int)strtol(vecStr[2], &aux, 10);
	*y=(int)strtol(vecStr[3], &aux, 10);
	*cant=(int)strtol(vecStr[4], &aux, 10);
	}
