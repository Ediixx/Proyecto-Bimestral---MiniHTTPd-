#ifndef SERVER_H
#define SERVER_H

#define QLEN 10 /* cuantos clientes pueden esperar en la cola del listen */

/* crea el socket, lo enlaza al puerto y lo deja escuchando */
int server_init(int portno, int qlen);

#endif
