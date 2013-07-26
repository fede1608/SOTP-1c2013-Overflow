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

//************************** CONSTANTES **************************
#define EVENT_SIZE ( sizeof (struct inotify_event))
#define EVENT_BUF_LEN ( 1024 * ( EVENT_SIZE + 16 ) )
//----------------------------------------------------------------

//******************** DEFINICIONES GLOBALES *********************
//Variables Globales del sistema
//int varGlobalQuantum=3;
//int varGlobalSleep=0.5* 1000000;
int varGlobalQuantum;
int varGlobalSleep;
int varGlobalDeadlock;
int varGlobalSleepDeadlock;
int flagGlobalFin=1; // Flag que informa el fin del programa. Por defecto está en 1 = No termino.
int g_contPersonajes=0; //Contador global de personajes. Por defecto está en 0.
char* nombreNivel; //Es usado por la funcion esMiNivel que se manda al list_find
t_list* listaNiveles; //Variable global utilizada dentro del planificador y orquestador

//Structs propios de la plataforma

typedef struct t_nodoRec {
char id;
int cantAsignada;
} NodoRecurso;

typedef struct t_infoPlanificador {
	int port;
	t_queue* colaListos;
	t_queue* colaBloqueados;
	int contPersonajes;
} InfoPlanificador;

