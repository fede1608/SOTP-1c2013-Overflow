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
#include  "socketsOv.h" //Librería compartida para sockects de overflow

//----------------------------------------------------------------

//************************* DEFINICIONES *************************

//----------------------------------------------------------------

//************************** PROTOTIPOS **************************

//Prototipos de funciones a utilizar. Se definien luego de la función main.
int planificador (int numeroDeNivel);
int orquestador (void);

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
int planificador (int numeroDeNivel) {

	//Se informa del estado del thread por pantalla: inciado
	char* nombreDePlanificador = malloc(sizeof("THR Planificador Nivel ") + sizeof(int));
	strcpy(nombreDePlanificador, "THR Planificador Nivel ");
	sprintf(nombreDePlanificador + (strlen(nombreDePlanificador)), "%d", numeroDeNivel);
	printf("%s: iniciado\n", nombreDePlanificador);

	//Se informa del estado del thread por pantalla: terminado
	printf("%s: terminado\n", nombreDePlanificador);
	return EXIT_SUCCESS;
};
//FIN DE PLANIFICADOR


//ORQUESTADOR:
//Función que será ejecutada a través de un hilo para asignar un nivel y un planificador a cada
//proceso personaje que se conecte.
//Cardinalidad = un único thread que atiende las solicitudes
int orquestador (void) {

	printf("THR Orquestador: iniciado.\n THR Orquestador: Esperando solicitudes de personajes...\n");

	//Creamos un socket de esucha para el puerto 5151
	int socketEscucha = quieroUnPutoSocketDeEscucha(5151);

	if (listen(socketEscucha, 10) != 0) {
		perror("Error al poner a escuchar socket");
		return EXIT_FAILURE;
	}

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

	printf("THR Orquestador: terminado\n");
	return EXIT_SUCCESS;
};
//FIN DE ORQUESTADOR

//----------------------------------------------------------------
