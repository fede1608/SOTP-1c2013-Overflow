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
//son constantes
#define EVENT_SIZE ( sizeof (struct inotify_event))
#define EVENT_BUF_LEN ( 1024 * ( EVENT_SIZE + 16 ) )
//******************** DEFINICIONES GLOBALES *********************
//Variables Globales del sistema
//int varGlobalQuantum=3;
//int varGlobalSleep=0.5* 1000000;
int varGlobalQuantum;
int varGlobalSleep;
int varGlobalDeadlock;
int varGlobalSleepDeadlock;
int flagGlobalFin=1;//flag q anuncia el fin del programa
int g_contPersonajes=0;//contador global de personajes
char* nombreNivel;//se usa para la funcion que se manda al list_find
t_list* listaNiveles;

//Structs propios de la plataforma

typedef struct t_nodoRec {
char id;
int cantAsignada;
} NodoRecurso;

typedef struct t_infoPlanificador {
	int port;
	t_queue* colaListos;
} InfoPlanificador;

typedef struct t_nodoNivel {//TODO completar segun las necesidades
	char* nombreNivel; //Ej: @ ! / % $ &
	char ip[20];
	int port;
	int puertoPlanif;
	int socket;
	int deadlockActivado;
	int sleepDeadlock;
	t_queue * colaListos;
	t_queue * colaBloquedos;
} NodoNivel;

typedef struct t_infoNivel {
	char* nombre;
	char ip[20];
	int port;
	int puertoPlanif;
	NodoNivel* nodoN;
} InfoNivel;

typedef struct t_nodoPersonaje {//TODO completar segun las necesidades
	char simboloRepresentativo; //Ej: @ ! / % $ &
	int socket;
	char recursoPedido;
} NodoPersonaje;

typedef struct t_msjPersonaje {
	int solicitaRecurso;
	int bloqueado;
	int finNivel;
	char recursoSolicitado;
} MensajePersonaje;

//----------------------------------------------------------------

//************************** LOGUEO **************************

t_log_level detail = LOG_LEVEL_TRACE;
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
void desconexionPersonaje(int socketDesconectar, MensajePersonaje msjPersonaje, int quantum, t_queue * colaListos);
//----------------------------------------------------------------

//******************** FUNCIONES PRINCIPALES *********************