typedef struct t_nodoNivel {//TODO completar segun las necesidades
	char* nombreNivel;
	char ip[20];
	int port;
	int puertoPlanif;
	int socket;
	int deadlockActivado;
	int sleepDeadlock;
	t_queue * colaListos;
	t_queue * colaBloqueados;
	pthread_mutex_t sem;
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
	int orden;
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

//Prototipos de funciones a utilizar. Se definen luego de la función main.
int planificador (InfoNivel* nivel);
int orquestador (void);
int listenerPersonaje(InfoPlanificador* planificador, int socketEscucha);
char* print_ip(int ip);
bool esMiNivel(NodoNivel* nodo);
void desconexionPersonaje(int socketDesconectar, MensajePersonaje msjPersonaje, int quantum, t_queue * colaListos);
//----------------------------------------------------------------

//******************** FUNCIONES PRINCIPALES *********************

//MAIN:
//Función principal del proceso, es la encargada de crear al orquestador y al planificador
//Cardinalidad = un único thread
int main (void) {

	//Se crea la lista que va a contener niveles (se usa en el orquestador y planificador)
	listaNiveles=list_create();

	//Inicialización de los logs para el orquestador y los planificadores
	logOrquestador = log_create("LogOrquestador.log","Orquestador",true,detail);
	logPlanificador = log_create("LogPlanificador.log","Planificador",true,detail);

	//Se carga el archivo de configuración que utiliza la plataforma
	t_config* config = config_create("config.txt");
	varGlobalQuantum=config_get_int_value(config,"Quantum");
	//Tener en cuenta que en el archivo de configuración los valores de los tiempos están en
	//segundos y es necesario pasarlos a micro segundos para usarlos en las funciones sleep
	varGlobalSleep=(int)(config_get_double_value(config,"TiempoDeRetardoDelQuantum")* 1000000);
	varGlobalDeadlock=config_get_int_value(config,"AlgoritmoDeDeteccionDeInterbloqueo");
	varGlobalSleepDeadlock=(int)(config_get_double_value(config,"EjecucionDeAlgoritmoDeDeteccionDeInterbloqueo")* 1000000);

	log_info(logOrquestador,"Proceso plataforma iniciado");
	log_info(logOrquestador,"Quantum: %d Sleep: %d microsegundos",varGlobalQuantum,	varGlobalSleep);
	log_info(logOrquestador,"Creando THR Orquestador...");

	//Inicialización del thread orquestador
	pthread_t thr_orquestador;
	pthread_create(&thr_orquestador, NULL, orquestador, NULL);
	log_info(logOrquestador,"THR Orquestador creado correctamente");

	//TODO: Revisar cuando se espera y cierra el orquestador
	//pthread_join(thr_orquestador, NULL); //Esperamos que termine el thread orquestador
	//pthread_detach(thr_orquestador);

	//********************* INICIO INOTIFY *********************
	//Descripción del segmento:
	//INOTIFY es un un subsistema del kernel de linux que nos informa de cambios
	//que hayan tenido lugar en el filesystem. En la plataforma esto se utiliza
	//para recargar el archivo de configuración en caso de que haya sido modificado
	//mientras la plataforma se encuentra corriendo.

	int fd; //file_descriptor
	int wd; //watch_descriptor

	//Obtenemos la ruta del archivo de configuración de la plataforma
	char cwd[1024];
	char * fileconf="/config.txt";
	if (getcwd(cwd, sizeof(cwd)) != NULL){
		log_debug(logOrquestador,"Directorio (CWD) de INOTIFY: %s", cwd);
		strcat(cwd, fileconf);
		log_debug(logOrquestador,"Archivo de INOTIFY: %s", cwd);
	}
	else {
		log_error(logOrquestador,"Error con la funcion getcwd(...) en el segmento INOTIFY de main()");
	}

	struct timeval time;
	fd_set rfds;
	int ret;

	//Creamos una instancia de inotify, devuelve un archivo descriptor
	fd = inotify_init();

	//Comprobamos que haya un descriptor válido, sino es así se inicio mal el inotify
	if ( fd < 0 ) {
		log_error(logOrquestador,"Error con la funcion inotify_init(...) en el segmento INOTIFY de main()");
	}
	//Creamos un watch para el evento IN_MODIFY. Esto quiere decir que el archivo de configuración
	//será "monitoreado" para saber si es modificado, si esto sucede se disparará un evento.
	//Sintaxis: watch_descriptor = inotify_add_watch (archivoDescrpitor, path, evento)
	wd = inotify_add_watch( fd, cwd, IN_MODIFY);

	// El archivo descriptor creado por inotify, es el que recibe la información sobre los eventos ocurridos.
	// Para leer esta información el descriptor se lee como si fuera un archivo comun y corriente pero
	// la diferencia esta en que lo que leemos no es el contenido de un archivo sino la información
	// referente a los eventos ocurridos

	//Ciclo While A: Sale cuando se termina el programa (flagGlobalfin = 0)
	while(flagGlobalFin) {

		time.tv_sec = 2;
		time.tv_usec = 0;//varGlobalSleep;

		FD_ZERO(&rfds);
		FD_SET(fd,&rfds);
		ret = select (fd + 1, &rfds, NULL, NULL, &time);
		if (ret < 0){
			log_error(logOrquestador,"Error con la funcion select(...) en el segmento INOTIFY de main()");
		}
		if (!ret){
			log_error(logOrquestador,"Error de timeout en el segmento INOTIFY de main()");
		}
		if (FD_ISSET (fd, &rfds)){

			char buffer[EVENT_BUF_LEN];
			//Leemos el archivo FD que contiene los eventos y los guardamos en un buffer
			int length = read( fd, buffer, EVENT_BUF_LEN );
			//Verficamos si hubo errores en la lectura del archivo FD
			if ( length < 0 ) {
				log_error(logOrquestador,"Error al leer del archivo FD en el segmento INOTIFY de main()");
			}

			//A continuación recorremos el buffer para revisar todos los eventos que pudieron haber sucedido
			int i = 0;
			//Ciclo While B: Mientras haya evento en el buffer
			while ( i < length ) {
				//Guardamos en event el evento del buffer
				struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];

				//NOTA: Si "watcheamos" (monitoreamos) un archivo no va a venir el nombre porque es redundante por eso event->len es igual a 0
				//Logeamos el evento
				log_debug(logOrquestador,"Evento INOTIFY: wd=%d mask=%u cookie=%u len=%u",
							event->wd, event->mask,
							event->cookie, event->len);

				//Si el evento fue de tipo IN_MODIFY quiere decir que el archivo de configuración fue modificado
				//mientras la plataforma se encuentra corriendo y debemos volver a cargar la configuración
				if ( event->mask & IN_MODIFY) {
					config_destroy(config);
					config = config_create("config.txt");
					if(config_has_property(config,"Quantum")){
						varGlobalQuantum=config_get_int_value(config,"Quantum");
						log_info(logOrquestador,"El archivo de configuracion fue modificado");
						log_info(logOrquestador,"Nuevo valor del Quantum: %d",varGlobalQuantum);
					}
				}

				buffer[EVENT_BUF_LEN]="";
				i += EVENT_SIZE + event->len;

			} //Ciera el ciclo While B
		} //Cierra If
	} //Cierra el cilco While A (el que comprueba la finalización del programa)

	//A partir de este punto se supone que el programa esta finalizandose
	//porque flagGlobalfin es igual a 0 al salir del ciclo anterior

	//Removemos el watch que habíamos aplicado al archivo
	inotify_rm_watch(fd,wd);

	//Se cierra la instancia de inotify
	close(fd);
	//******************** FIN INOTIFY *********************

	config_destroy(config);
	log_info(logOrquestador,"Proceso plataforma finalizado correctamente\n");

	//******************** KOOPA *********************
	//Antes de cerrar el proceso plataforma ejecutamos lo último que nos pide el TP:
	//ejectura el proceso koopa (se supone que para este momento todos los pjs terminaron
	//su plan de niveles)
	//todo hay que ver donde metemos a koopa para que esto sea consistente, yo diria meterlo en la misma carpeta q el planificador

	//Se crea un array con los argumentos que deberá recibir la función main(...) de koopa
	//El array debe terminar en NULL para indicar el fin del mismo
	char *environ[]= {"../../Libs","reglas.txt",NULL};
	//Ejecutamos koopa (en un nuevo proceso)
	execv("koopa",environ);

	//Finaliza la función main() del proceso plataforma
	return EXIT_SUCCESS;
}
//FIN DE MAIN


