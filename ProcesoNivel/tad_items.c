#include "tad_items.h"
#include "stdlib.h"


 ITEM_NIVEL *  CrearItem(ITEM_NIVEL** ListaItems, char id, int x , int y, char tipo, int cant_rec) {
        ITEM_NIVEL * temp;
        temp = malloc(sizeof(ITEM_NIVEL));

        temp->id = id;
        temp->posx=x;
        temp->posy=y;
        temp->item_type = tipo;
        temp->quantity = cant_rec;
        temp->next = *ListaItems;
        *ListaItems = temp;
        return temp;
}



 ITEM_NIVEL *  CrearPersonaje(ITEM_NIVEL** ListaItems, char id, int x , int y) {
	 ITEM_NIVEL * temp;
	 temp=CrearItem(ListaItems, id, x, y, PERSONAJE_ITEM_TYPE, 0);
	 return temp;
}

 ITEM_NIVEL *  CrearCaja(ITEM_NIVEL** ListaItems, char id, int x , int y, int cant) {
	ITEM_NIVEL * temp;
    CrearItem(ListaItems, id, x, y, RECURSO_ITEM_TYPE, cant);
    return temp;
}

void BorrarItem(ITEM_NIVEL** ListaItems, char id) {
        ITEM_NIVEL * temp = *ListaItems;
        ITEM_NIVEL * oldtemp;

        if ((temp != NULL) && (temp->id == id)) {
                *ListaItems = (*ListaItems)->next;
		free(temp);
        } else {
                while((temp != NULL) && (temp->id != id)) {
                        oldtemp = temp;
                        temp = temp->next;
                }
                if ((temp != NULL) && (temp->id == id)) {
                        oldtemp->next = temp->next;
			free(temp);
                }
        }

}

void MoverPersonaje(ITEM_NIVEL* ListaItems, char id, int x, int y) {

        ITEM_NIVEL * temp;
        temp = ListaItems;

        while ((temp != NULL) && (temp->id != id)) {
                temp = temp->next;
        }
        if ((temp != NULL) && (temp->id == id)) {
                temp->posx = x;
                temp->posy = y;
        }

}


int restarRecurso(ITEM_NIVEL* ListaItems, char id) {

        ITEM_NIVEL * temp;
        temp = ListaItems;

        while ((temp != NULL) && (temp->id != id)) {
                temp = temp->next;
        }
        if ((temp != NULL) && (temp->id == id)) {
                if ((temp->item_type) && (temp->quantity > 0)) {
                        temp->quantity--;
                        return temp->quantity+1;
                } else return 0;
        }
        return -1;
}

ITEM_NIVEL* obtenerRecurso(ITEM_NIVEL* ListaItems, char id) {

        ITEM_NIVEL * temp;
        temp = ListaItems;

        while ((temp != NULL) && (temp->id != id)) {
                temp = temp->next;
        }
        if ((temp != NULL) && (temp->id == id)) {
                if ((temp->item_type)&& (temp->item_type==RECURSO_ITEM_TYPE)) {
                        return temp;
                }
        }
return NULL;
}

ITEM_NIVEL* obtenerPersonaje(ITEM_NIVEL* ListaItems, char id) {

        ITEM_NIVEL * temp;
        temp = ListaItems;

        while ((temp != NULL) && (temp->id != id)) {
                temp = temp->next;
        }
        if ((temp != NULL) && (temp->id == id)) {
                if ((temp->item_type) && (temp->item_type==PERSONAJE_ITEM_TYPE)) {
                        return temp;
                }
        }
return NULL;
}

