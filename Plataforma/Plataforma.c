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
#include "temporal.h"

//----------------------------------------------------------------

//************************* DEFINICIONES *************************

//Define la estructura que debe recibir la función logear para registrar un evento
typedef struct t_log {
	int tipo; //Tipo de evento, ejemplos: INFO = 0, DEBUG = 1, WARN = 2, ERROR = 3
	char nombreDeElemento[64]; //Nombre de quien genera el evento, ejemplo: THR ORQUESTADOR o THR PLANIFICADOR (NIVEL 1)
	char descripcion[128]; //Descripción del evento
} Log;

//----------------------------------------------------------------

//************************** PROTOTIPOS **************************

//Prototipos de funciones a utilizar. Se definien luego de la función main.
void planificador(int numeroDeNivel);
void orquestador (void);
void logear (Log elementoALogear);

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

//LOGEAR:
//Función que agrega un evento al log que se encuentra en disco.
//Cardinalidad = 0 hasta N threads ejecutandose secuencialmente
//Dependencias: <unistd.h> y <sys/types.h>
void logear (Log elementoALogear) {
    char ruta[1024];//Acá se va a guardar la ruta en donde se encuentra la Plataforma

    //Obtener y revisar que la ruta actual no sea nula (se guarda en "ruta")
    if (getcwd(ruta, sizeof(ruta)) != NULL) {

    	char* cadena = malloc(512); //TODO: Optimizar la cantidad de memoria alocada

        switch(elementoALogear.tipo){
			case 0:
				strcpy(cadena,"[INFO] [");
				break;
			case 1:
				strcpy(cadena,"[DEBUG] [");
				break;
			case 2:
				strcpy(cadena,"[WARN] [");
				break;
			case 3:
				strcpy(cadena,"[ERROR] [");
				break;

			//Hay un caso default en caso de que el tipo de evento no se reconozca,
			//en tal caso se termina la ejecución del logeo usando return;
			default:
				printf("Error al logear el evento: no se reconoce el tipo\n");
				return;
        }

        //generamos una cadena con toda la información necesaria para el log
    	strcat(cadena, temporal_get_string_time() );
    	strcat(cadena, "] [");
    	strcat(cadena, elementoALogear.nombreDeElemento);
    	strcat(cadena, "]/(");

    	//Se usa cadena + (strlen(cadena) para que sprintf concatene a partir del último
    	//caracter de la cadena, sino empieza por la posición 0 y sobre escribe la cadena
    	sprintf(cadena + (strlen(cadena)), "%d",getpid()); //getpid() devuelve un int que tenemos que meter en un string, por eso usamos sprintf
    	strcat(cadena,":");
    	//TODO: Revisar porque no funciona el gettid()
    	sprintf(cadena + (strlen(cadena)),"%d):", pthread_self()));//Sucede lo mismo que con getpid()
    	strcat(cadena, elementoALogear.descripcion);
    	strcat(cadena,"\n");

    	//Abrimos el archivo log.txt para append
    	FILE *archivoLog;
    	strcat(ruta, "/log.txt");
    	archivoLog = fopen (ruta, "a");
    	fputs(cadena, archivoLog);
    	fclose (archivoLog);

    }
    else perror("Error al logear el evento: no se encuentra la ruta actual");

};
//FIN DE LOGEAR

//PLANIFICADOR:
//Función que será ejecutada a través de un hilo para atender la planificación de cada nivel
//Cardinalidad = 0 hasta N threads ejecutandose simultaneamente, uno por nivel
void planificador (int numeroDeNivel) {

	char* nombreDePlanificador = malloc(sizeof("THR Planificador Nivel ") + sizeof(int));
	strcpy(nombreDePlanificador, "THR Planificador Nivel ");

	sprintf(nombreDePlanificador + (strlen(nombreDePlanificador)), "%d", numeroDeNivel);

	printf("%s: iniciado\n", nombreDePlanificador);

	Log logDePlanificador;
	logDePlanificador.tipo = 0;
	strcpy(logDePlanificador.nombreDeElemento, nombreDePlanificador);
	strcpy(logDePlanificador.descripcion, "Iniciado");

	logear(logDePlanificador);

	printf("%s: terminado\n", nombreDePlanificador);
};
//FIN DE PLANIFICADOR

//ORQUESTADOR:
//Función que será ejecutada a través de un hilo para asignar un nivel y un planificador a cada
//proceso personaje que se conecte.
//Cardinalidad = un único thread que atiende las solicitudes
void orquestador (void) {

	printf("THR Orquestador: iniciado\n");

	//Creamos 5 threads a modo de prueba
	int i;
	pthread_t vectorDeThreads[4];
	for (i=0; i<5; i++){

		int numeroDeNivel = i;
		printf("Creando THR Planificador para nivel %d...\n",numeroDeNivel);

		pthread_create(&vectorDeThreads[i], NULL, planificador, numeroDeNivel);
	};

	//Esperamos que termine cada thread de los planificadores creados
	for (i=0; i<5; i++){
		pthread_join(vectorDeThreads[i], NULL);
		pthread_detach(vectorDeThreads[i]);

	}

	printf("THR Orquestador: terminado\n");

};
//FIN DE ORQUESTADOR

//----------------------------------------------------------------