//MAIN:
//Función principal del proceso, es la encargada de crear al orquestador y al planificador
//Cardinalidad = un único thread
int main (void) {
	listaNiveles=list_create();
	logOrquestador = log_create("LogOrquestador.log","Orquestador",true,detail);
	logPlanificador = log_create("LogPlanificador.log","Planificador",true,detail);
	t_config* config = config_create("config.txt");
	varGlobalQuantum=config_get_int_value(config,"Quantum");
	varGlobalSleep=(int)(config_get_double_value(config,"TiempoDeRetardoDelQuantum")* 1000000);
	varGlobalDeadlock=config_get_int_value(config,"AlgoritmoDeDeteccionDeInterbloqueo");
	varGlobalSleepDeadlock=(int)(config_get_double_value(config,"EjecucionDeAlgoritmoDeDeteccionDeInterbloqueo")* 1000000);
	log_info(logOrquestador,"Quantum: %d Sleep: %d microseconds",varGlobalQuantum,	varGlobalSleep);
	log_info(logOrquestador,"Proceso plataforma iniciado. Creando THR Orquestador...");
	log_info(logOrquestador,"Proceso plataforma iniciado");
	log_info(logOrquestador,"Creando THR Orquestador");

	pthread_t thr_orquestador;
	pthread_create(&thr_orquestador, NULL, orquestador, NULL);
	log_info(logOrquestador,"THR Orquestador creado correctamente");

	//pthread_join(thr_orquestador, NULL); //Esperamos que termine el thread orquestador
	//pthread_detach(thr_orquestador);

	//********************* INICIO INOTIFY *********************
	int fd;//file_descriptor
	int wd;// watch_descriptor

	char cwd[1024];
	char * fileconf="/config.txt";
	if (getcwd(cwd, sizeof(cwd)) != NULL){
		log_debug(logOrquestador,"Current working dir: %s", cwd);
		strcat(cwd, fileconf);
		log_debug(logOrquestador,"Current file: %s", cwd);
	}
	else
	   perror("getcwd() error");

	struct timeval time;
	fd_set rfds;
	int ret;
	//Creamos una instancia de inotify, me devuelve un archivo descriptor
	fd = inotify_init();
	//Verificamos los errores
	if ( fd < 0 ) {
		perror( "inotify_init" );
	}
	// Creamos un watch para el evento IN_MODIFY
	//watch_descriptor= inotify_add_watch(archivoDescrpitor, path, evento)
	wd = inotify_add_watch( fd, cwd, IN_MODIFY);

	// El archivo descriptor creado por inotify, es el que recibe la información sobre los eventos ocurridos
	// para leer esta información el descriptor se lee como si fuera un archivo comun y corriente pero
	// la diferencia esta en que lo que leemos no es el contenido de un archivo sino la información
	// referente a los eventos ocurridos
	while(flagGlobalFin) { //sale cuando se termina el programa
		time.tv_sec = 2;
		time.tv_usec = 0;//varGlobalSleep;



		FD_ZERO(&rfds);
		FD_SET(fd,&rfds);
		ret = select (fd + 1, &rfds, NULL, NULL, &time);
		if (ret < 0){
			 perror ("select");
		}
		if (!ret){
			 /* timed out!  */
		}
		if (FD_ISSET (fd, &rfds)){

	//	while(1){//flagGlobalFin) { //sale cuando se termina el programa
		int i = 0;
		char buffer[EVENT_BUF_LEN];
		int length = read( fd, buffer, EVENT_BUF_LEN );
		//Verficamos los errores
		if ( length < 0 ) {
			perror( "read" );
		}

		while ( i < length ) {
		struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];
		//si watcheamos un archivo no va a venir el nombre poqrue es redundante por eso event->len es igual a 0

		log_debug(logOrquestador,"wd=%d mask=%u cookie=%u len=%u",
		                event->wd, event->mask,
		                event->cookie, event->len);
				if ( event->mask & IN_MODIFY)
				{

					config_destroy(config);
					config = config_create("config.txt");
					if(config_has_property(config,"Quantum")){
					varGlobalQuantum=config_get_int_value(config,"Quantum");
					log_info(logOrquestador,"El archivo de configuracion fue modificado");
					log_info(logOrquestador,"Nuevo Valor del Quantum: %d",varGlobalQuantum);
					}
				}

		buffer[EVENT_BUF_LEN]="";
		i += EVENT_SIZE + event->len;

		}

	}

}
//removing the “/tmp” directory from the watch list.
inotify_rm_watch(fd,wd);

//Se cierra la instancia de inotify
close(fd);
//******************** FIN INOTIFY *********************
config_destroy(config);
log_info(logOrquestador,"Proceso plataforma finalizado correctamente\n");

