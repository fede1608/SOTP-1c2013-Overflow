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
#include <pthread.h>
#include "collections/queue.h"
#include  "socketsOv.h" //Librería compartida para sockects de overflow

//----------------------------------------------------------------

//******************** DEFINICIONES GLOBALES *********************
//Variables estándar del sistema
int varGlobalQuantum=3;
int varGlobalSleep=1;

//Structs propios de la plataforma

typedef struct t_infoPlanificador {
	int port;
	t_queue* colaListos;
} InfoPlanificador;

typedef struct t_infoNivel {
	int numero;
	char ip[20];
	int port;
} InfoNivel;

typedef struct t_nodoPersonaje {//TODO completar segun las necesidades
	char simboloRepresentativo; //Ej: @ ! / % $ &
	int socket;
} NodoPersonaje;

typedef struct t_msjPersonaje {
	int solicitaRecurso;
	int bloqueado;
	int finNivel;
} MensajePersonaje;

//----------------------------------------------------------------

//************************** PROTOTIPOS **************************

//Prototipos de funciones a utilizar. Se definien luego de la función main.
int planificador (InfoNivel* nivel);
int orquestador (void);
int listenerPersonaje(InfoPlanificador* planificador);

//----------------------------------------------------------------

//******************** FUNCIONES PRINCIPALES *********************

//MAIN:
//Función principal del proceso, es la encargada de crear al orquestador y al planificador
//Cardinalidad = un único thread
int main (void) {

	printf("Proceso plataforma iniciado\nCreando THR Orquestador...\n");

	pthread_t thr_orquestador;
	pthread_create(&thr_orquestador, NULL, orquestador, NULL);

	pthread_join(thr_orquestador, NULL); //Esperamos que termine el thread orquestador
	pthread_detach(thr_orquestador);

	printf("Proceso plataforma finalizado correctamente\n");
	return 0;
}
//FIN DE MAIN


