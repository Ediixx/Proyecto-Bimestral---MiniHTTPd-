#include "mime.h"

#include <string.h>

/* cada extension con su tipo mime */
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
    {NULL, NULL} /* marca donde termina la tabla */
};

/* busca la extension del archivo y devuelve su Content-Type */
const char *mime_type(const char *path) {
  const char *ext;
  int i;

  ext = strrchr(path, '.'); /* ultimo punto: "www/style.css" -> ".css" */
  if (ext == NULL)
    return "application/octet-stream";

  for (i = 0; tabla[i].ext != NULL; i++) {
    if (strcmp(ext, tabla[i].ext) == 0)
      return tabla[i].type;
  }

  /* si no esta en la tabla lo mandamos como binario generico */
  return "application/octet-stream";
}