//PLANIFICADOR:
//Función que será ejecutada a través de un hilo para atender la planificación de cada nivel
//Cardinalidad = 0 hasta N threads ejecutándose simultáneamente, uno por nivel
int planificador (InfoNivel* nivel) {

	//Cola de personajes listos y bloqueados
	t_queue *colaListos=queue_create();
	t_queue *colaBloqueados=queue_create();
	nivel->nodoN->colaBloqueados=colaBloqueados;
	nivel->nodoN->colaListos=colaListos;

	//TODO implementar handshake Conectarse con el nivel
	//int socketNivel=quieroUnPutoSocketAndando(nivel->ipN,nivel->portN);

	log_info(logPlanificador,"IP: %s // Puerto: %d",nivel->ip,nivel->port);

	//Configuramos el planificador
	void* nodoAux;
	InfoPlanificador *planificadorActual;
	planificadorActual=malloc(sizeof(InfoPlanificador));
	planificadorActual->colaListos=colaListos;
	planificadorActual->colaBloqueados=colaBloqueados;
	planificadorActual->contPersonajes=0;
	//Pasar puerto especificado por el nivel al planificador
	planificadorActual->port=nivel->puertoPlanif;

	//Se necesita algun char (cualquiera) para poder usar la función de enviar mensaje más adelante
	char* auxcar;
	auxcar=malloc(sizeof(char));
	*auxcar='P';

	//Hacemos que el planificador utilize el quantum definido en la variable global varGlobalQuantum
	//que depende del valor especificado en el archivo de configuración
	int quantum = varGlobalQuantum;

	/*Esta var auxiliar lo que hace es que si no se pudo
	 * mandar el movimiento permitido al personaje,
	 * en el if para recibir la respuesta automaticamente
	 * ejecute la desconexion del Personaje
	 */
	int varAuxiliar=0;

	fd_set descriptoresLectura;	/* Descriptores de interes para select() */
    //Creamos un socket de escucha
	int socketEscucha;
    if((socketEscucha=quieroUnPutoSocketDeEscucha(planificadorActual->port)) != 1)
    	log_info(logPlanificador,"Escuchando Conexiones en el puerto: %d",planificadorActual->port);
    else
    	log_error(logPlanificador,"No se pudo crear el socket de escucha en el puerto: %d",planificadorActual->port);

    //Ciclo While A: permanente que atenderá las solicitudes de los personajes para el nivel asociado a este planificador
	while(1){

		/*todo Handleo desconexion de Nivel: no me gusta como esta hecho pero es
		 * la unica solucion que se me ocurrio en este momento.
		 * La idea es que esta variable global sea una flag que se active
		 * cuando un nivel se cierra y funciona barbaro para romper
		 * el while y que termine el thread, el problema es cuando
		 * se desconecten varios niveles al mismo tiempo,
		 * quizas cierra alguno que no va*/

		//nombreNivel es una variable global, es decir que puede ser accedida directamente por la función esMiNivel más adelante
		nombreNivel = nivel->nombre;

		//esMiNivel es una función booleana que devuelve un true si la variable global "nombreNivel" es igual a
		//el nombre de un nivel que se le pasa como argumento

		//Si en la lista global de niveles que se encuentran conectados a la plataforma no se encuentra el de este planificador
		if(!list_find(listaNiveles, esMiNivel)){

			int i;
			NodoPersonaje * auxPersDesc;
			char mj= 'K'; //Es necesario mandar algo en el mensaje, puede ser cualquier cosa porque lo importante en este caso es el header

			//Desconectar sockets de cola de listos para que terminen los PJ's
			//Ciclo mientras la cola de listos contenga elementos
			while(!queue_is_empty(colaListos)){
				auxPersDesc = queue_pop(colaListos);
				//Enviamos un mensaje de desconexión al personaje
				//NOTA: Para el personaje un header = 10 equivale a desconexión por nivel
				mandarMensaje(auxPersDesc->socket, 10, sizeof(char), &mj);
				close(auxPersDesc->socket);
			}

			//Desconectar sockets de cola de bloqueados para que terminen los PJ's
			//Ciclo mientras la cola de bloqueados contenga elementos
			while(!queue_is_empty(colaBloqueados)){
				auxPersDesc = queue_pop(colaBloqueados);
				//Enviamos un mensaje de desconexión al personaje
				//NOTA: Para el personaje un header = 10 equivale a desconexión por nivel
				mandarMensaje(auxPersDesc->socket, 10, sizeof(char), &mj);
				close(auxPersDesc->socket);
			}

			log_info(logPlanificador,"El nivel de este planificador ya no se encuentra en la lista de niveles de la plataforma. Todos los personajes desconectados, se rompe el while(1) y se cierra el thread.");
			break; //Esto termina el ciclo permanente A ( el While(1) )
		}

		//A partir de este momento se realiza la atención de nuevas conexiones a través de un select()
		//El uso del select() reemplazó al thread de listenerDePersonajes()

		/* Se inicializa descriptoresLectura */
		FD_ZERO (&descriptoresLectura);
		FD_SET (socketEscucha, &descriptoresLectura);/* Se añade para select() el socket de escucha del servidor */
		/* Espera indefinida hasta que alguno de los descriptores tenga algo
		 * que decir: un nuevo cliente  */
		struct timeval timeout;
		timeout.tv_sec=0;
		timeout.tv_usec=0;
		//Select al estar configurado con un timeout de 0 segundos (y microsegundos), espera hasta que le llegue algo (bloqueante)
		select (socketEscucha + 1, &descriptoresLectura, NULL, NULL, &timeout);
		/* Se comprueba si algún cliente nuevo desea conectarse y se le
		 * admite. FD_ISSET nos dice si hubo cambios en ese descriptor */
		pthread_mutex_lock(&nivel->nodoN->sem);
			if (FD_ISSET (socketEscucha, &descriptoresLectura)){
				//Dentro de la función listenerPersonaje el nuevo personaje se agrega a la cola de listos y ,además, si
				//estaba en la cola de bloqueados se lo quita de la misma
				listenerPersonaje(planificadorActual,socketEscucha);
			}
		pthread_mutex_unlock(&nivel->nodoN->sem);

		//A partir de este momento se realiza la atención de los personajes que ya estaban conectados al planificador
		//enviándole el mensaje de movimiento permitido al personaje que corresponda según el quantum

		//Si la cola de personajes listos no esta vacia
		if(!queue_is_empty(colaListos)){
			//pthread_mutex_lock(&nivel->nodoN->sem);

			//Imprimir PJ listos
			log_info(logPlanificador,"*****Personajes en la cola de Listos*****");
			int p;
			//Este mutex evita que se este cargando un nuevo personaje a través de un listenerPersonaje mientras se procesa
			//a los que ya se encuentran conectados al planificador
			pthread_mutex_lock(&nivel->nodoN->sem);
				int tam=queue_size(colaListos);
				for (p=0;p<tam;p++){
					//Agarramos el primer personaje de la cola de listos
					NodoPersonaje* nodoP=queue_pop(colaListos);
					//Lo logeamos
					log_info(logPlanificador,"%d° Personaje: %c Socket: %d",p+1,nodoP->simboloRepresentativo,nodoP->socket);
					//Lo volvemos a poner al final de la cola
					queue_push(colaListos,nodoP);
				}
			pthread_mutex_unlock(&nivel->nodoN->sem);

			//Imprimir PJ bloqueados
			//Este mutex cumple la misma función que cuando se procesa la cola de listos
			pthread_mutex_lock(&nivel->nodoN->sem);
				log_info(logPlanificador,"*****Personajes en la cola de Bloqueados*****");
				tam=queue_size(colaBloqueados);
				for (p=0;p<tam;p++){
					//Agarramos el primer personaje de la cola de bloqueados
					NodoPersonaje* nodoP=queue_pop(colaBloqueados);
					//Lo logeamos
					log_info(logPlanificador,"%d° Personaje: %c Recurso Solicitado: %c Socket: %d",p+1,nodoP->simboloRepresentativo,nodoP->recursoPedido,nodoP->socket);
					//Lo volemos a poner al final de la cola
					queue_push(colaBloqueados,nodoP);
				}
			pthread_mutex_unlock(&nivel->nodoN->sem);

			NodoPersonaje *personajeActual; //todo Revisar que los punteros de personajeActual anden bien

			//Quantum > 0, es decir le queda quantum al personaje que esta siendo atendido
			if(quantum>0){

				log_info(logPlanificador,"El Quantum %d es mayor a 0",quantum);
				//Mandar mensaje de movimiento permitido al socket del personaje del nodo actual (primer nodo de la cola)
				pthread_mutex_lock(&nivel->nodoN->sem);
					personajeActual = (NodoPersonaje*) queue_peek(colaListos);
					if(mandarMensaje(personajeActual->socket, 8, sizeof(char), auxcar) > 0){
						log_info(logPlanificador,"Se mando Mov permitido al personaje %c",personajeActual->simboloRepresentativo);
						varAuxiliar=0;
					}
					else {
						log_error(logPlanificador,"No se pudo enviar un Mov permitido al personaje %c",personajeActual->simboloRepresentativo);
						varAuxiliar=1;
					}
				pthread_mutex_unlock(&nivel->nodoN->sem);

			}
			//En caso de que no le quede quantum al personaje que estaba siendo atendido
			else {
				//Restauramos el valor del quantum
				quantum=varGlobalQuantum;

				log_info(logPlanificador,"Se termino el quantum");
				//Sacar el nodo actual (primer nodo de la cola) y enviarlo al fondo de la misma
				pthread_mutex_lock(&nivel->nodoN->sem);
					NodoPersonaje* perAux = queue_pop(colaListos);

					log_info(logPlanificador,"Se saco al pj %c de la cola de listos",perAux->simboloRepresentativo);
					queue_push(colaListos,perAux);

					log_info(logPlanificador,"Se puso al pj %c al final de la cola de listos",perAux->simboloRepresentativo);
					//Buscar el primero de la cola de listos y mandarle un mensaje de movimiento permitido
					personajeActual = (NodoPersonaje*) queue_peek(colaListos);
				pthread_mutex_unlock(&nivel->nodoN->sem);

				log_info(logPlanificador,"El primer personaje de la cola es: %c",personajeActual->simboloRepresentativo);
				if(mandarMensaje(personajeActual->socket, 8, sizeof(char), auxcar) > 0){
					log_info(logPlanificador,"Se mando Mov permitido al personaje %c",personajeActual->simboloRepresentativo);
					varAuxiliar=0;
				}
				else {
					log_error(logPlanificador,"No se pudo enviar Mov permitido al personaje %c",personajeActual->simboloRepresentativo);
					varAuxiliar=1;
				}
			}


			//A partir de este momento se realiza la atención al mensaje que nos está enviándo el personaje que tiene el turno

			//Esperar respuesta de turno terminado (con la info sobre si quedo bloqueado y si tomo recurso)
			Header headerMsjPersonaje;
			MensajePersonaje msjPersonaje;
			log_info(logPlanificador,"Esperando respuesta de turno concluido");

			//Si no hubo errores al envíar el mensaje de movimiento permitdo a un personaje y el mismo respondió con algo
			if(varAuxiliar == 0 && recibirHeader(personajeActual->socket, &headerMsjPersonaje) > 0){

				//Recibir data nos permite acceder al contenido del mensaje que nos envió el personaje
				if(recibirData(personajeActual->socket, headerMsjPersonaje, (void**) &msjPersonaje)){

					log_info(logPlanificador,"Respuesta recibida");
					log_debug(logPlanificador,"Solicita Recurso: %d Bloqueado: %d FinNivel: %d Recurso: %c",msjPersonaje.solicitaRecurso,msjPersonaje.bloqueado,msjPersonaje.finNivel,msjPersonaje.recursoSolicitado);

					//***Comportamientos según el mensaje que se recibe del personaje***

					//Si solicita recurso y SI quedo bloqueado {quantum=varGlogalQuantum; poner al final de la cola de bloquedados}
					//Solicita SI, Bloqueado SI
					if(msjPersonaje.solicitaRecurso & msjPersonaje.bloqueado){
						log_debug(logPlanificador,"Per: %c Rec %c bloq1 %d",personajeActual->simboloRepresentativo,msjPersonaje.recursoSolicitado,quantum);
						personajeActual->recursoPedido = msjPersonaje.recursoSolicitado;
						//Se restaura el quantum
						quantum = varGlobalQuantum + 1;
						pthread_mutex_lock(&nivel->nodoN->sem);
						 	//Sacamos al personaje de la cola de listos y lo ponemos al final de la cola de bloqueados
							queue_push(colaBloqueados,queue_pop(colaListos));
						pthread_mutex_unlock(&nivel->nodoN->sem);
						log_debug(logPlanificador,"Per: %c Rec %c bloq2 %d",personajeActual->simboloRepresentativo,msjPersonaje.recursoSolicitado,quantum);
					}

					//En este caso o no solicita un recurso o no quedo bloqueado
					else {

						//SI solicita recurso y NO quedo bloqueado {quantum=varGlogalQuantum + 1; poner al final de la cola}
						//Solicita SI, Bloqueado NO
						if(msjPersonaje.solicitaRecurso & !msjPersonaje.bloqueado){

							//Si el movimiento realizado por el personaje era el último que necesitaba para terminar el nivel
							//Solicita SI, Bloqueado NO, Fin SI
							if(msjPersonaje.finNivel){
								log_info(logPlanificador,"El personaje %c termino el Nivel",personajeActual->simboloRepresentativo);
								pthread_mutex_lock(&nivel->nodoN->sem);
									//Como termino lo sacamos de la cola de listos
									queue_pop(colaListos);
								pthread_mutex_unlock(&nivel->nodoN->sem);
								msjPersonaje.solicitaRecurso = 0;
								msjPersonaje.bloqueado = 0;
								//Se restaura el quantum
								quantum = varGlobalQuantum + 1;
								//TODO: Acá no habría que cerrar la conexión con el personaje con un close(personajeActual->socket); ?
								log_info(logPlanificador,"El personaje %c fue retirado de la cola",personajeActual->simboloRepresentativo);
							}

							//Si el movimiento realizado por el personaje NO era el último para terminar el nivel
							//Solicita SI, Bloqueado NO, Fin NO
							else{
								log_info(logPlanificador,"Rec no bloq1 %d",quantum);
								//Se restaura el quantum
								quantum = varGlobalQuantum + 1;
								pthread_mutex_lock(&nivel->nodoN->sem);
									//Como se le otorga un recurso, debe pasar al final de la cola de listos
									queue_push(colaListos,queue_pop(colaListos));
								pthread_mutex_unlock(&nivel->nodoN->sem);
								log_info(logPlanificador,"Se puso el Personaje %c al final de la cola luego de asignarle el recurso %c", personajeActual->simboloRepresentativo,msjPersonaje.recursoSolicitado);
							}
						}

						//En caso de que NO solicita recurso
						//Solicita NO, Bloqueado NO
						else{

							//Si el personaje termina el nivel sin tener que haber pedido un recurso en el úlitmo moviemiento
							if(msjPersonaje.finNivel){
								log_info(logPlanificador,"El personaje %c termino el Nivel",personajeActual->simboloRepresentativo);
								pthread_mutex_lock(&nivel->nodoN->sem);
									//Como termino lo sacamos de la cola de listos
									queue_pop(colaListos);
								pthread_mutex_unlock(&nivel->nodoN->sem);
								msjPersonaje.solicitaRecurso=0;
								msjPersonaje.bloqueado=0;
								//Se restaura el quantum
								quantum = varGlobalQuantum + 1;
								//Terminamos la conexión con el personaje
								close(personajeActual->socket);
								log_info(logPlanificador,"El personaje %c fue retirado de la cola",personajeActual->simboloRepresentativo);
							}

						}
					}

					quantum--;
					log_debug(logPlanificador,"Quatum Left: %d Sleep: %d",quantum,varGlobalSleep);
					usleep(varGlobalSleep);
				}

				//En caso de que haya habido algún error al recibir los datos o aparentemente se haya desconectado el personaje
				else {
					//Se retira al PJ de la cola de Listos
					desconexionPersonaje(personajeActual->socket, msjPersonaje, quantum, colaListos);
					log_error(logPlanificador,"No llego la data del PJ: %c",personajeActual->simboloRepresentativo);
				}
			}

			//En caso de que haya habido algún error al recibir el header o aparentemente se haya desconectado el personaje
			else {
				//Se retira al PJ de la cola de Listos
				desconexionPersonaje(personajeActual->socket, msjPersonaje, quantum, colaListos);
				log_error(logPlanificador,"No llego el header del PJ: %c",personajeActual->simboloRepresentativo);
			}

		}
		else{
			log_debug(logPlanificador,"Cola de listos vacia --> Sleep");
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
				simboloRecibido=malloc(sizeof(char));
				Header unHeader;
				if(recibirHeader(nodoN->socket,&unHeader)>0){
					log_info(logOrquestador,"Header recibido del socket: %d tipo: %d payload: %d",socketNuevaConexion,unHeader.type, unHeader.payloadlength);
					NodoRecurso *nodoR;
					NodoRecurso * recursosAsignados1;
					char * pjDeadlock;
					switch(unHeader.type){

						case 3:
						pthread_mutex_lock(&nodoN->sem);
						{//todo borrar imprimir PJ listos
						log_debug(logOrquestador,"*****Personajes en la Cola de Listos*****");
						int p;
						int tam=queue_size(nodoN->colaListos);
						for (p=0;p<tam;p++){
							NodoPersonaje* nodoP=list_get(nodoN->colaListos->elements,p);
							log_debug(logOrquestador,"%d ° Personaje: %c",p+1,nodoP->simboloRepresentativo);

						}
						//imprimir PJ bloqueados
						log_debug(logOrquestador,"*****Personajes en la Cola de Bloqueados*****");
						tam=queue_size(nodoN->colaBloqueados);
						for (p=0;p<tam;p++){
							NodoPersonaje* nodoP=list_get(nodoN->colaBloqueados->elements,p);
							log_debug(logOrquestador,"%d ° Personaje: %c Recurso Solicitado: %c",p+1,nodoP->simboloRepresentativo,nodoP->recursoPedido);

						}}


							log_info(logOrquestador,"Resolver deadlock");
							pjDeadlock=malloc(unHeader.payloadlength);
							recibirData(nodoN->socket,unHeader,(void**)pjDeadlock);
							int r2,minimo2,p2,tamColaBloq2;
							NodoPersonaje* nodoPj;
							minimo2=9999;//valor absurdo
							tamColaBloq2=queue_size(nodoN->colaBloqueados);
							for(r2=0;r2<(unHeader.payloadlength);r2++){
								for(p2=0;p2<tamColaBloq2;p2++){
									nodoPj=queue_pop(nodoN->colaBloqueados);
									if((pjDeadlock[r2]==nodoPj->simboloRepresentativo)&&(minimo2>nodoPj->orden))
										minimo2=nodoPj->orden;
									queue_push(nodoN->colaBloqueados,nodoPj);
								}
								log_debug(logOrquestador,"%d° Personaje en deadlock recibido: %c",r2+1,pjDeadlock[r2]);
							}
							for(p2=0;p2<tamColaBloq2;p2++){
								nodoPj=queue_pop(nodoN->colaBloqueados);
								if(minimo2==nodoPj->orden){
									mandarMensaje(nodoPj->socket,9,sizeof(char),&(nodoPj->simboloRepresentativo));
									log_info(logOrquestador,"Se mato al personaje %c y se procede a liberar sus preciosos recursos",nodoPj->simboloRepresentativo);
									//							free(nodoPj);
								}
								else
									queue_push(nodoN->colaBloqueados,nodoPj);
							}


							{//todo borrar imprimir PJ listos
							log_debug(logOrquestador,"*****Personajes en la Cola de Listos*****");
							int p;
							int tam=queue_size(nodoN->colaListos);
							for (p=0;p<tam;p++){
								NodoPersonaje* nodoP=list_get(nodoN->colaListos->elements,p);
								log_debug(logOrquestador,"%d ° Personaje: %c",p+1,nodoP->simboloRepresentativo);

							}
							//imprimir PJ bloqueados
							log_debug(logOrquestador,"*****Personajes en la Cola de Bloqueados*****");
							tam=queue_size(nodoN->colaBloqueados);
							for (p=0;p<tam;p++){
								NodoPersonaje* nodoP=list_get(nodoN->colaBloqueados->elements,p);
								log_debug(logOrquestador,"%d ° Personaje: %c Recurso Solicitado: %c",p+1,nodoP->simboloRepresentativo,nodoP->recursoPedido);

							}}



							pthread_mutex_unlock(&nodoN->sem);
							break;
						case 4://el nivel envia los recursos liberados
						pthread_mutex_lock(&nodoN->sem);

						{//todo borrar imprimir PJ listos
						log_debug(logOrquestador,"*****Personajes en la Cola de Listos*****");
						int p;
						int tam=queue_size(nodoN->colaListos);
						for (p=0;p<tam;p++){
							NodoPersonaje* nodoP=list_get(nodoN->colaListos->elements,p);
							log_debug(logOrquestador,"%d ° Personaje: %c",p+1,nodoP->simboloRepresentativo);

						}
						//imprimir PJ bloqueados
						log_debug(logOrquestador,"*****Personajes en la Cola de Bloqueados*****");
						tam=queue_size(nodoN->colaBloqueados);
						for (p=0;p<tam;p++){
							NodoPersonaje* nodoP=list_get(nodoN->colaBloqueados->elements,p);
							log_debug(logOrquestador,"%d ° Personaje: %c Recurso Solicitado: %c",p+1,nodoP->simboloRepresentativo,nodoP->recursoPedido);

						}}



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
							tamColaBloq=queue_size(nodoN->colaBloqueados);
							for (p=0;p<tamColaBloq;p++){
							//for each recursoRecibido{
								NodoPersonaje* nodoP=queue_pop(nodoN->colaBloqueados);
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
										queue_push(nodoN->colaBloqueados,nodoP);
								}
							}
							{//todo borrar imprimir PJ listos
								log_debug(logOrquestador,"*****Personajes en la Cola de Listos*****");
								int p;
								int tam=queue_size(nodoN->colaListos);
								for (p=0;p<tam;p++){
									NodoPersonaje* nodoP=list_get(nodoN->colaListos->elements,p);
									log_debug(logOrquestador,"%d ° Personaje: %c",p+1,nodoP->simboloRepresentativo);

								}
								//imprimir PJ bloqueados
								log_debug(logOrquestador,"*****Personajes en la Cola de Bloqueados*****");
								tam=queue_size(nodoN->colaBloqueados);
								for (p=0;p<tam;p++){
									NodoPersonaje* nodoP=list_get(nodoN->colaBloqueados->elements,p);
									log_debug(logOrquestador,"%d ° Personaje: %c Recurso Solicitado: %c",p+1,nodoP->simboloRepresentativo,nodoP->recursoPedido);

								}}

							//mandar recAsignados
							log_info(logOrquestador,"Se envio los recursos Asignados");
							mandarMensaje(nodoN->socket,4,unHeader.payloadlength,(void*)recursosAsignados1);
							free(nodoR);
							free(recursosAsignados1);
							pthread_mutex_unlock(&nodoN->sem);
							break;
						default:
							break;
						}
//					close(socketNuevaConexion);WTF lucas?
				}
				/* Si se desconecta el nivel correspondiente
				 * (el socket se desconecta)
				es eliminado de la lista de niveles */
				else {
					nombreNivel = nodoN->nombreNivel;
					list_remove_by_condition(listaNiveles, esMiNivel);
					log_info(logOrquestador,"Se borro el nivel %s por desconexion",nodoN->nombreNivel);
					close(nodoN->socket);
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
			simboloRecibido=malloc(sizeof(char));
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
					simboloRecibido=malloc(sizeof(char));

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
						case 3: case 4:	case 6: case 7://case 3: el personaje se muere por sigterm y reinicia el nivelActual
									//case 4:el personaje pierde todas las vidas y reinicia el plan de niveles

							log_info(logOrquestador,"Esperando solicitud de nivel...");
							nivelDelPersonaje=malloc(unHeader.payloadlength);
							if(recibirData(socketNuevaConexion,unHeader,(void**)nivelDelPersonaje)){
								if(unHeader.type==3) log_info(logOrquestador,"El Personaje %c murio por la señal SIGTERM y reinicia el nivel: %s",*simboloRecibido,nivelDelPersonaje);
								else if(unHeader.type==4)log_info(logOrquestador,"El Personaje %c perdio todas sus vidas y debe reiniciar su plan de niveles desde: %s",*simboloRecibido,nivelDelPersonaje);
								else if(unHeader.type==6)log_info(logOrquestador,"El Personaje %c fue asesinado descaradamente por el orquestador y solicita volver al nivel: %s",*simboloRecibido,nivelDelPersonaje);
								else log_info(logOrquestador,"El Personaje %c informa que el nivel se cerro inesperadamente y vuelve a solicitar la informacion del nivel: %s",*simboloRecibido,nivelDelPersonaje);
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
									if(unHeader.type==3) {
										int p;
										int tam=queue_size(nivel->colaBloqueados);
										for (p=0;p<tam;p++){
											NodoPersonaje* nodoP=list_get(nivel->colaBloqueados->elements,p);
											if(nodoP->simboloRepresentativo==*simboloRecibido){
												list_remove(nivel->colaBloqueados->elements,p);
												tam--;
												p--;
												log_debug(logOrquestador,"Se depuro la cola de bloqueados del personaje salido");
											}
										}
									}
								}
								//saca al personaje de la cola de bloqueados si se murio por sigInt


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
					pthread_mutex_init(&nodoNivel->sem, NULL);
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
//Función que escucha nuevas conexiones de personajes
int listenerPersonaje(InfoPlanificador* planificador, int socketEscucha){

		//IMPLEMENTACIÓN DENTRO DEL CÓDIGO DEL TP SIN USAR SELECT()
		//  Este thread se encargará de escuchar nuevas conexiones de personajes indefinidamente (ver función listenerPersonaje)
		//	pthread_t threadPersonaje;
		//	pthread_create(&threadPersonaje, NULL, listenerPersonaje, (void *)planificadorActual);
		//	log_info(logPlanificador,"THR de escucha del planificador de puerto %d creado correctamente",nivel->port);

	    int socketNuevaConexion;


	    //Usar el siguiente ciclo indefinido si se va a utilizar la función en un thread (es decir, sin usar select())
	    //Ciclo While(1) para escuchar nuevos personajes indefinidamente
	    //while (1){

			// Escuchar nuevas conexiones entrantes.
			if (listen(socketEscucha, 1) != 0) {
				log_error(logPlanificador,"Error al bindear socket escuchar");
				return EXIT_FAILURE;
			}
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
				if (mandarMensaje(socketNuevaConexion,0 , sizeof(char),simboloRecibido)) {
					//log_info(logger,"Mando mensaje al personaje %c",*simboloRecibido);
					log_info(logPlanificador,"Handshake respondido al Personaje %c", *simboloRecibido);
				}
				int i,tam;
				tam=queue_size(planificador->colaBloqueados);
				for(i=0;i<tam;i++){
					NodoPersonaje* auxNodoP= list_get(planificador->colaBloqueados->elements,i);
					if(auxNodoP->simboloRepresentativo==*simboloRecibido){
						list_remove(planificador->colaBloqueados->elements,i);
						i--;
						tam--;
					}
				}
				//Agrega el nuevo personaje a la cola de listos del planificador
				NodoPersonaje* personaje;
				personaje=malloc(sizeof(NodoPersonaje));
				personaje->simboloRepresentativo=*simboloRecibido; //Ej: @ ! / % $ &
				personaje->socket=socketNuevaConexion;
				personaje->orden=planificador->contPersonajes;
				queue_push(planificador->colaListos,personaje);
				planificador->contPersonajes++;
				log_info(logPlanificador,"Se agrego al Personaje %c Socket: %d a la cola de listos",*simboloRecibido,socketNuevaConexion);



			}

		//}Cierra el while(1)

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
	NodoPersonaje* auxP = queue_pop(colaListos);
	log_info(logOrquestador,"Se desconecto imprevistamente el Pj: %c socket:%d",auxP->simboloRepresentativo,socketDesconectar);
	msjPersonaje.solicitaRecurso = 0;
	msjPersonaje.bloqueado = 0;
	quantum =varGlobalQuantum + 1;
	g_contPersonajes--;
	//todo: porque no cerramos la conexión?
//	close(socketDesconectar);
}
//----------------------------------------------------------------
