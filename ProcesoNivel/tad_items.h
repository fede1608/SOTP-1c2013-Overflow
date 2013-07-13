#include "nivel.h"

void BorrarItem(ITEM_NIVEL** i, char id);
int restarRecurso(ITEM_NIVEL* i, char id);
int sumarRecurso(ITEM_NIVEL* ListaItems, char id, int cant);
void MoverPersonaje(ITEM_NIVEL* i, char personaje, int x, int y);
ITEM_NIVEL* CrearPersonaje(ITEM_NIVEL** i, char id, int x , int y);
ITEM_NIVEL* CrearCaja(ITEM_NIVEL** i, char id, int x , int y, int cant);
ITEM_NIVEL* CrearItem(ITEM_NIVEL** i, char id, int x, int y, char tipo, int cant);
ITEM_NIVEL* obtenerRecurso(ITEM_NIVEL* ListaItems, char id);
ITEM_NIVEL* obtenerPersonaje(ITEM_NIVEL* ListaItems, char id);
