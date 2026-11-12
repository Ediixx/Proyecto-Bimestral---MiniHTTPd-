#include "files.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define CHUNK 1024

int write_all(int fd, const char *buf, int len) {
  int n;

  while (len > 0) {
    n = write(fd, buf, len);
    if (n < 0)
      return -1;
    buf += n; /* Avanzar lo que ya se envió */
    len -= n;
  }
  return 0;
}

int file_open(const char *uri, char *path, int *fd, long *size) {
  char fname[PATH_MAX];
  char root[PATH_MAX];
  struct stat st;
  size_t len;

  /* "/" -> "/index.html" (igual que en el código de clase) */
  if (strcmp(uri, "/") == 0)
    uri = "/index.html";

  /* Filtro rápido contra Directory Traversal */
  if (strstr(uri, "..") != NULL)
    return 403;

  /* "www" + "/style.css" -> "www/style.css" (snprintf en vez de strcpy) */
  snprintf(fname, sizeof(fname), "%s%s", DOC_ROOT, uri);

  /* realpath convierte a ruta absoluta real, resolviendo "..", "." y enlaces.
     Ej: "www/style.css" -> "/home/user/minihttpd/www/style.css" */
  if (realpath(DOC_ROOT, root) == NULL)
    return 500;

  if (realpath(fname, path) == NULL) {
    if (errno == ENOENT || errno == ENOTDIR)
      return 404; /* No existe */
    if (errno == EACCES)
      return 403; /* Sin permiso */
    return 500;
  }

  /* La ruta real DEBE empezar con la ruta real de www/ */
  len = strlen(root);
  if (strncmp(path, root, len) != 0 || path[len] != '/')
    return 403;

  /* Abrir el archivo (el open del código de clase, ahora revisando errores) */
  *fd = open(path, O_RDONLY);
  if (*fd < 0) {
    if (errno == EACCES)
      return 403;
    return 500;
  }

  /* fstat da el tamaño (para Content-Length) y el tipo de archivo */
  if (fstat(*fd, &st) < 0) {
    close(*fd);
    return 500;
  }
  if (!S_ISREG(st.st_mode)) { /* Es una carpeta u otra cosa */
    close(*fd);
    return 403;
  }

  *size = st.st_size;
  return 200;
}

/* El bucle del código de clase: leer del archivo y escribir en el socket */
int file_send(int nsd, int fd) {
  char buf[CHUNK];
  int nread;

  while ((nread = read(fd, buf, CHUNK)) > 0) {
    if (write_all(nsd, buf, nread) < 0)
      return -1;
  }
  return 0;
}