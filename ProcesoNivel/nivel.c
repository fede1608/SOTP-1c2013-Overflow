/*
 * nivel.c
 *
 *  Created on: 29/04/2013
 *      Author: utnso
 */
#include "config.h"
#include "nivel.h"
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
#include  <socketsCom/socketsOv.h>

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

void handler(DataP dataPer);
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
	void* buffer;

	//Crea archivo log
	//Si existe lo abre, sino, lo crea
	//Con trace va a poder loguear t-o-d-o
	t_log_level detail = LOG_LEVEL_TRACE;

	//LogNivel = Nombre de archivo log
	//ProcesoNivel = Nombre del proceso
	//false = Que no aparezca en pantalla los logs
	//detail = Detalle con el que se va a loguear (TRACE,INFO,DEBUG,etc)
	logger = log_create("LogNivel","ProcesoNivel",false,detail);

//	t_config* config=config_create("config.txt");
	nivel_gui_inicializar();
	nivel_gui_get_area_nivel(&rows, &cols);

//listenear
	int socketEscucha, socketNuevaConexion;

		struct sockaddr_in socketInfo;
		int optval = 1;

		//NO PIENSO LOGUEAR ESTO SI YA LO HACE EL LISTENER

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
				//TODO Borrar
				//printf("pos rec");
				log_info(logger,"pos rec");
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
						//TODO Borrar
						//printf("char %c",*rec);
						log_info(logger,"char %c",*rec);
						flor->quantity--;
						if(*rec=='F'){posRec.x=flor->posx;posRec.y=flor->posy;}
						if(*rec=='H'){posRec.x=hongo->posx;posRec.y=hongo->posy;}
						//TODO Borrar
						//printf("posEnv: %d %d ",posRec.x,posRec.y);
						log_info(logger,"posEnv: %d %d ",posRec.x,posRec.y);
						if (mandarMensaje(socketNuevaConexion,3 , sizeof(Posicion),&posRec)) {

						}
					}
			else{
				//tipo2
				char* sth;
				recibirData(socketNuevaConexion, headMen, (void**)&posPer);
				pj1->posx=posPer.x;
				pj1->posy=posPer.y;
				//TODO Borrar
				//printf("px: %d py: %d rx: %d ry:%d",posPer.x,posPer.y,posRec.x,posRec.y);
				log_info(logger,"px: %d py: %d rx: %d ry:%d",posPer.x,posPer.y,posRec.x,posRec.y);
				sth=malloc(1);
				sth[0]='k';
				if (mandarMensaje(socketNuevaConexion,4 , sizeof(char),(void*)sth)) {
					//TODO Borrar
					//printf("resp pos %c %d %d \n",flor->id,flor->posx,flor->posy);
					log_info(logger,"resp pos %c %d %d \n",flor->id,flor->posx,flor->posy);
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
//handler de cada personaje recibe un struct con el socket y el puntero a su nodo
void handler(DataP dataPer)
{
	void* buffer;
	Posicion posAux;
	char* carAux;
	Header unHeader;
	ITEM_NIVEL* nodoAux;
while(1){
	recibirHeader(dataPer.socket,&unHeader);

	switch(unHeader.type){
	case 0:
		recibirData(dataPer.socket,unHeader, (void**)&carAux);
		//TODO: borrar
		//printf("Llego el Personaje %c al nivel",*carAux);
		log_info(logger,"Llego el Personaje %c al nivel", *carAux);
		if (mandarMensaje(dataPer.socket,0 , 1,carAux))
		{
			printf("a");
		}
			break;
	case 1:
		recibirData(dataPer.socket,unHeader, (void**)&carAux);
		//TODO Borrar
		//printf("El Personaje %c solicita la Posición del Recurso %c\n",dataPer.nodo->id,*carAux);
		log_info(logger,"El Personaje %c solicita la Posición del Recurso %c\n",dataPer.nodo->id,*carAux);
		nodoAux=obtenerRecurso(listaItems, *carAux);
		posAux.x=nodoAux->posx;
		posAux.y=nodoAux->posy;
		buffer=&posAux;
		if(mandarMensaje(dataPer.socket,1 , sizeof(Posicion),buffer))
			//TODO Borrar
			//printf("Se mando la pos(%d,%d) del Rec %c al Personaje %c\n",posAux.x,posAux.y,*carAux,dataPer.nodo->id);
			log_info(logger,"Se mando la pos(%d,%d) del Rec %c al Personaje %c\n",posAux.x,posAux.y,*carAux,dataPer.nodo->id);
		break;
	case 2:
		recibirData(dataPer.socket, unHeader, (void**)&posAux);
		dataPer.nodo->posx=posAux.x;
		dataPer.nodo->posy=posAux.y;
		//TODO Borrar
		//printf("Se recibió la posición(%d,%d) del Personaje %c\n",posAux.x,posAux.y,dataPer.nodo->id);
		log_info(logger,"Se recibió la posición(%d,%d) del Personaje %c\n",posAux.x,posAux.y,dataPer.nodo->id);
		carAux=malloc(1);
		carAux[0]='K';
		if (mandarMensaje(dataPer.socket,4 , sizeof(char),(void*)carAux)) {
			//TODO Borrar
			//printf("Se le aviso al Personaje %c que llego bien su Posición",dataPer.nodo->id);
			log_info(logger,"Se le aviso al Personaje %c que llego bien su Posición",dataPer.nodo->id);
		}
		break;
	case 3:
		recibirData(dataPer.socket, unHeader, (void**)&carAux);
		//TODO Borrar
		//printf("El Personaje %c solicita una Instancia del Recurso %c\n",dataPer.nodo->id,*carAux);
		log_info(logger,"El Personaje %c solicita una Instancia del Recurso %c\n",dataPer.nodo->id,*carAux);
		if(restarRecurso(listaItems, *carAux)>0)
		{
			//TODO Borrar
			//printf("Hay instancias del recurso %c y se le dio una al Personaje %c\n",*carAux,dataPer.nodo->id);
			log_info(logger,"Hay instancias del recurso %c y se le dio una al Personaje %c\n",*carAux,dataPer.nodo->id);
			nodoAux=obtenerRecurso(listaItems, *carAux);
			posAux.x=nodoAux->posx;
			posAux.y=nodoAux->posy;
			buffer=&posAux;
			if(mandarMensaje(dataPer.socket,1 , sizeof(Posicion),buffer))
				//TODO Borrar
				//printf("Se mando la pos(%d,%d) del Rec %c\n",posAux.x,posAux.y,*carAux);
				log_info(logger,"Se mando la pos(%d,%d) del Rec %c\n",posAux.x,posAux.y,*carAux);
		}else{
			//TODO Borrar
			//printf("No hay instancias del recurso %c y no se le dio una al Personaje %c",*carAux,dataPer.nodo->id);
			log_info(logger,"No hay instancias del recurso %c y no se le dio una al Personaje %c",*carAux,dataPer.nodo->id);
			posAux.x=dataPer.nodo->posx;
			posAux.y=dataPer.nodo->posy;
			buffer=&posAux;
			if(mandarMensaje(dataPer.socket,1 , sizeof(Posicion),buffer))
				//TODO Borrar
				//printf("Se mando la posActual(%d,%d) del Personaje %c al Personaje %c\n",posAux.x,posAux.y,dataPer.nodo->id,dataPer.nodo->id);
				log_info(logger,"Se mando la posActual(%d,%d) del Personaje %c al Personaje %c\n",posAux.x,posAux.y,dataPer.nodo->id,dataPer.nodo->id);
		}
		break;
	default:
			break;
	}

}

}


int listenear(void){

    int socketEscucha ,socketNuevaConexion;

            struct sockaddr_in socketInfo;
            int optval = 1;

            // Crear un socket:
            // AF_INET: Socket de internet IPv4
            // SOCK_STREAM: Orientado a la conexion, TCP
            // 0: Usar protocolo por defecto para AF_INET-SOCK_STREAM: Protocolo TCP/IPv4
            if ((socketEscucha = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            	//TODO Borrar
            	//perror("Error al crear el socket");
            	log_error(logger,"Error al crear el socket");
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

            	//TODO Borrar
                //perror("Error al bindear socket escucha");
                log_error(logger,"Error al bindear socket escucha");
                return EXIT_FAILURE;
            }

        // Escuchar nuevas conexiones entrantes.
            if (listen(socketEscucha, 1) != 0) {
                //TODO Borrar
            	//perror("Error al poner a escuchar socket");
                log_error(logger,"Error al bindear socket escucha");
                return EXIT_FAILURE;
            }

            while (1){
            	//TODO Borrar
            	//printf("Escuchando conexiones entrantes.\n");
            	log_info(logger,"Escuchando conexiones entrantes");

                // Aceptar una nueva conexion entrante. Se genera un nuevo socket con la nueva conexion.
                // La funci贸n accept es bloqueante, no sigue la ejecuci贸n hasta que se reciba algo
                if ((socketNuevaConexion = accept(socketEscucha, NULL, 0)) < 0) {

                	//TODO Borrar
                	//perror("Error al aceptar conexion entrante");
                	log_error(logger,"Error al aceptar conexion entrante");
                    return EXIT_FAILURE;
                }

                //Handshake en el que recibe la letra del personaje
                char* rec;
                if(recibirMensaje(socketNuevaConexion, (void**)&rec)>=0) {

                    //TODO Borrar
                	//printf("Llego el Personaje %c del nivel",*rec);
                	log_info(logger,"Llego el Personaje %c del nivel",*rec);

                    if (mandarMensaje(socketNuevaConexion,0 , 1,rec)) {
                    	//??????
                    	//TODO Borrar
                    	//printf("a");
                    	log_info(logger,"a");

                    }

                    DataP personaje;

                    personaje.socket = socketNuevaConexion;

                    //Agrega personaje a la lista y devuelve nodo
                    personaje.nodo = CrearPersonaje(&listaItems, *rec, 0 ,0);

                    //TODO Fede vos sabes que hacer
                    pthread_t threadPersonaje;
                    pthread_create(&threadPersonaje, NULL, handler, &personaje);

                }



            }
    return 1;
}

void sacarInfoCaja(char * caja, char* id, int* x , int* y, int* cant){
	char ** vecStr;
	char* aux;
	vecStr=string_split(caja, ",");
	*id=*vecStr[1];
	*x=(int)strtol(vecStr[2], &aux, 10);
	*y=(int)strtol(vecStr[3], &aux, 10);
	*cant=(int)strtol(vecStr[4], &aux, 10);
	}
