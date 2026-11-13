#ifndef HTTP_H
#define HTTP_H

#define BUFSZ 4096    /* tamaño maximo de toda la peticion */
#define MAX_LINE 1024 /* tamaño maximo de la primera linea (GET / HTTP/1.1) */
#define MAX_HEADER 1024 /* tamaño maximo de cada encabezado */
#define MAX_URI 512   /* tamaño maximo de la uri */

/* atiende UNA peticion del cliente. Devuelve 1 si la conexion sigue abierta
   (keep-alive) o 0 si hay q cerrarla */
int handle_client(int nsd);

#endif
