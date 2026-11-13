#ifndef FILES_H
#define FILES_H

#define DOC_ROOT "www" /* carpeta donde estan los archivos de la pagina */

/* busca el archivo q piden. Devuelve 200, 403, 404 o 500.
   Si devuelve 200 deja el archivo abierto en *fd, el tamaño en *size
   y la ruta real en path (path tiene q ser de tamaño PATH_MAX) */
int file_open(const char *uri, char *path, int *fd, long *size);

/* manda el archivo completo por el socket. Devuelve 0 o -1 si falla */
int file_send(int nsd, int fd);

/* escribe los len bytes completos (write a veces manda menos) */
int write_all(int fd, const char *buf, int len);

#endif
