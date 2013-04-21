#include <stdbool.h>
#include "libmemoria.h"
#include <stdlib.h>
#include <string.h>
//int cont = 0;
//char *array1[10];
//t_list* array2;
t_list * lista = list_create();
void *mem = 0;
int tam = 0;
t_memoria crear_memoria(int tamanio) {

	//array1[cont] = (char *)realloc(array1[cont],sizeof()*cont);
	//array2 = (t_list *)realloc(array2,sizeof(t_list*)*cont);
	//cont +=1;

	tam = tamanio;
	mem = malloc(tamanio);
	particion *vacio = malloc(sizeof(particion));
	vacio->id = '0';
	vacio->inicio = (int)mem;
	vacio->libre = true;
	vacio->tamanio = tamanio;
	vacio->dato = NULL;

	list_add_in_index(lista,vacio->id,vacio);
	return "lista";
}

int almacenar_particion(t_memoria segmento, char id, int tamanio, char* contenido) {
	if (tamanio>tam) return -1;
	if (strcmp(segmento,"lista"))
		{	int i;
			for (i=0;i<lista->elements_count;i++) {
				particion *part = list_get(lista,i);
				if (part->id==id) return -1;
				if (part->libre==true) {
					if (part->tamanio<=tamanio)
					{
						particion *nuevaP = malloc(sizeof(particion));
						nuevaP->id = id;
						nuevaP->inicio = part->inicio;
						nuevaP->libre = false;
						nuevaP->tamanio = tamanio;
						nuevaP->dato = contenido;
						list_add_in_index(lista,i,nuevaP);
						int resto = part->tamanio -tamanio;
						if (resto > 0) {
							//si sobra me moria se crea una particion free
							particion *memresto = malloc(sizeof(particion));
							memresto->id = '0';
							memresto->inicio = part->inicio + tamanio;
							memresto->libre = true;
							memresto->tamanio = resto;
							memresto->dato = NULL;
							list_add_in_index(lista,i+1,memresto);
							list_remove(lista,i+2);
						}else list_remove(lista,i+1);
						return 1;
					}



				}
			} return 0; //no encontro espacios vacios
		}

	return -1;
}

int eliminar_particion(t_memoria segmento, char id) {
	if (strcmp(segmento,"lista"))
	{
		int i;
		for (i=0;i<lista->elements_count;i++) {
			particion *part = list_get(lista,i);
			if (part->id==id) {
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
				}
				return 1;
			}
		}
	}
	return 0;
}

void liberar_memoria(t_memoria segmento) {
	if (strcmp(segmento,"lista")) free(mem);
}

t_list* particiones(t_memoria segmento) {

	if (strcmp(segmento,"lista")) return lista;

	//if false va a retornar por aca
	return NULL;
}
