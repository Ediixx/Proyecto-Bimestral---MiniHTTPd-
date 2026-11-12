#ifndef HTTP_H
#define HTTP_H

#define BUFSZ 4096
#define MAX_LINE 1024
#define MAX_HEADER 1024
#define MAX_URI 512

/* Atiende UNA solicitud del cliente. Se devuelve 1 si la conexión debe seguir
 * abierta (keep-alive) o 0 si debe cerarse*/
int handle_client(int nsd);

#endif