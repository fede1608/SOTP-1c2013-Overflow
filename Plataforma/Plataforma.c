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
#include <unistd.h>
#include "collections/queue.h"
#include "collections/list.h"
#include  "socketsOv.h" //Librería compartida para sockects de overflow

//----------------------------------------------------------------

//******************** DEFINICIONES GLOBALES *********************
//Variables estándar del sistema
int varGlobalQuantum=3;
int varGlobalSleep=1* 1000000;

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
	char recursoPedido;
} NodoPersonaje;

typedef struct t_nodoNivel {//TODO completar segun las necesidades
	char* nombreNivel; //Ej: @ ! / % $ &
	char ip[20];
	int port;
} NodoNivel;

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
	*auxcar='P';

	int quantum=varGlobalQuantum;

	while(1){
		//Si la no esta vacia
		if(!queue_is_empty(colaListos)){
			printf("La lista no esta vacia\n");
			NodoPersonaje *personajeActual; //todo Revisar que los punteros de personajeActual anden bien

			if(quantum>0){
				printf("Quantum > 0\n");
				//Mandar mensaje de movimiento permitido al socket del personaje del nodo actual (primer nodo de la cola)
				personajeActual = (NodoPersonaje*) queue_peek(colaListos);
				mandarMensaje(personajeActual->socket, 8, 1, auxcar);
				printf("Se mando Mov permitido al personaje %c\n",personajeActual->simboloRepresentativo);
			}
			else {
				quantum=varGlobalQuantum;
				printf("Se acabo el quantum");
				//Sacar el nodo actual (primer nodo de la cola) y enviarlo al fondo de la misma
				//todo Sincronizar colaListos con el Listener
				nodoAux=queue_pop(colaListos);
				printf("Se saco de la cola de listos");
				queue_push(colaListos,nodoAux);
				printf("Se puso al final de la cola de listos");
				//Buscar el primero de la cola de listos y mandarle un mensaje de movimiento permitido
				personajeActual = (NodoPersonaje*) queue_peek(colaListos);
				printf("Se saco al primer personaje de la cola");
				mandarMensaje(personajeActual->socket, 8, 1, auxcar);
				printf("Se mando Mov permitido al personaje %c\n",personajeActual->simboloRepresentativo);
			}

			//Esperar respuesta de turno terminado (con la info sobre si quedo bloqueado y si tomo recurso)
			Header headerMsjPersonaje;
			MensajePersonaje msjPersonaje;
			printf("Esperando respuesta de turno concluido\n");
			recibirHeader(personajeActual->socket, &headerMsjPersonaje);
			recibirData(personajeActual->socket, headerMsjPersonaje, (void**) &msjPersonaje);
			printf("Respuesta recibida\n");
			//Comportamientos según el mensaje que se recibe del personaje

			//Si informa de fin de nivel se lo retira de la cola de listos
			if(msjPersonaje.finNivel){
				printf("El personaje termino el Nivel\n");
				queue_pop(colaListos);
				msjPersonaje.solicitaRecurso=0;
				msjPersonaje.bloqueado=0;
				quantum=varGlobalQuantum+1;
				printf("Se retiro al personaje de la cola\n");
			}

			//Si solicita recurso y NO quedo bloqueado {quantum=varGlogalQuantum; poner al final de la cola}
			if(msjPersonaje.solicitaRecurso & !msjPersonaje.bloqueado){
				printf("Rec no bloq1 %d\n",quantum);
				quantum=varGlobalQuantum+1;
				queue_push(colaListos,queue_pop(colaListos));
				printf("Rec no bloq2 %d\n",quantum);
			}

			//Si solicita recurso y SI quedo bloqueado {quatum=varGlogalQuantum; poner al final de la cola de bloquedados}
			if(msjPersonaje.solicitaRecurso & msjPersonaje.bloqueado){
				printf("Rec bloq1 %d",quantum);
				quantum=varGlobalQuantum+1;
				queue_push(colaBloqueados,queue_pop(colaListos));
				printf("Rec bloq2 %d",quantum);
			}

			printf("Q %d\n",quantum);
			quantum--;
			printf("Q %d\n",quantum);
			usleep(varGlobalSleep);
		}else usleep(10000); //para que no quede en espera activa
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
		simboloRecibido=malloc(1);
		Header unHeader;
		if(recibirHeader(socketNuevaConexion,&unHeader)){

		if(recibirData(socketNuevaConexion,unHeader,(void**)simboloRecibido)>=0){



//		if(recibirMensaje(socketNuevaConexion, (void**) &simboloRecibido)>=0) {
			if (mandarMensaje(socketNuevaConexion,0 , 1,simboloRecibido)) {
				//log_info(logger,"Mando mensaje al personaje %c",*rec);
				printf("Entro Personaje: %c\n",*simboloRecibido);
			}
		}
		}

		char* nivelDelPersonaje;
		//Esperar solicitud de info de conexion de Nivel y Planifador
		printf("Esperando solictud de nivel\n");
//		if(recibirHeader(socketNuevaConexion,&unHeader)){
//			printf("Info Header: %d %d\n",unHeader.payloadlength,unHeader.type);
//			if(recibirData(socketNuevaConexion,unHeader,(void**)nivelDelPersonaje)){
		if(recibirMensaje(socketNuevaConexion, (void**) &nivelDelPersonaje)>=0) {
				printf("Nivel recibido: %s\n",nivelDelPersonaje);
				//Enviar info de conexión (IP y port) del Nivel y el Planificador asociado a ese nivel
				mandarMensaje(socketNuevaConexion,1,sizeof(ConxNivPlan),&msj);
//			}
		}
		//cerrar socket
		close(socketNuevaConexion);

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
