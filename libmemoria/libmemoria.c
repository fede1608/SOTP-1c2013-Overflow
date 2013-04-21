#include <stdbool.h>
#include "libmemoria.h"
#include <stdlib.h>


t_memoria crear_memoria(int tamanio) {
	t_list *lista = list_create();
	void *mem = malloc(tamanio);
	particion *vacio = malloc(sizeof(particion));
	vacio->id = 0;
	vacio->inicio = mem;
	vacio->libre = true;
	vacio->tamanio = tamanio;
	list_add(lista,vacio);
	return NULL;
}

int almacenar_particion(t_memoria segmento, char id, int tamanio, char* contenido) {
	return -1;
}

int eliminar_particion(t_memoria segmento, char id) {
	return 0;
}

void liberar_memoria(t_memoria segmento) {
	
}

t_list* particiones(t_memoria segmento) {
	t_list* list = list_create();
	return list;
}
