/*
 * personaje.c
 *
 *  Created on: 29/04/2013
 *      Author: utnso
 */
#include "config.h"
#include "nivel.h"
#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <sys/ioctl.h>



static int rows,cols;

int main(void){
	t_config* config=config_create("config.txt");

	nivel_gui_inicializar();

	nivel_gui_get_area_nivel(&rows, &cols);

//	Conectar con el personaje

	ITEM_NIVEL * pj1=malloc(sizeof(ITEM_NIVEL));
	pj1->id=config_get_string_value(config,"simbolo")[0];
	pj1->item_type=PERSONAJE_ITEM_TYPE;
	pj1->posx=12;
	pj1->posy=12;
	pj1->quantity=1;
	pj1->next=malloc(sizeof(ITEM_NIVEL));
	ITEM_NIVEL * flor=pj1->next;
	flor->id='F';
		flor->item_type=RECURSO_ITEM_TYPE;
		flor->posx=10;
		flor->posy=14;
		flor->quantity=7;
		flor->next=malloc(sizeof(ITEM_NIVEL));

	ITEM_NIVEL * hongo=flor->next;
		hongo->id='H';
		hongo->item_type=RECURSO_ITEM_TYPE;
		hongo->posx=17;
		hongo->posy=6;
		hongo->quantity=2;
		hongo->next=NULL;

		char** obj;
		int now=0;
		obj=config_get_array_value(config,"obj[Nivel1]");
		ITEM_NIVEL * elemento=flor;

		while(1){



			//if ((char)*obj[now]==elemento->id) { //strcmp devuelve 0 si es true por eso hay q negarlo, vaya uno a saber porque
				if (pj1->posx!=elemento->posx) {
					(elemento->posx-pj1->posx)>0?pj1->posx++:pj1->posx--;
				}else if (pj1->posy!=elemento->posy) {
					(elemento->posy-pj1->posy)>0?pj1->posy++:pj1->posy--;
				}else {
//					if (elemento->next==NULL) {return 1;}
//					else{elemento=elemento->next;}
				now++;

				int i;
				printf("TEST ");
				elemento=flor;
					for (i=0;i<2;i++){
						printf("%i %c %c ",now,(char)*obj[now],elemento->id);

						if ((char)*obj[now]==elemento->id){
							i=2;
						}else elemento=elemento->next;
					}printf("%i %c %c %i %i ",now,(char)*obj[now],elemento->id,elemento->posx,elemento->posy);
				}

			//}else now++;
			nivel_gui_dibujar(pj1);
		sleep(1);
		//if (now==4) break;
		}

		sleep(3);
nivel_gui_terminar();
return 0;
}