//******************** KOOPA *********************
//todo hay que ver donde metemos a koopa para que esto sea consistente, yo diria meterlo en la misma carpeta q el planificador
char *environ[]= {"../../Libs","../../Varios/reglas2.txt",NULL};
execv("../../Varios/koopa1.2",environ);
//printf("salio todo mal\n");
//******************** FIN INOTIFY *********************

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
	nivel->nodoN->colaBloquedos=colaBloqueados;
	nivel->nodoN->colaListos=colaListos;
	//TODO implementar handshake Conectarse con el nivel
	//int socketNivel=quieroUnPutoSocketAndando(nivel->ipN,nivek->portN);

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

		/*TODO: no me gusta como esta hecho pero es
		 * la unica solucion que se me ocurrio en este momento.
		 * La idea es que esta variable global sea una flag que se active
		 * cuando un nivel se cierra y funciona barbaro para romper
		 * el while y que termine el thread, el problema es cuando
		 * se desconecten varios niveles al mismo tiempo,
		 * quizas cierra alguno que no va*/

		nombreNivel = nivel->nombre;

		if(!list_find(listaNiveles,esMiNivel)){
			log_info(logPlanificador,"Se rompe el while(1) y se cierra el thread");
			break;
		}

		//Si la cola no esta vacia
		if(!queue_is_empty(colaListos)){
			//imprimir PJ listos
			log_info(logOrquestador,"*****Personajes en la Cola de Listos*****");
			int p;
			int tam=queue_size(colaListos);
			for (p=0;p<tam;p++){
				NodoPersonaje* nodoP=queue_pop(colaListos);
				log_info(logOrquestador,"%d° Personaje: %c",p+1,nodoP->simboloRepresentativo);
				queue_push(colaListos,nodoP);
			}
			//imprimir PJ bloqueados
			log_info(logOrquestador,"*****Personajes en la Cola de Bloqueados*****");
			tam=queue_size(colaBloqueados);
			for (p=0;p<tam;p++){
				NodoPersonaje* nodoP=queue_pop(colaBloqueados);
				log_info(logOrquestador,"%d° Personaje: %c Recurso Solicitado: %c",p+1,nodoP->simboloRepresentativo,nodoP->recursoPedido);
				queue_push(colaBloqueados,nodoP);
			}
//			log_info(logPlanificador,"La lista no esta vacia");
			NodoPersonaje *personajeActual; //todo Revisar que los punteros de personajeActual anden bien

			if(quantum>0){

				log_info(logPlanificador,"El Quantum %d es mayor a 0",quantum);
				//Mandar mensaje de movimiento permitido al socket del personaje del nodo actual (primer nodo de la cola)
				personajeActual = (NodoPersonaje*) queue_peek(colaListos);
				if(mandarMensaje(personajeActual->socket, 8, sizeof(char), auxcar) > 0){

					log_info(logPlanificador,"Se mando Mov permitido al personaje %c",personajeActual->simboloRepresentativo);
				}
				else {
					log_error(logPlanificador,"No se pudo enviar un Mov permitido al personaje %c",personajeActual->simboloRepresentativo);
				}

			}
			else {
				quantum=varGlobalQuantum;

				log_info(logPlanificador,"Se termino el quantum");
				//Sacar el nodo actual (primer nodo de la cola) y enviarlo al fondo de la misma
				//todo Sincronizar colaListos con el Listener
				nodoAux=queue_pop(colaListos);

				log_info(logPlanificador,"Se saco de la cola de listos");
				queue_push(colaListos,nodoAux);

				log_info(logPlanificador,"Se puso al final de la cola de listos");
				//Buscar el primero de la cola de listos y mandarle un mensaje de movimiento permitido
				personajeActual = (NodoPersonaje*) queue_peek(colaListos);

				log_info(logPlanificador,"Se saco al primer personaje de la cola");
				if(mandarMensaje(personajeActual->socket, 8, sizeof(char), auxcar) > 0){
					log_info(logPlanificador,"Se mando Mov permitido al personaje %c",personajeActual->simboloRepresentativo);
				}
				else {
					log_error(logPlanificador,"No se pudo enviar Mov permitido al personaje %c",personajeActual->simboloRepresentativo);
				}
			}

			//Esperar respuesta de turno terminado (con la info sobre si quedo bloqueado y si tomo recurso)
			Header headerMsjPersonaje;
			MensajePersonaje msjPersonaje;
			log_info(logPlanificador,"Esperando respuesta de turno concluido");
			if(recibirHeader(personajeActual->socket, &headerMsjPersonaje) > 0){
				if(recibirData(personajeActual->socket, headerMsjPersonaje, (void**) &msjPersonaje)){
					log_info(logPlanificador,"Respuesta recibida");
					//Comportamientos según el mensaje que se recibe del personaje

					log_debug(logPlanificador,"NeedRec: %d Blocked: %d FinNivel: %d Rec: %c",msjPersonaje.solicitaRecurso,msjPersonaje.bloqueado,msjPersonaje.finNivel,msjPersonaje.recursoSolicitado);
		//Si solicita recurso y SI quedo bloqueado {quatum=varGlogalQuantum; poner al final de la cola de bloquedados}
					if(msjPersonaje.solicitaRecurso & msjPersonaje.bloqueado){
						log_info(logPlanificador,"Rec%c bloq1 %d",msjPersonaje.recursoSolicitado,quantum);
						personajeActual->recursoPedido=msjPersonaje.recursoSolicitado;
						quantum=varGlobalQuantum+1;
						queue_push(colaBloqueados,queue_pop(colaListos));
						log_info(logPlanificador,"Rec bloq2 %d", quantum);
					}else{
		//Si solicita recurso y NO quedo bloqueado {quantum=varGlogalQuantum; poner al final de la cola}
						if(msjPersonaje.solicitaRecurso & !msjPersonaje.bloqueado){
							if(msjPersonaje.finNivel){
								log_info(logPlanificador,"El personaje %c termino el Nivel",personajeActual->simboloRepresentativo);
								queue_pop(colaListos);
								msjPersonaje.solicitaRecurso=0;
								msjPersonaje.bloqueado=0;
								quantum=varGlobalQuantum+1;
								log_info(logPlanificador,"El personaje %c fue retirado de la cola",personajeActual->simboloRepresentativo);
							}else{
							log_info(logPlanificador,"Rec no bloq1 %d",quantum);
							quantum=varGlobalQuantum+1;
							queue_push(colaListos,queue_pop(colaListos));
							log_info(logPlanificador,"Se puso el Personaje %c al final de la cola luego de asignarle el recurso %c", personajeActual->simboloRepresentativo,msjPersonaje.recursoSolicitado);
							}
						}else{
							if(msjPersonaje.finNivel){
								log_info(logPlanificador,"El personaje %c termino el Nivel",personajeActual->simboloRepresentativo);
								queue_pop(colaListos);
								msjPersonaje.solicitaRecurso=0;
								msjPersonaje.bloqueado=0;
								quantum=varGlobalQuantum+1;
								close(personajeActual->socket);
								log_info(logPlanificador,"El personaje %c fue retirado de la cola",personajeActual->simboloRepresentativo);
							}
						}
					}

					quantum--;
					log_debug(logPlanificador,"Quatum Left: %d Sleep: %d",quantum,varGlobalSleep);
					usleep(varGlobalSleep);
				}
				else {
					//Se retira al PJ de la cola de Listos
					//Por aparente desconexion
					desconexionPersonaje(personajeActual->socket, msjPersonaje, quantum, colaListos);
					log_error(logPlanificador,"No llego la data del PJ: %c",personajeActual->simboloRepresentativo);

				}
			}
			else {
				//Se retira al PJ de la cola de Listos
				//Por aparente desconexion
				desconexionPersonaje(personajeActual->socket, msjPersonaje, quantum, colaListos);
				log_error(logPlanificador,"No llego el header del PJ: %c",personajeActual->simboloRepresentativo);
			}

		}else{
//			log_debug(logPlanificador,"Cola de listos vacia --> Sleep");
			usleep(varGlobalSleep); //para que no quede en espera activa
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
	//t_list* listaNiveles=list_create();
	int contadorPuerto=4999;
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

	//Creamos un socket de escucha para el puerto inicial setteado en contadorPuerto
	int socketEscucha;
	if ((socketEscucha = quieroUnPutoSocketDeEscucha(contadorPuerto)) != 1){
		contadorPuerto++;
		log_info(logOrquestador,"Socket de escucha para el puerto 4999 creado");
	}
	else {
		log_error(logOrquestador,"No se pudo crear el socket de escucha para el puerto 4999");
	}
	//TODO: Supongo que el while(1) debería estar dentro del else
	//Chequear eso

	fd_set descriptoresLectura;	/* Descriptores de interes para select() */						/* Buffer para leer de los socket */
	int maximo=socketEscucha;							/* Número de descriptor más grande */
	int i;								/* Para bucles */
	while(1){

		NodoNivel *nodoN;
		/* Se inicializa descriptoresLectura */
		FD_ZERO (&descriptoresLectura);
		/* Se añade para select() el socket servidor */
		FD_SET (socketEscucha, &descriptoresLectura);
		/* Se añaden para select() los sockets con los clientes ya conectados */
		for (i=0; i<list_size(listaNiveles); i++){
			nodoN=list_get(listaNiveles,i);
			FD_SET(nodoN->socket, &descriptoresLectura);
			if(nodoN->socket > maximo)
				maximo=nodoN->socket;
		}

		/* Espera indefinida hasta que alguno de los descriptores tenga algo
		 * que decir: un nuevo cliente o un cliente ya conectado que envía un
		 * mensaje */
		select (maximo + 1, &descriptoresLectura, NULL, NULL, NULL);
		/* Se comprueba si algún cliente ya conectado ha enviado algo */
		for (i=0; i<list_size(listaNiveles); i++){
			nodoN=list_get(listaNiveles,i);
			if (FD_ISSET (nodoN->socket, &descriptoresLectura))
			{
				//recibir info del nivel
				char *simboloRecibido; //Ej: @ ! / % $ &
				simboloRecibido=malloc(1);
				Header unHeader;
				if(recibirHeader(nodoN->socket,&unHeader)>0){
					log_info(logOrquestador,"Header recibido del socket: %d",socketNuevaConexion);
					NodoRecurso *nodoR;
					NodoRecurso * recursosAsignados1;
					switch(unHeader.type){
						//TODO: implementar

						case 3:

							break;
						case 4://el nivel envia los recursos liberados

							log_info(logOrquestador,"El nivel %s solicita liberar recursos",nodoN->nombreNivel);
							nodoR=malloc(unHeader.payloadlength);
							recibirData(nodoN->socket,unHeader,(void**)nodoR);
							//buscar en entre los bloqueados y empezar a asignar recursos liberados, pasar pj a listos
							int p,r,tamColaBloq;
							for(r=0;r<(unHeader.payloadlength/sizeof(NodoRecurso));r++)
								log_debug(logOrquestador,"id: %c recALib: %d",nodoR[r].id,nodoR[r].cantAsignada);
							recursosAsignados1=malloc(unHeader.payloadlength);
							memcpy(recursosAsignados1,nodoR,unHeader.payloadlength);

							//inicializa la los rec asignados
							for(r=0;r<(unHeader.payloadlength/sizeof(NodoRecurso));r++)
								recursosAsignados1[r].cantAsignada=0;
							//for each pj bloq{
							tamColaBloq=queue_size(nodoN->colaBloquedos);
							for (p=0;p<tamColaBloq;p++){
							//for each recursoRecibido{
								NodoPersonaje* nodoP=queue_pop(nodoN->colaBloquedos);
								log_debug(logOrquestador,"Per: %c recBloc:%c",nodoP->simboloRepresentativo,nodoP->recursoPedido);
								for(r=0;r<(unHeader.payloadlength/sizeof(NodoRecurso));r++){
									//rec==id && rec.cant>0? joya restar, sumar al asignado, liberar pj, romper for
									if((nodoR[r].id==nodoP->recursoPedido)&&(nodoR[r].cantAsignada>0)){
										nodoR[r].cantAsignada--;
										recursosAsignados1[r].cantAsignada++;
										log_info(logOrquestador,"Se libero al personaje %c", nodoP->simboloRepresentativo);
										queue_push(nodoN->colaListos,nodoP);
										r=(unHeader.payloadlength/sizeof(NodoRecurso));
//										break;
									} else if((r+1)==(unHeader.payloadlength/sizeof(NodoRecurso)))//si es el ultimo recurso y todavia no salio del for..
										queue_push(nodoN->colaBloquedos,nodoP);
								}
							}
							//mandar recAsignados
							log_info(logOrquestador,"Se envio los recursos Asignados");
							mandarMensaje(nodoN->socket,4,unHeader.payloadlength,(void*)recursosAsignados1);
							free(nodoR);
							free(recursosAsignados1);
							break;
						default:
							break;
						}
					close(socketNuevaConexion);
				}
				/* Si se desconecta el nivel correspondiente
				 * (el socket se desconecta)
				es eliminado de la lista de niveles */
				else {
					nombreNivel = nodoN->nombreNivel;
					list_remove_by_condition(listaNiveles, esMiNivel);
					log_info(logOrquestador,"Se borro el nivel %s por desconexion",nodoN->nombreNivel);
					close(socketNuevaConexion);
				}
			}
		}



		/* Se comprueba si algún cliente nuevo desea conectarse y se le
		 * admite */
		if (FD_ISSET (socketEscucha, &descriptoresLectura))
		{
			//llamar al nuevaconexion

			if (listen(socketEscucha, 1) != 0) {
				log_error(logOrquestador,"Error al poner a escuchar socket");
				return EXIT_FAILURE;
			}
			struct sockaddr_in cli_addr;
			int socklen=sizeof(cli_addr);
			if ((socketNuevaConexion = accept(socketEscucha, &cli_addr, &socklen)) < 0) {
				log_error(logOrquestador,"Error al aceptar conexion");
				return EXIT_FAILURE;
			}
			char* ipCliente;
			ipCliente=print_ip(cli_addr.sin_addr.s_addr);
			log_info(logOrquestador,"Ip del cliente conectado: %d %s",cli_addr.sin_addr.s_addr, ipCliente);

			//Handshake en el que recibe el simbolo del personaje
			char *simboloRecibido; //Ej: @ ! / % $ &
			simboloRecibido=malloc(1);
			Header unHeader;
			if(recibirHeader(socketNuevaConexion,&unHeader)){

				log_info(logOrquestador,"Header recibido del socket: %d",socketNuevaConexion);

				switch(unHeader.type){
				case 0://case Personaje
					log_info(logOrquestador,"Header tipo 0");
					strcpy(msj.ipNivel,"127.0.0.1");
					strcpy(msj.ipPlanificador,"127.0.0.1");
					msj.portNivel=0;
					msj.portPlanificador=0;
					simboloRecibido=malloc(1);

					if(recibirData(socketNuevaConexion,unHeader,(void**)simboloRecibido) >= 0){
						if (mandarMensaje(socketNuevaConexion,0 , sizeof(char),simboloRecibido) > 0) {
							log_info(logOrquestador,"Entro Personaje: %c",*simboloRecibido);
						}
					}
					char* nivelDelPersonaje;
					//Esperar solicitud de info de conexion de Nivel y Planifador

					if(recibirHeader(socketNuevaConexion,&unHeader)){
						log_debug(logOrquestador,"Info Header: payload:%d type:%d",unHeader.payloadlength,unHeader.type);
						switch(unHeader.type){
						case 1://Personaje entra por primera vez al orquestador (incrementa el contador de personajes)
							log_info(logOrquestador,"El Personaje %c se conecto por primera vez",*simboloRecibido);
							log_info(logOrquestador,"Esperando solicitud de nivel...");
							nivelDelPersonaje=malloc(unHeader.payloadlength);
							if(recibirData(socketNuevaConexion,unHeader,(void**)nivelDelPersonaje)){
	//						if(recibirMensaje(socketNuevaConexion, (void**) &nivelDelPersonaje)>=0) {
								log_info(logOrquestador,"Nivel recibido %s",nivelDelPersonaje);
								//Enviar info de conexión (IP y port) del Nivel y el Planificador asociado a ese nivel
								//settear variable global para usar en funcion q se manda a list_find
								nombreNivel=nivelDelPersonaje;
								log_info(logOrquestador,"Se busca el nivel");
								NodoNivel* nivel =list_find(listaNiveles,esMiNivel);
								log_info(logOrquestador,"Se encontro el nivel %p",nivel);
								if(nivel!=NULL){
									g_contPersonajes++;
									log_debug(logOrquestador,"Personajes Restantes: %d Fin: %d",g_contPersonajes,flagGlobalFin);
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
							}
							break;
						case 2://el personaje se conecta desp de terminar un nivel

							log_info(logOrquestador,"Esperando solicitud de nivel...");
							nivelDelPersonaje=malloc(unHeader.payloadlength);
							if(recibirData(socketNuevaConexion,unHeader,(void**)nivelDelPersonaje)){
							log_info(logOrquestador,"El Personaje %c termino el nivel %s",*simboloRecibido,nivelDelPersonaje);
							}
							log_info(logOrquestador,"Esperando solicitud de nivel...");
							if(recibirMensaje(socketNuevaConexion, (void**) &nivelDelPersonaje)>=0) {
								log_info(logOrquestador,"El personaje %c solicita informacion del nivel %s",*simboloRecibido,nivelDelPersonaje);
								//Enviar info de conexión (IP y port) del Nivel y el Planificador asociado a ese nivel
								//settear variable global para usar en funcion q se manda a list_find
								nombreNivel=nivelDelPersonaje;
								log_info(logOrquestador,"Se busca el nivel");
								NodoNivel* nivel =list_find(listaNiveles,esMiNivel);
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
							}
							break;
						case 3: case 4:	//case 3: el personaje se muere por sigterm y reinicia el nivelActual
									//case 4:el personaje pierde todas las vidas y reinicia el plan de niveles

							log_info(logOrquestador,"Esperando solicitud de nivel...");
							nivelDelPersonaje=malloc(unHeader.payloadlength);
							if(recibirData(socketNuevaConexion,unHeader,(void**)nivelDelPersonaje)){
								if(unHeader.type==3) log_info(logOrquestador,"El Personaje %c murio por la señal SIGTERM y reinicia el nivel: %s",*simboloRecibido,nivelDelPersonaje);
								else log_info(logOrquestador,"El Personaje %c perdio todas sus vidas y debe reiniciar su plan de niveles desde: %s",*simboloRecibido,nivelDelPersonaje);
	//						if(recibirMensaje(socketNuevaConexion, (void**) &nivelDelPersonaje)>=0) {

								//Enviar info de conexión (IP y port) del Nivel y el Planificador asociado a ese nivel
								//settear variable global para usar en funcion q se manda a list_find
								nombreNivel=nivelDelPersonaje;
								log_debug(logOrquestador,"Se busca el nivel");
								NodoNivel* nivel =list_find(listaNiveles,esMiNivel);
								log_info(logOrquestador,"Se encontro el nivel %p",nivel);
								if(nivel!=NULL){
									log_info(logOrquestador,"El nivel existe (o sea, es != NULL)");
									strcpy(msj.ipNivel,nivel->ip);
									msj.portNivel=nivel->port;
									msj.portPlanificador=nivel->puertoPlanif;
									log_info(logOrquestador,"Se copiaron los datos del nivel al mensaje para mandar al Personaje %c",*simboloRecibido);
								}

								if(mandarMensaje(socketNuevaConexion,1,sizeof(ConxNivPlan),&msj)>=0){
									log_info(logOrquestador,"Se enviaron los datos del nivel al Personaje %c",*simboloRecibido);
								}
								else {
									log_error(logOrquestador,"No se pudo enviar los datos del nivel al Personaje %c",*simboloRecibido);
								}
							}
							break;

						case 5: //el pj termina su plan de nivel
							nivelDelPersonaje=malloc(unHeader.payloadlength);
							if(recibirData(socketNuevaConexion,unHeader,(void**)nivelDelPersonaje)){
								g_contPersonajes--;
								log_info(logOrquestador,"El personaje %c termino su plan de niveles",*simboloRecibido);

								if(g_contPersonajes==0){
									flagGlobalFin=0;
								}
								log_debug(logOrquestador,"Personajes Restantes: %d Fin: %d",g_contPersonajes,flagGlobalFin);
							}
							break;
						default:
							break;
						}
					}

					//cerrar socket
					close(socketNuevaConexion);
					log_info(logOrquestador,"Se cerro la conexion con el personaje %c",*simboloRecibido);
					break;

				case 2: //case Nivel
					simboloRecibido=malloc(unHeader.payloadlength);
					if(recibirData(socketNuevaConexion,unHeader,(void**)simboloRecibido)>=0){
						if (mandarMensaje(socketNuevaConexion,0 , sizeof(char),simboloRecibido) > 0) {
							//log_info(logger,"Mando mensaje al personaje %c",*rec);
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
					nodoNivel->socket=socketNuevaConexion;
					nodoNivel->deadlockActivado=varGlobalDeadlock;
					nodoNivel->sleepDeadlock=varGlobalSleepDeadlock;
					list_add(listaNiveles,nodoNivel);
					log_info(logOrquestador,"Se anadio el nivel %s a la lista de Niveles",simboloRecibido);

					InfoNivel *nivel;
					nivel=malloc(sizeof(InfoNivel));
					strcpy(nivel->ip,ipCliente);
					nivel->nombre=simboloRecibido;
					nivel->port=nodoNivel->port;
					nivel->puertoPlanif=contadorPuerto;
					nivel->nodoN=nodoNivel;
					contadorPuerto++;

					int* aux;
					aux=malloc(sizeof(int));
					*aux=nodoNivel->port;
					//manda el puerto asignado al nivel para que escuche conexiones.
					//TODO: y si es -1 de qué nos pintamos?
					if(!mandarMensaje(socketNuevaConexion,1, sizeof(int),aux)){
						log_error(logOrquestador,"No se pudo enviar un mensaje al nivel %s de socket %d",simboloRecibido,socketNuevaConexion);
						break;}
					free(aux);
					if(!recibirMensaje(socketNuevaConexion,(void**)&aux)){
						log_error(logOrquestador,"No se pudo recibir un mensaje del nivel %s de socket %d",simboloRecibido,socketNuevaConexion);
						break; }
					free(aux);

					//******MANDAMOS LA INFO AL DEADLOCK DE CADA NIVEL*******
					int* aux2;
					aux2=malloc(sizeof(int));
					*aux2=nodoNivel->deadlockActivado;

					if(!mandarMensaje(socketNuevaConexion,1, sizeof(int),aux2)){
						log_error(logOrquestador,"No se pudo enviar un mensaje al nivel %s de socket %d",simboloRecibido,socketNuevaConexion);
						break;}
					free(aux2);
					if(!recibirMensaje(socketNuevaConexion,(void**)&aux2)){
						log_error(logOrquestador,"No se pudo recibir un mensaje del nivel %s de socket %d",simboloRecibido,socketNuevaConexion);
						break; }
					free(aux2);

					int* aux3;
					aux3=malloc(sizeof(int));
					*aux3=nodoNivel->sleepDeadlock;
					if(!mandarMensaje(socketNuevaConexion,1, sizeof(int),aux3)){
						log_error(logOrquestador,"No se pudo enviar un mensaje al nivel %s de socket %d",simboloRecibido,socketNuevaConexion);
						break;}
					free(aux3);
					if(!recibirMensaje(socketNuevaConexion,(void**)&aux3)){
						log_error(logOrquestador,"No se pudo recibir un mensaje del nivel %s de socket %d",simboloRecibido,socketNuevaConexion);
						break; }
					free(aux3);
					//******FIN DE MANDAMOS LA INFO AL DEADLOCK DE CADA NIVEL*******



					pthread_t threadPlanif;
					pthread_create(&threadPlanif, NULL, planificador, (void *)nivel);
					log_info(logOrquestador,"Se creo el THR del planificador correspondiente al nivel %s",simboloRecibido);
					break;
				default:
					break;
				}
			}
		}
	}//Cierra While(1)

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
//	printf("%s %s\n",nodo->nombreNivel,nombreNivel);

	if(strcmp(nodo->nombreNivel,nombreNivel)==0)
		return true;
	return false;
}

void desconexionPersonaje(int socketDesconectar, MensajePersonaje msjPersonaje, int quantum, t_queue * colaListos){
	queue_pop(colaListos);
	msjPersonaje.solicitaRecurso=0;
	msjPersonaje.bloqueado=0;
	quantum=varGlobalQuantum+1;
	g_contPersonajes--;
	close(socketDesconectar);
}
//----------------------------------------------------------------
