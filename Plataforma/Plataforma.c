/************************** DESCRIPCIÓN **************************
 * Plataforma.c
 *
 *    Creado el: 14/06/2013
 *        Autor: Grupo Overflow
 *
 *  Descripción: Proceso principal que coordina las conexiones entre los personajes
 	 	 	 	 y los niveles.
 	 	 	 	 Esta compuesto por varios threads:
 	 	 	 	 1) Thread principal (main) que inicializa los dos threads específicos
 	 	 	 	 2) Thread Orquestador que asigna un nivel y su planificador al personaje
 	 	 	 	 	que lo solicite
 	 	 	 	 3) Thread Planificador N que se asocia a un nivel en particular
 */

//*************************** INCLUDES ***************************

#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include "collections/queue.h"
#include "collections/list.h"
#include "log.h"
#include  "socketsOv.h" //Librería compartida para sockects de overflow
#include <sys/inotify.h> //Libreria inotify
#include "config.h"
 #include <errno.h>
//----------------------------------------------------------------

//******************** DEFINICIONES GLOBALES *********************
//Variables Globales del sistema
//int varGlobalQuantum=3;
//int varGlobalSleep=0.5* 1000000;
int varGlobalQuantum;
int varGlobalSleep;
char* nombreNivel;//se usa para la funcion que se manda al list_find

#define EVENT_SIZE ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN ( 1024 * ( EVENT_SIZE + 24 ) )

//Structs propios de la plataforma

typedef struct t_infoPlanificador {
	int port;
	t_queue* colaListos;
} InfoPlanificador;

typedef struct t_infoNivel {
	char* nombre;
	char ip[20];
	int port;
	int puertoPlanif;
} InfoNivel;

typedef struct t_nodoPersonaje {//TODO completar segun las necesidades
	char simboloRepresentativo; //Ej: @ ! / % $ &
	int socket;
	char recursoPedido;
} NodoPersonaje;

typedef struct t_nodoNivel {//TODO completar segun las necesidades
	char* nombreNivel; //Ej: @ ! / % $ &
	char ip[20];
	int port;
	int puertoPlanif;
} NodoNivel;

typedef struct t_msjPersonaje {
	int solicitaRecurso;
	int bloqueado;
	int finNivel;
	char recursoSolicitado;
} MensajePersonaje;

//----------------------------------------------------------------

//************************** LOGUEO **************************

t_log_level detail = LOG_LEVEL_INFO;
t_log * logOrquestador;
//TODO: Necesita ser sincronizado porque hay muchas instancias de planificador
t_log * logPlanificador;

//----------------------------------------------------------------

//************************** PROTOTIPOS **************************

//Prototipos de funciones a utilizar. Se definien luego de la función main.
int planificador (InfoNivel* nivel);
int orquestador (void);
int listenerPersonaje(InfoPlanificador* planificador);
char* print_ip(int ip);
bool esMiNivel(NodoNivel* nodo);
//----------------------------------------------------------------

//******************** FUNCIONES PRINCIPALES *********************

