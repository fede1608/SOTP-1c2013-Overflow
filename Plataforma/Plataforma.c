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

//************************* DEFINICIONES *************************

//----------------------------------------------------------------
//----------------------------------------------------------------

//************************* Structs *************************

typedef struct t_infomsj {
	int port;
	t_queue* colaL;
} Info;
typedef struct t_infomsj2 {
	int nivel;
	char ipN[20];
	int portN;
} InfoP;
typedef struct t_nodoPer {//TODO completar segun las necesidades
	char per;
	int socket;
} NodoPersonaje;
//----------------------------------------------------------------
//************************** PROTOTIPOS **************************

//Prototipos de funciones a utilizar. Se definien luego de la función main.
int planificador (InfoP* infoP);
int orquestador (void);
int listenerP(Info* info);

//----------------------------------------------------------------

//************************** FUNCIONES ***************************

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
};
//FIN DE MAIN


//PLANIFICADOR:
//Función que será ejecutada a través de un hilo para atender la planificación de cada nivel
//Cardinalidad = 0 hasta N threads ejecutandose simultaneamente, uno por nivel
int planificador (InfoP* infoP) {

//	//Se informa del estado del thread por pantalla: inciado
//	char* nombreDePlanificador = malloc(sizeof("THR Planificador Nivel ") + sizeof(int));
//	strcpy(nombreDePlanificador, "THR Planificador Nivel ");
//	sprintf(nombreDePlanificador + (strlen(nombreDePlanificador)), "%d", numeroDeNivel);
//	printf("%s: iniciado\n", nombreDePlanificador);
//
//	//Se informa del estado del thread por pantalla: terminado
//	printf("%s: terminado\n", nombreDePlanificador);
//return EXIT_SUCCESS;


	t_queue *colaListos=queue_create();
	t_queue *colaBloqueados=queue_create();

	//TODO Conectarse con el nivel

	//TODO Abrir listener de Personajes (guardar socket e info del pj en la estructura)

	//while(1){ si la lista no apunta a null{
	//			si quantum>0{
	//				Mandar mensaje de mov permitido al socket del personaje del nodo actual(primer nodo de la cola)}
	//			else if(lista no es null){
	//
	//			quatum=varGlogalQuantum;
	//			buscar el primero de la cola de listos y mandarle un msj de mov permitido}
	//			esperar respuesta de turno terminado(con la info sobre si quedo bloqueado y si tomo recurso);
	//			si (tomo recurso& no quedo bloqueado) {quantum=varGlogalQuantum; poner al final de la cola}
	//			si (pidio recurso& quedo bloqueado){quatum=varGlogalQuantum; poner alfinal de la cola de bloquedados}
	//			sleep(varGlobalSleep);
	//			}
	//		}




	return 0;
};
//FIN DE PLANIFICADOR


//ORQUESTADOR:
//Función que será ejecutada a través de un hilo para asignar un nivel y un planificador a cada
//proceso personaje que se conecte.
//Cardinalidad = un único thread que atiende las solicitudes
int orquestador (void) {
	int socketNuevaConexion;
	typedef struct t_infoNP {
		char ipN[16];
		int portN;
		char ipP[16];
		int portP;
	} IPNivPlan;
	IPNivPlan msj;
	strcpy(msj.ipN,"127.0.0.1");
	strcpy(msj.ipP,"127.0.0.1");
	msj.portN=5000;
	msj.portP=5001;
	//	printf("THR Orquestador: iniciado.\n THR Orquestador: Esperando solicitudes de personajes...\n");
	//
	//	//Creamos un socket de esucha para el puerto 4999
	int socketEscucha = quieroUnPutoSocketDeEscucha(4999);
	while(1){
		if (listen(socketEscucha, 1) != 0) {
			perror("Error al poner a escuchar socket");
			return EXIT_FAILURE;
		}
		if ((socketNuevaConexion = accept(socketEscucha, NULL, 0)) < 0) {
		//	log_error(logger,"Error al aceptar conexion entrante");
			return EXIT_FAILURE;
		}

		//Handshake en el que recibe la letra del personaje
		char* rec;
		if(recibirMensaje(socketNuevaConexion, (void**)&rec)>=0) {
		if (mandarMensaje(socketNuevaConexion,0 , 1,rec)) {
		//	log_info(logger,"Mando mensaje al personaje %c",*rec);
			printf("%c",*rec);
		}
		Header unHeader;
		int* intaux;
		//esperar solicitud de info nivel/Planif
		if(recibirHeader(socketNuevaConexion,&unHeader)){
			if(recibirData(socketNuevaConexion,unHeader,(void**)&intaux)){
			//Obtener info de ip & port
				mandarMensaje(socketNuevaConexion,1,sizeof(IPNivPlan),msj);
			}
		}

	}
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

//----------------------------------------------------------------
int listenerP(Info* info){

	    int socketEscucha,socketNuevaConexion;
	    socketEscucha=quieroUnPutoSocketDeEscucha(info->port);

	            while (1){
	            	// Escuchar nuevas conexiones entrantes.
	            if (listen(socketEscucha, 1) != 0) {
//	                log_error(logger,"Error al bindear socket escucha");
	                return EXIT_FAILURE;
	            }
//	            	log_info(logger,"Escuchando conexiones entrantes");

	                // Aceptar una nueva conexion entrante. Se genera un nuevo socket con la nueva conexion.
	                // La funcion accept es bloqueante, no sigue la ejecuci贸n hasta que se reciba algo
	                if ((socketNuevaConexion = accept(socketEscucha, NULL, 0)) < 0) {
//	                	log_error(logger,"Error al aceptar conexion entrante");
	                    return EXIT_FAILURE;
	                }

	                //Handshake en el que recibe la letra del personaje
	                char* rec;
	                if(recibirMensaje(socketNuevaConexion, (void**)&rec)>=0) {

//	                	log_info(logger,"Llego el Personaje %c del nivel",*rec);

	                    if (mandarMensaje(socketNuevaConexion,0 , 1,rec)) {

//	                    	log_info(logger,"Mando mensaje al personaje %c",*rec);
	                    	printf("sth");
	                    }
	                    NodoPersonaje* nodo;
	                    nodo->per=*rec;
	                    nodo->socket=socketNuevaConexion;
	                    queue_push(info->colaL,nodo);


	                    //Agrega personaje a la lista y devuelve nodo

//	                    log_info(logger,"Mando el socket %d (Thread)", personaje->socket);



	                }



	            }
	    return 1;
	}
