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

/* write a veces manda menos bytes de los q le pedimos, entonces se repite
   hasta q se envie todo */
int write_all(int fd, const char *buf, int len) {
  int n;

  while (len > 0) {
    n = write(fd, buf, len);
    if (n < 0)
      return -1;
    buf += n; /* avanzamos lo que ya se mando */
    len -= n;
  }
  return 0;
}

/* busca el archivo dentro de www/ y lo abre. Devuelve el codigo http
   (200, 403, 404 o 500) */
int file_open(const char *uri, char *path, int *fd, long *size) {
  char fname[PATH_MAX];
  char root[PATH_MAX];
  struct stat st;
  size_t len;

  /* si piden "/" mandamos el index.html (como en el codigo de clase) */
  if (strcmp(uri, "/") == 0)
    uri = "/index.html";

  /* primer filtro contra directory traversal, si trae ".." lo rechazamos */
  if (strstr(uri, "..") != NULL)
    return 403;

  /* armamos la ruta: "www" + "/style.css" -> "www/style.css"
     (usamos snprintf pq strcpy es inseguro) */
  snprintf(fname, sizeof(fname), "%s%s", DOC_ROOT, uri);

  /* realpath saca la ruta real completa, sin ".." ni enlaces */
  if (realpath(DOC_ROOT, root) == NULL)
    return 500;

  if (realpath(fname, path) == NULL) {
    if (errno == ENOENT || errno == ENOTDIR)
      return 404; /* no existe */
    if (errno == EACCES)
      return 403; /* sin permiso */
    return 500;
  }

  /* la ruta real tiene q empezar con la ruta de www/, sino se salio de la
     carpeta y no lo dejamos */
  len = strlen(root);
  if (strncmp(path, root, len) != 0 || path[len] != '/')
    return 403;

  /* abrimos el archivo (el open de clase pero ahora revisando errores) */
  *fd = open(path, O_RDONLY);
  if (*fd < 0) {
    if (errno == EACCES)
      return 403;
    return 500;
  }

  /* fstat nos da el tamaño (para el Content-Length) y el tipo de archivo */
  if (fstat(*fd, &st) < 0) {
    close(*fd);
    return 500;
  }
  if (!S_ISREG(st.st_mode)) { /* si es una carpeta u otra cosa no sirve */
    close(*fd);
    return 403;
  }

  *size = st.st_size;
  return 200;
}

/* el bucle de clase: lee del archivo y escribe en el socket */
int file_send(int nsd, int fd) {
  char buf[CHUNK];
  int nread;

  while ((nread = read(fd, buf, CHUNK)) > 0) {
    if (write_all(nsd, buf, nread) < 0)
      return -1; /* el cliente cerro la conexion */
  }
  return 0;
}