//MAIN:
//Función principal del proceso, es la encargada de crear al orquestador y al planificador
//Cardinalidad = un único thread
int main (void) {
	logOrquestador = log_create("LogOrquestador.log","Orquestador",true,detail);
	logPlanificador = log_create("LogPlanificador.log","Planificador",true,detail);
	t_config* configNivel = config_create("config.txt");
	varGlobalQuantum=config_get_int_value(configNivel,"Quantum");
	varGlobalSleep=(int)(config_get_double_value(configNivel,"TiempoDeRetardoDelQuantum")* 1000000);
	printf("Quantum: %d Sleep: %d microseconds\n",varGlobalQuantum,	varGlobalSleep);

//******************** inicio inotify *********************
	//variables del inotify

	int fd;//file_descriptor
	int wd;// watch_descriptor


	while(1) {
	int i = 0;
	char buffer[EVENT_BUF_LEN];
	//Creamos una instancia de inotify, me devuelve un archivo descriptor
	fd = inotify_init();
	//Verificamos los errores
	if ( fd < 0 ) {
		perror( "inotify_init" );
	}

	// Creamos un reloj para el evento IN_MODIFY
	//watch_descriptor= inotify_add_watch(archivoDescrpitor, path, evento)
	wd = inotify_add_watch( fd, "/home/utnso/workspace/Plataforma", IN_MODIFY);

	// El archivo descriptor creado por inotify, es el que recibe la información sobre los eventos ocurridos
	// para leer esta información el descriptor se lee como si fuera un archivo comun y corriente pero
	// la diferencia esta en que lo que leemos no es el contenido de un archivo sino la información
	// referente a los eventos ocurridos

	int length = read( fd, buffer, EVENT_BUF_LEN );
	//Verficamos los errores
	if ( length < 0 ) {
		perror( "read" );
	}

	while ( i < length ) {
	struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];
		if ( event->len )
		{
			if ( event->mask & IN_MODIFY)
			{
				if ( event->mask & IN_ISDIR )
				{
					//printf( "El directorio %s fue modificado.\n", event->name );
				}
				else
				{
					printf( "El archivo %s fue modificado.\n", event->name );
				}
			}
		}
	buffer[EVENT_BUF_LEN]="";
	i += EVENT_SIZE + event->len;
	}
	}
	//removing the “/tmp” directory from the watch list.
	inotify_rm_watch( fd, wd );
	//Se cierra la instancia de inotify
	close( fd );
	//******************** fin inotify *********************

		printf("Proceso plataforma iniciado\nCreando THR Orquestador...\n");
	log_info(logOrquestador,"Proceso plataforma iniciado");
	log_info(logOrquestador,"Creando THR Orquestador");

	pthread_t thr_orquestador;
	pthread_create(&thr_orquestador, NULL, orquestador, NULL);
	log_info(logOrquestador,"THR Orquestador creado correctamente");

	pthread_join(thr_orquestador, NULL); //Esperamos que termine el thread orquestador
	pthread_detach(thr_orquestador);

	printf("Proceso plataforma finalizado correctamente\n");
	return 0;
}
//FIN DE MAIN


