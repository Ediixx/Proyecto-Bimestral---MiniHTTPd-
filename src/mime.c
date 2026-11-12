#include "mime.h"

#include <string.h>

struct mime {
  const char *ext;
  const char *type;
};

static struct mime tabla[] = {
    {".html", "text/html"},
    {".css", "text/css"},
    {".js", "application/javascript"},
    {".png", "image/png"},
    {".jpg", "image/jpeg"},
    {NULL, NULL} /* Marca el final de la tabla */
};

const char *mime_type(const char *path) {
  const char *ext;
  int i;

  ext = strrchr(path, '.'); /* Último punto: "www/style.css" -> ".css" */
  if (ext == NULL)
    return "application/octet-stream";

  for (i = 0; tabla[i].ext != NULL; i++) {
    if (strcmp(ext, tabla[i].ext) == 0)
      return tabla[i].type;
  }

  return "application/octet-stream"; /* Tipo desconocido: datos binarios */
}