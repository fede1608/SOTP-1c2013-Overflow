Proceso Koopa
=============

Este proceso tiene las siguientes dependencias:
- libmemoria.so (Librería de manejo de memoria)
- libso-commons-library.so

Para ejecutarlo, setear antes la variable de entorno LD_LIBRARY_PATH desde bash. El valor de la variable debe ser una cadena con las rutas donde estén ubicadas las librerías separadas por :
-> Ejemplo:
export LD_LIBRARY_PATH=/home/utnso/workspace/memoria/Debug/:/home/utnso/workspace/so-commons-library/Debug
o
export LD_LIBRARY_PATH=/home/utnso/tp-20131c-overflow/Libs/:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/home/utnso/TPSO/tp-20131c-overflow/Libs:$LD_LIBRARY_PATH

curl -u 'overflowDT' -L -o TP.tar https://api.github.com/repos/sisoputnfrba/tp-20131c-overflow/tarball/master

sudo rm /etc/udev/rules.d/70-persistent-net.rules

chmod -v -R 777 carpetaDelRepo

export LD_LIBRARY_PATH=/home/utnso/tp-20131c-overflow-master/Tests/Libs/



Parámetros
----------
./koopa [archivo.txt]
Modo normal. Ejecutará la lista de pedidos que contenga el archivo y notificará si el algoritmo funciona correctamente.

./koopa -debug
Modo de pruebas, para verificar rápidamente el funcionamiento de la librería. Se escriben por la entrada operaciones como GRABAR:#:4:abcd y se van mostrando por pantalla los resultados a medida que se ejecutan.

Librería
----------
En /memoria hay un ejemplo de implementación de la librería de memoria. Al importar ese proyecto al eclipse debería poderse compilar y empezar a trabajar sobre el código.
La única precondición es tener importado también el proyecto so-commons-library.

1) Abrir el Ubuntu Server desde VM VirtualBox
2) Ejecutar ifconfig y ver la IP
3) Abrir WinSCP. Usar la IP del paso 2 y el login utnso/utnso
4) Abrir el archivo .zip con el TP y copiar todo lo de adentro al server
5) Ejecutar chmod -v -R 777 tp-20131c-overflow-master
6) Ejecutar export LD_LIBRARY_PATH=/home/utnso/tp-20131c-overflow-master/Tests/Libs/
------------------------------------------------------------------------------------
7) DEJAR ABIERTO EL SERVIDOR EN LA VM Y ABRIR LA CANTIDAD DE PUTTYS NECESARIOS
------------------------------------------------------------------------------------
Para cada Putty configurar y ejecutar cuando corresponda.
Ejemplo para levantar un personaje:

8) cd tp-20131c-overflow-master/Tests/Esquema1/P1
9) nano config.txt (cambiar la IP por la del servidor de la plataforma)
10) Apretar Ctrl+X -> S -> Enter

Esperar hasta que todos estén listos y ejecutar el siguiente comando para que arranque el personaje

11) ./P1