//PLANIFICADOR:
//Función que será ejecutada a través de un hilo para atender la planificación de cada nivel
//Cardinalidad = 0 hasta N threads ejecutándose simultáneamente, uno por nivel
int planificador (InfoNivel* nivel) {


	//cola de personajes listos y bloqueados
	t_queue *colaListos=queue_create();
	t_queue *colaBloqueados=queue_create();

	//TODO implementar handshake Conectarse con el nivel
	//int socketNivel=quieroUnPutoSocketAndando(nivel->ipN,nivek->portN);
	printf("%s %d\n",nivel->ip,nivel->port);
	log_info(logPlanificador,"IP: %s // Puerto: %d",nivel->ip,nivel->port);

	// Creamos el listener de personajes del planificador (guardar socket e info del pj en la estructura)
	void* nodoAux;
	InfoPlanificador *planificadorActual;
	planificadorActual=malloc(sizeof(InfoPlanificador));
	planificadorActual->colaListos=colaListos;
	//pasar puerto de la plataforma
	planificadorActual->port=nivel->puertoPlanif;
	//Este thread se encargará de escuchar nuevas conexiones de personajes indefinidamente (ver función listenerPersonaje)
	pthread_t threadPersonaje;
	pthread_create(&threadPersonaje, NULL, listenerPersonaje, (void *)planificadorActual);
	log_info(logPlanificador,"THR de escucha del planificador de puerto %d creado correctamente",nivel->port);

	//Se necesita algun char (cualquiera) para poder usar la función de enviar mensaje más adelante
	char* auxcar;
	auxcar=malloc(1);
	*auxcar='P';

	int quantum=varGlobalQuantum;

	while(1){
		//Si la cola no esta vacia
		if(!queue_is_empty(colaListos)){
			printf("La lista no esta vacia\n");
			log_info(logPlanificador,"La lista no esta vacia");
			NodoPersonaje *personajeActual; //todo Revisar que los punteros de personajeActual anden bien

			if(quantum>0){
				printf("Quantum > 0\n");
				log_info(logPlanificador,"El Quantum es mayor a 0");
				//Mandar mensaje de movimiento permitido al socket del personaje del nodo actual (primer nodo de la cola)
				personajeActual = (NodoPersonaje*) queue_peek(colaListos);
				if(mandarMensaje(personajeActual->socket, 8, sizeof(char), auxcar) > 0){
					printf("Se mando Mov permitido al personaje %c\n",personajeActual->simboloRepresentativo);
					log_info(logPlanificador,"Se mando Mov permitido al personaje %c",personajeActual->simboloRepresentativo);
				}
				else {
					log_error(logPlanificador,"No se pudo enviar un Mov permitido al personaje %c",personajeActual->simboloRepresentativo);
				}

			}
			else {
				quantum=varGlobalQuantum;
				printf("Se acabo el quantum");
				log_info(logPlanificador,"Se acabo el quantum");
				//Sacar el nodo actual (primer nodo de la cola) y enviarlo al fondo de la misma
				//todo Sincronizar colaListos con el Listener
				nodoAux=queue_pop(colaListos);
				printf("Se saco de la cola de listos");
				log_info(logPlanificador,"Se saco de la cola de listos");
				queue_push(colaListos,nodoAux);
				printf("Se puso al final de la cola de listos");
				log_info(logPlanificador,"Se puso al final de la cola de listos");
				//Buscar el primero de la cola de listos y mandarle un mensaje de movimiento permitido
				personajeActual = (NodoPersonaje*) queue_peek(colaListos);
				printf("Se saco al primer personaje de la cola");
				log_info(logPlanificador,"Se saco al primer personaje de la cola");
				if(mandarMensaje(personajeActual->socket, 8, sizeof(char), auxcar) > 0){
					printf("Se mando Mov permitido al personaje %c\n",personajeActual->simboloRepresentativo);
					log_info(logPlanificador,"Se mando Mov permitido al personaje %c",personajeActual->simboloRepresentativo);
				}
				else {
					log_error(logPlanificador,"No se pudo enviar Mov permitido al personaje %c",personajeActual->simboloRepresentativo);
				}
			}

			//Esperar respuesta de turno terminado (con la info sobre si quedo bloqueado y si tomo recurso)
			Header headerMsjPersonaje;
			MensajePersonaje msjPersonaje;
			printf("Esperando respuesta de turno concluido\n");
			log_info(logPlanificador,"Esperando respuesta de turno concluido");
			recibirHeader(personajeActual->socket, &headerMsjPersonaje);
			recibirData(personajeActual->socket, headerMsjPersonaje, (void**) &msjPersonaje);
			printf("Respuesta recibida\n");
			log_info(logPlanificador,"Respuesta recibida");
			//Comportamientos según el mensaje que se recibe del personaje

			//Si informa de fin de nivel se lo retira de la cola de listos
//			if(msjPersonaje.finNivel){
//				printf("El personaje termino el Nivel\n");
//				log_info(logPlanificador,"El personaje %c termino el Nivel",personajeActual->simboloRepresentativo);
//				queue_pop(colaListos);
//				msjPersonaje.solicitaRecurso=0;
//				msjPersonaje.bloqueado=0;
//				quantum=varGlobalQuantum+1;
//				printf("Se retiro al personaje de la cola\n");
//				log_info(logPlanificador,"El personaje %c fue retirado de la cola",personajeActual->simboloRepresentativo);
//			}
			log_debug(logPlanificador,"NeedRec: %d Blocked: %d FinNivel: %d Rec: %c",msjPersonaje.solicitaRecurso,msjPersonaje.bloqueado,msjPersonaje.finNivel,msjPersonaje.recursoSolicitado);
//Si solicita recurso y SI quedo bloqueado {quatum=varGlogalQuantum; poner al final de la cola de bloquedados}
			if(msjPersonaje.solicitaRecurso & msjPersonaje.bloqueado){
				printf("Rec bloq1 %d",quantum);
				log_info(logPlanificador,"Rec no bloq1 %d",quantum);
				quantum=varGlobalQuantum+1;
				queue_push(colaBloqueados,queue_pop(colaListos));
				printf("Rec bloq2 %d",quantum);
				log_info(logPlanificador,"Rec no bloq2 %d", quantum);
			}else{
//Si solicita recurso y NO quedo bloqueado {quantum=varGlogalQuantum; poner al final de la cola}
				if(msjPersonaje.solicitaRecurso & !msjPersonaje.bloqueado){
					if(msjPersonaje.finNivel){
						printf("El personaje termino el Nivel\n");
						log_info(logPlanificador,"El personaje %c termino el Nivel",personajeActual->simboloRepresentativo);
						queue_pop(colaListos);
						msjPersonaje.solicitaRecurso=0;
						msjPersonaje.bloqueado=0;
						quantum=varGlobalQuantum+1;
						printf("Se retiro al personaje de la cola\n");
						log_info(logPlanificador,"El personaje %c fue retirado de la cola",personajeActual->simboloRepresentativo);
					}else{
					log_info(logPlanificador,"Rec no bloq1 %d",quantum);
					quantum=varGlobalQuantum+1;
					queue_push(colaListos,queue_pop(colaListos));
					log_info(logPlanificador,"Se puso el Personaje %c al final de la cola luego de asignarle el recurso %c", personajeActual->simboloRepresentativo,msjPersonaje.recursoSolicitado);
					}
				}else{
					if(msjPersonaje.finNivel){
						printf("El personaje termino el Nivel\n");
						log_info(logPlanificador,"El personaje %c termino el Nivel",personajeActual->simboloRepresentativo);
						queue_pop(colaListos);
						msjPersonaje.solicitaRecurso=0;
						msjPersonaje.bloqueado=0;
						quantum=varGlobalQuantum+1;
						printf("Se retiro al personaje de la cola\n");
						log_info(logPlanificador,"El personaje %c fue retirado de la cola",personajeActual->simboloRepresentativo);
					}
				}
			}


			quantum--;
			log_debug(logPlanificador,"Quatum Left: %d\n",quantum);

			usleep(varGlobalSleep);
		}else{
			log_debug(logPlanificador,"Cola vacia --> Sleep");
			usleep(200000); //para que no quede en espera activa
		}
	}//Cierra While(1)

	return EXIT_SUCCESS;
}
//FIN DE PLANIFICADOR