//PLANIFICADOR:
//Función que será ejecutada a través de un hilo para atender la planificación de cada nivel
//Cardinalidad = 0 hasta N threads ejecutandose simultaneamente, uno por nivel
int planificador (InfoNivel* nivel) {

//	//Se informa del estado del thread por pantalla: inciado
//	char* nombreDePlanificador = malloc(sizeof("THR Planificador Nivel ") + sizeof(int));
//	strcpy(nombreDePlanificador, "THR Planificador Nivel ");
//	sprintf(nombreDePlanificador + (strlen(nombreDePlanificador)), "%d", numeroDeNivel);
//	printf("%s: iniciado\n", nombreDePlanificador);
//
//	//Se informa del estado del thread por pantalla: terminado
//	printf("%s: terminado\n", nombreDePlanificador);
//return EXIT_SUCCESS;

	//cola de personajes listos y bloqueados
	t_queue *colaListos=queue_create();
	t_queue *colaBloqueados=queue_create();

	//TODO implementar handshake Conectarse con el nivel
	//int socketNivel=quieroUnPutoSocketAndando(nivel->ipN,nivek->portN);
	printf("%s %d\n",nivel->ip,nivel->port);

	// Creamos el listener de personajes del planificador (guardar socket e info del pj en la estructura)
	void* nodoAux;
	InfoPlanificador *planificadorActual;
	planificadorActual=malloc(sizeof(InfoPlanificador));
	planificadorActual->colaListos=colaListos;
	//pasar puerto de la plataforma
	planificadorActual->port=5001;
	//Este thread se encargará de esuchar nuevas conexiones de personajes indefinidamente (ver función listenerPersonaje)
	pthread_t threadPersonaje;
	pthread_create(&threadPersonaje, NULL, listenerPersonaje, (void *)planificadorActual);

	//Se necesita algun char (cualquiera) para poder usar la función de enviar mensaje más adelante
	char* auxcar;
	auxcar=malloc(1);
	*auxcar='T';

	int quantum=varGlobalQuantum;

	while(1){
		//Si la no esta vacia
		if(!queue_is_empty(colaListos)){

			NodoPersonaje *personajeActual; //todo Revisar que los punteros de personajeActual anden bien

			if(quantum>0){
				//Mandar mensaje de movimiento permitido al socket del personaje del nodo actual (primer nodo de la cola)
				personajeActual = (NodoPersonaje*) queue_peek(colaListos);
				mandarMensaje(personajeActual->socket, 8, 1, auxcar);
			}
			else {
				quantum=varGlobalQuantum;

				//Sacar el nodo actual (primer nodo de la cola) y enviarlo al fondo de la misma
				//todo Sincronizar colaListos con el Listener
				nodoAux=queue_pop(colaListos);
				queue_push(colaListos,nodoAux);

				//Buscar el primero de la cola de listos y mandarle un mensaje de movimiento permitido
				personajeActual = (NodoPersonaje*) queue_peek(colaListos);
				mandarMensaje(personajeActual->socket, 8, 1, auxcar);

			}

			//Esperar respuesta de turno terminado (con la info sobre si quedo bloqueado y si tomo recurso)
			Header headerMsjPersonaje;
			MensajePersonaje msjPersonaje;
			recibirHeader(personajeActual->socket, &headerMsjPersonaje);
			recibirData(personajeActual->socket, headerMsjPersonaje, (void**) &msjPersonaje);

			//Comportamientos según el mensaje que se recibe del personaje

			//Si informa de fin de nivel se lo retira de la cola de listos
			if(msjPersonaje.finNivel){
				queue_pop(colaListos);
			}

			//Si solicita recurso y NO quedo bloqueado {quantum=varGlogalQuantum; poner al final de la cola}
			if(msjPersonaje.solicitaRecurso & !msjPersonaje.bloqueado){
				printf("%d",quantum);
				quantum=varGlobalQuantum;
				printf("%d",quantum);
				queue_push(colaListos,queue_pop(colaListos));
			}

			//Si solicita recurso y SI quedo bloqueado {quatum=varGlogalQuantum; poner al final de la cola de bloquedados}
			if(msjPersonaje.solicitaRecurso & msjPersonaje.bloqueado){
				printf("%d",quantum);
				quantum=varGlobalQuantum;
				printf("%d",quantum);
				queue_push(colaBloqueados,queue_pop(colaListos));
			}

			printf("Q %d\n",quantum);
			quantum--;
			printf("Q %d\n",quantum);
			sleep(varGlobalSleep);
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

	int socketNuevaConexion;
	//Struct con info de conexión (IP y puerto) del nivel y el planificador asociado a ese nivel
	typedef struct t_infoConxNivPlan {
		char ipNivel[16];
		int portNivel;
		char ipPlanificador[16];
		int portPlanificador;
	} ConxNivPlan;
	ConxNivPlan msj;

	//todo Desharcodear IPs de planificador y nivel
	strcpy(msj.ipNivel,"127.0.0.1");
	strcpy(msj.ipPlanificador,"127.0.0.1");
	msj.portNivel=5000;
	msj.portPlanificador=5001;
	//printf("THR Orquestador: iniciado.\n THR Orquestador: Esperando solicitudes de personajes...\n");

	//TODO llamar al handler de niveles
	InfoNivel *nivel;
	nivel=malloc(sizeof(InfoNivel));
	strcpy(nivel->ip,"127.0.0.1");
	nivel->numero=1;
	nivel->port=5000;
	pthread_t threadPersonaje;
	pthread_create(&threadPersonaje, NULL, planificador, (void *)nivel);

	//Creamos un socket de esucha para el puerto 4999
	int socketEscucha = quieroUnPutoSocketDeEscucha(4999);
	while(1){

		if (listen(socketEscucha, 1) != 0) {
			perror("Error al poner a escuchar socket");
			return EXIT_FAILURE;
		}
		if ((socketNuevaConexion = accept(socketEscucha, NULL, 0)) < 0) {
			//log_error(logger,"Error al aceptar conexion entrante");
			return EXIT_FAILURE;
		}

		//Handshake en el que recibe el simbolo del personaje
		char *simboloRecibido; //Ej: @ ! / % $ &
		if(recibirMensaje(socketNuevaConexion, (void**) &simboloRecibido)>=0) {
			if (mandarMensaje(socketNuevaConexion,0 , 1,simboloRecibido)) {
				//log_info(logger,"Mando mensaje al personaje %c",*rec);
				printf("%c",*simboloRecibido);
			}
		}
		Header unHeader;
		int* intaux;
		//Esperar solicitud de info de conexion de Nivel y Planifador
		if(recibirHeader(socketNuevaConexion,&unHeader)){
			if(recibirData(socketNuevaConexion,unHeader,(void**)&intaux)){
				//Enviar info de conexión (IP y port) del Nivel y el Planificador asociado a ese nivel
				mandarMensaje(socketNuevaConexion,1,sizeof(ConxNivPlan),&msj);
			}
		}
		//cerrar socket
		close(socketNuevaConexion);

	}//Cierra While(1)

	//log_info(logger,"Mando el socket %d (Thread)", personaje->socket);
	//
	//pthread_t threadPersonaje;
	//
	//pthread_create(&threadPersonaje, NULL, han, (void *)msj);


	//	//TEST:Creamos 5 threads a modo de prueba
	//	int i;
	//	pthread_t vectorDeThreads[4];
	//	for (i=0; i<5; i++){
	//
	//		int numeroDeNivel = i;
	//		printf("Creando THR Planificador para nivel %d...\n",numeroDeNivel);
	//
	//		pthread_create(&vectorDeThreads[i], NULL, planificador, numeroDeNivel);
	//	};
	//
	//	//Esperamos que termine cada thread de los planificadores creados
	//	for (i=0; i<5; i++){
	//		pthread_join(vectorDeThreads[i], NULL);
	//		pthread_detach(vectorDeThreads[i]);
	//
	//	}

	//	printf("THR Orquestador: terminado\n");

	return EXIT_SUCCESS;
}
//FIN DE ORQUESTADOR

//******************* FUNCIONES AUXILIARES ********************

//listenerPersonaje:
//Función que escucha nuevas conexiones de personajes y
//será ejecutada a través de un hilo por un planificador
//Cardinalidad = 0 hasta N threads ejecutandose simultaneamente, uno por planificador
int listenerPersonaje(InfoPlanificador* planificador){

	    int socketEscucha,socketNuevaConexion;
	    socketEscucha=quieroUnPutoSocketDeEscucha(planificador->port);

	    //Ciclo While(1) para escuchar nuevos personajes indefinidamente
		while (1){

			// Escuchar nuevas conexiones entrantes.
			if (listen(socketEscucha, 1) != 0) {
				//log_error(logger,"Error al bindear socket escucha");
				return EXIT_FAILURE;
			}
			//log_info(logger,"Escuchando conexiones entrantes");

			// Aceptar una nueva conexion entrante. Se genera un nuevo socket con la nueva conexion.
			// La funcion accept es bloqueante, no sigue la ejecución hasta que se reciba algo
			if ((socketNuevaConexion = accept(socketEscucha, NULL, 0)) < 0) {
				//log_error(logger,"Error al aceptar conexion entrante");
				return EXIT_FAILURE;
			}

			//Handshake en el que recibe el simbolo del personaje
			char *simboloRecibido; //Ej: @ ! / % $ &
			if(recibirMensaje(socketNuevaConexion, (void**) &simboloRecibido)>=0) {
				//log_info(logger,"Llego el Personaje %c del nivel",*simboloRecibido);
				if (mandarMensaje(socketNuevaConexion,0 , 1,simboloRecibido)) {
					//log_info(logger,"Mando mensaje al personaje %c",*simboloRecibido);
					printf("sth");
				}

				//Agrega el nuevo personaje a la cola de listos del planificador
				NodoPersonaje* personaje;
				personaje=malloc(sizeof(NodoPersonaje));
				personaje->simboloRepresentativo=*simboloRecibido; //Ej: @ ! / % $ &
				personaje->socket=socketNuevaConexion;
				queue_push(planificador->colaListos,personaje);

				//log_info(logger,"Mando el socket %d (Thread)", personaje->socket);

			}

		}//Cierra el while(1)

	    return EXIT_SUCCESS;
	}

//----------------------------------------------------------------
