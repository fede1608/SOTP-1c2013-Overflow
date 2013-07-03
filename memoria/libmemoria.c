#include <stdio.h>
#include <stdbool.h>
#include "libmemoria.h"
#include <stdlib.h>
#include <string.h>

t_list * lista;
void *memory;
int tam;


t_memoria crear_memoria(int tamanio) {
	lista = list_create();
	tam = tamanio;
	t_memoria mem = malloc(tamanio);
	memory=mem;
	particion *vacio = malloc(sizeof(particion));
	vacio->id = '0';
	vacio->inicio =0;
	vacio->libre = true;
	vacio->tamanio = tamanio;
	vacio->dato=mem;
	list_add(lista,vacio);
	return mem;
	}

int almacenar_particion(t_memoria segmento, char id, int tamanio, char* contenido) {

	if (tamanio>tam) return -1;
		int i;
		int partmax1;
		int memmax=0;
			for (i=0;i<lista->elements_count;i++) {

				particion *part = list_get(lista,i);

				if ((part->libre==true)&&(part->tamanio > memmax) )
				{
					memmax = part->tamanio;
					partmax1=i;
				}


			}

			particion *partmax = list_get(lista,partmax1);
					if (memmax>=tamanio)
					{
				// Agregar nueva particion
						particion *nuevaP = malloc(sizeof(particion));
						nuevaP->id = id;
						nuevaP->inicio = partmax->inicio;
						nuevaP->libre = false;
						nuevaP->tamanio = tamanio;
						nuevaP->dato=segmento+(nuevaP->inicio);
						memcpy(nuevaP->dato,contenido,tamanio);

						strcpy(nuevaP->dato,contenido);
						list_add_in_index(lista,partmax1,nuevaP);
						int resto = partmax->tamanio -tamanio;
						if (resto > 0) {
							//si sobra memoria se crea una particion libre
							particion *memresto = malloc(sizeof(particion));
							memresto->id = '0';
							memresto->inicio = partmax->inicio + tamanio;
							memresto->libre = true;
							memresto->tamanio = resto;
							memresto->dato = segmento+memresto->inicio;
							list_add_in_index(lista,partmax1+1,memresto);
							list_remove(lista,partmax1+2);
						}else list_remove(lista,partmax1+1);
						return 1;
					}
			return 0; //no encontro espacios vacios
//		}
//	return -1;
}

int eliminar_particion(t_memoria segmento, char id) {
//	if (!strcmp(segmento,"lista"))
//	{
		int i;
		int found=0;
		for (i=0;i<lista->elements_count;i++) {
			particion *part = list_get(lista,i);
			if (part->id==id) {

				//todo el laburo q hice para compactar la memoria al pedo¬¬
				/*if(i==0) //si es el primer nodo entonces preguntar solo por el nodo siguiente
				{
					if(part->tamanio==tam){//si el primer nodo es igual al tamaño total no preguntar por nadie ¬¬ I know you will check this
						part->dato=NULL;
						part->libre=true;
						part->id='0';
						printf("testEPSuccess");
						return 1;
					}

					particion *partpost = list_get(lista,i+1);
					if (partpost->libre==true) {
						part->dato=NULL;
						part->libre=true;
						part->tamanio+=partpost->tamanio;
						part->id='0';
						list_remove(lista,i+1);
						printf("testEPSuccess");
						return 1;
					}else
						{
							part->dato=NULL;
							part->libre=true;
							part->id='0';
							printf("testEPSuccess");
							return 1;
						}

				} else if (i==(lista->elements_count-1))//si es el ultimo nodo entonces preguntar por el nodo anterior
				{
					particion *partpre = list_get(lista,i-1);
					if (partpre->libre==true) {
						partpre->tamanio+=part->tamanio;
						list_remove(lista,i);
						printf("testEPSuccess");
						return 1;
					}else
					{
						part->dato=NULL;
						part->libre=true;
						part->id='0';
						printf("testEPSuccess");
						return 1;
					}
				}
				//sino es ninguna de las anteriores esta en el medio
				particion *partpre = list_get(lista,i-1);
				particion *partpost = list_get(lista,i+1);
				if (partpre->libre==true) {
					partpre->tamanio+=part->tamanio;
					if (partpost->libre==true) {
						partpre->tamanio+=partpost->tamanio;
						list_remove(lista,i+1);
						}
					list_remove(lista,i);
				} else if (partpost->libre==true) {
				part->dato=NULL;
				part->libre=true;
				part->tamanio+=partpost->tamanio;
				part->id='0';
				list_remove(lista,i+1);
				}else
				{
					part->dato=NULL;
					part->libre=true;
					part->id='0';
				}




				printf("testEPSuccess");
				return 1;*/
				part->libre=true;
				found=1;
				//3 lineas del orto, me cagoo

			}
		}
	//}
		if(!found) return 0;
		return 1;
}

void liberar_memoria(t_memoria segmento) {
	list_destroy_and_destroy_elements(lista,NULL);
	free(segmento);
}

t_list* particiones(t_memoria segmento) {
	t_list* listaaux = list_create();
	 list_add_all(listaaux,lista);
	 return lista;
	}