//ORQUESTADOR:
//Función que será ejecutada a través de un hilo para asignar un nivel y un planificador a cada
//proceso personaje que se conecte.
//Cardinalidad = un único thread que atiende las solicitudes
int orquestador (void) {
	t_list* listaNiveles=list_create();
	int contadorPuerto=5000;
	int socketNuevaConexion;
	//Struct con info de conexión (IP y puerto) del nivel y el planificador asociado a ese nivel
	typedef struct t_infoConxNivPlan {
		char ipNivel[16];
		int portNivel;
		char ipPlanificador[16];
		int portPlanificador;
	} ConxNivPlan;

	ConxNivPlan msj;

	log_info(logOrquestador,"THR Orquestador: iniciado");
	log_info(logOrquestador,"THR Orquestador: Esperando conexiones de personajes o Niveles...");

	//Creamos un socket de escucha para el puerto 4999
	int socketEscucha;
	if ((socketEscucha = quieroUnPutoSocketDeEscucha(4999)) != 1){
		//TODO: Des-hardcodear el puerto 4999
		log_info(logOrquestador,"Socket de escucha para el puerto 4999 creado");
	}
	else {
		log_error(logOrquestador,"No se pudo crear el socket de escucha para el puerto 4999");
	}
	//TODO: Supongo que el while(1) debería estar dentro del else
	//Chequear eso
	while(1){

		if (listen(socketEscucha, 1) != 0) {
			perror("Error al poner a escuchar socket");
			log_error(logOrquestador,"Error al poner a escuchar socket");
			return EXIT_FAILURE;
		}
		struct sockaddr_in cli_addr;
		int socklen=sizeof(cli_addr);
		if ((socketNuevaConexion = accept(socketEscucha, &cli_addr, &socklen)) < 0) {
			perror("Error al aceptar conexion");
			log_error(logOrquestador,"Error al aceptar conexion");
			return EXIT_FAILURE;
		}
		char* ipCliente;
		ipCliente=print_ip(cli_addr.sin_addr.s_addr);
		printf("Ip del cliente conectado: %d %s\n",cli_addr.sin_addr.s_addr,ipCliente);
		log_info(logOrquestador,"Ip del cliente conectado: %d %s",cli_addr.sin_addr.s_addr, ipCliente);

		//Handshake en el que recibe el simbolo del personaje
		char *simboloRecibido; //Ej: @ ! / % $ &
		simboloRecibido=malloc(1);
		Header unHeader;
		if(recibirHeader(socketNuevaConexion,&unHeader)){

		log_info(logOrquestador,"Header recibido del socket: %d",socketNuevaConexion);

		switch(unHeader.type){

		//TODO: colocar una pequenia descripción de cada tipo

		case 0://case Personaje

			log_info(logOrquestador,"Header tipo 0");

			strcpy(msj.ipNivel,"127.0.0.1");
			strcpy(msj.ipPlanificador,"127.0.0.1");
			msj.portNivel=0;
			msj.portPlanificador=0;
			simboloRecibido=malloc(1);

			if(recibirData(socketNuevaConexion,unHeader,(void**)simboloRecibido) >= 0){
				if (mandarMensaje(socketNuevaConexion,0 , sizeof(char),simboloRecibido) > 0) {
					//log_info(logger,"Mando mensaje al personaje %c",*rec);
					printf("Entro Personaje: %c\n",*simboloRecibido);
					log_info(logOrquestador,"Entro Personaje: %c",*simboloRecibido);
				}
			}


			char* nivelDelPersonaje;
			//Esperar solicitud de info de conexion de Nivel y Planifador
			printf("Esperando solicitud de nivel\n");
			log_info(logOrquestador,"Esperando solicitud de nivel...");
	//		if(recibirHeader(socketNuevaConexion,&unHeader)){
	//			printf("Info Header: %d %d\n",unHeader.payloadlength,unHeader.type);
	//			if(recibirData(socketNuevaConexion,unHeader,(void**)nivelDelPersonaje)){
			if(recibirMensaje(socketNuevaConexion, (void**) &nivelDelPersonaje)>=0) {
					printf("Nivel recibido: %s\n",nivelDelPersonaje);
					log_info(logOrquestador,"Nivel recibido %s",nivelDelPersonaje);
					//Enviar info de conexión (IP y port) del Nivel y el Planificador asociado a ese nivel

					//settear variable global para usar en funcion q se manda a list_find
					nombreNivel=nivelDelPersonaje;
					printf("Se busca el nivel \n");
					log_info(logOrquestador,"Se busca el nivel");

					NodoNivel* nivel =list_find(listaNiveles,esMiNivel);

					printf("Se encontro el nivel %p\n",nivel);
					log_info(logOrquestador,"Se encontro el nivel %p",nivel);

					if(nivel!=NULL){
						log_info(logOrquestador,"El nivel existe (o sea, es != NULL)");
						strcpy(msj.ipNivel,nivel->ip);
						msj.portNivel=nivel->port;
						msj.portPlanificador=nivel->puertoPlanif;
						log_info(logOrquestador,"Se copiaron los datos del nivel al mensaje para mandar al Personaje %c",*simboloRecibido);
					}
					//TODO: en el caso que el nivel fuera NULL no debería mandar igual un mensaje,
					if(mandarMensaje(socketNuevaConexion,1,sizeof(ConxNivPlan),&msj)>=0){
						log_info(logOrquestador,"Se enviaron los datos del nivel al Personaje %c",*simboloRecibido);
					}
					else {
						log_error(logOrquestador,"No se pudo enviar los datos del nivel al Personaje %c",*simboloRecibido);
					}
	//			}
			}
			//cerrar socket
			close(socketNuevaConexion);
			log_info(logOrquestador,"Se cerro el socket %d",socketNuevaConexion);
			break;

		case 2: //case Nivel
			simboloRecibido=malloc(unHeader.payloadlength);
			if(recibirData(socketNuevaConexion,unHeader,(void**)simboloRecibido)>=0){
				if (mandarMensaje(socketNuevaConexion,0 , sizeof(char),simboloRecibido) > 0) {
					//log_info(logger,"Mando mensaje al personaje %c",*rec);
					printf("Entro Nivel: %s\n",simboloRecibido);
					log_info(logOrquestador,"Entro Nivel: %s",simboloRecibido);
				}
			}

			NodoNivel *nodoNivel;
			nodoNivel=malloc(sizeof(NodoNivel));
			strcpy(nodoNivel->ip,ipCliente);
			nodoNivel->nombreNivel=simboloRecibido;
			nodoNivel->port=contadorPuerto;
			contadorPuerto++;
			nodoNivel->puertoPlanif=contadorPuerto;

			list_add(listaNiveles,nodoNivel);
			log_info(logOrquestador,"Se anadio el nivel %s a la lista de Niveles",simboloRecibido);

			InfoNivel *nivel;
			nivel=malloc(sizeof(InfoNivel));
			strcpy(nivel->ip,ipCliente);
			nivel->nombre=simboloRecibido;
			nivel->port=nodoNivel->port;
			nivel->puertoPlanif=contadorPuerto;
			contadorPuerto++;

			int* aux;
			aux=malloc(sizeof(int));
			*aux=nodoNivel->port;
			//manda el puerto asignado al nivel para que escuche conexiones.
			//TODO: y si es -1 de qué nos pintamos?
			if(!mandarMensaje(socketNuevaConexion,1, sizeof(int),aux)){
				log_error(logOrquestador,"No se pudo enviar un mensaje al nivel %s de socket %d",simboloRecibido,socketNuevaConexion);
				break;} //TODO handlear desconexion
			free(aux);
			if(!recibirMensaje(socketNuevaConexion,(void**)&aux)){
				log_error(logOrquestador,"No se pudo recibir un mensaje del nivel %s de socket %d",simboloRecibido,socketNuevaConexion);
				break; }//TODO handlear desconexion
			free(aux);

			pthread_t threadPlanif;
			pthread_create(&threadPlanif, NULL, planificador, (void *)nivel);
			log_info(logOrquestador,"Se creo el THR del planificador correspondiente al nivel %s",simboloRecibido);

			break;
		default:
			break;
		}
	}

	}//Cierra While(1)

	//log_info(logger,"Mando el socket %d (Thread)", personaje->socket);
	//
	//pthread_t threadPersonaje;
	//
	//pthread_create(&threadPersonaje, NULL, han, (void *)msj);


	//	printf("THR Orquestador: terminado\n"); LLamar a Koopa

	return EXIT_SUCCESS;
}
//FIN DE ORQUESTADOR

