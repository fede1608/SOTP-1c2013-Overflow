/*
 * personaje.c
 *
 *  Created on: 29/04/2013
 *      Author: utnso
 */

#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include "config.h"
#include "nivel.h"
static int rows,cols;

int main(void){
	t_config* config=config_create("config.txt");

	nivel_gui_inicializar();

	nivel_gui_get_area_nivel(&rows, &cols);

//	printf("master");

	ITEM_NIVEL * pj1=malloc(sizeof(ITEM_NIVEL));
	pj1->id=config_get_string_value(config,"simbolo")[0];
	pj1->item_type=PERSONAJE_ITEM_TYPE;
	pj1->posx=4;
	pj1->posy=4;
	pj1->quantity=1;
	pj1->next=malloc(sizeof(ITEM_NIVEL));
	ITEM_NIVEL * pj2=pj1->next;
	pj2->id='&';
		pj2->item_type=RECURSO_ITEM_TYPE;
		pj2->posx=7;
		pj2->posy=7;
		pj2->quantity=7;
		pj2->next=NULL;


		while(1){
		nivel_gui_dibujar(pj1);
		//sleep(1);
		}


nivel_gui_terminar();
return 0;
}


