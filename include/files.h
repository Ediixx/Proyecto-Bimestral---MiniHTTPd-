#ifndef FILES_H
#define FILES_H

#define DOC_ROOT "www" /* Carpeta con los archivos del sitio */

/* Busca el archivo pedido. Devuelve 200, 403, 404 o 500.
   Si devuelve 200: deja el archivo abierto en *fd, su tamaño en *size
   y la ruta real en path (path debe tener tamaño PATH_MAX) */
int file_open(const char *uri, char *path, int *fd, long *size);

/* Envía el archivo completo por el socket. Devuelve 0 o -1 si falla */
int file_send(int nsd, int fd);

/* Escribe len bytes completos (write puede escribir menos de lo pedido) */
int write_all(int fd, const char *buf, int len);

#endif