//******************* FUNCIONES AUXILIARES ********************

//listenerPersonaje:
//Función que escucha nuevas conexiones de personajes y
//será ejecutada a través de un hilo por un planificador
//Cardinalidad = 0 hasta N threads ejecutándose simultaneamente, uno por planificador
int listenerPersonaje(InfoPlanificador* planificador){

	    int socketEscucha,socketNuevaConexion;
	    if((socketEscucha=quieroUnPutoSocketDeEscucha(planificador->port)) != 1){
	    	log_info(logPlanificador,"Se creo el socket de escucha %d",planificador->port);
	    }
	    //TODO: el while(1) debería estar adentro del if
	    else {
	    	log_error(logPlanificador,"No se pudo crear el socket de escucha");
	    }

	    //Ciclo While(1) para escuchar nuevos personajes indefinidamente
		while (1){

			// Escuchar nuevas conexiones entrantes.
			if (listen(socketEscucha, 1) != 0) {
				//log_error(logger,"Error al bindear socket escucha");
				log_error(logPlanificador,"Error al bindear socket escuchar");
				return EXIT_FAILURE;
			}
			//log_info(logger,"Escuchando conexiones entrantes");
			log_info(logPlanificador,"Escuchando conexiones entrantes");

			// Aceptar una nueva conexion entrante. Se genera un nuevo socket con la nueva conexion.
			// La funcion accept es bloqueante, no sigue la ejecución hasta que se reciba algo
			if ((socketNuevaConexion = accept(socketEscucha, NULL, 0)) < 0) {
				//log_error(logger,"Error al aceptar conexion entrante");
				log_error(logPlanificador,"Error al aceptar conexion entrante");
				return EXIT_FAILURE;
			}

			//Handshake en el que recibe el simbolo del personaje
			char *simboloRecibido; //Ej: @ ! / % $ &
			if(recibirMensaje(socketNuevaConexion, (void**) &simboloRecibido)>=0) {
				//log_info(logger,"Llego el Personaje %c del nivel",*simboloRecibido);
				if (mandarMensaje(socketNuevaConexion,0 , 1,simboloRecibido)) {
					//log_info(logger,"Mando mensaje al personaje %c",*simboloRecibido);
					printf("Handshake Respondido\n");
					log_info(logPlanificador,"Handshake respondido al Personaje %c", *simboloRecibido);
				}

				//Agrega el nuevo personaje a la cola de listos del planificador
				NodoPersonaje* personaje;
				personaje=malloc(sizeof(NodoPersonaje));
				personaje->simboloRepresentativo=*simboloRecibido; //Ej: @ ! / % $ &
				personaje->socket=socketNuevaConexion;
				queue_push(planificador->colaListos,personaje);

				log_info(logPlanificador,"Se agrego al Personaje %c a la cola de listos",*simboloRecibido);

				//log_info(logger,"Mando el socket %d (Thread)", personaje->socket);

			}

		}//Cierra el while(1)

	    return EXIT_SUCCESS;
	}

char* print_ip(int ip)
{
	char* aux;
	aux=malloc(16);
    unsigned char bytes[4];
    bytes[0] = ip & 0xFF;
    bytes[1] = (ip >> 8) & 0xFF;
    bytes[2] = (ip >> 16) & 0xFF;
    bytes[3] = (ip >> 24) & 0xFF;
    snprintf(aux,16,"%d.%d.%d.%d", bytes[0], bytes[1], bytes[2], bytes[3]);
    return aux;
}
bool esMiNivel(NodoNivel* nodo){
	printf("%s %s\n",nodo->nombreNivel,nombreNivel);

	if(strcmp(nodo->nombreNivel,nombreNivel)==0)
		return true;
	return false;
}
//----------------------------------------------------------------
