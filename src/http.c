#include "http.h"
#include "files.h"
#include "mime.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

/* Código numérico -> texto del estado */
static const char *status_text(int status) {
  switch (status) {
  case 200:
    return "OK";
  case 400:
    return "Bad Request";
  case 403:
    return "Forbidden";
  case 404:
    return "Not Found";
  case 405:
    return "Method Not Allowed";
  default:
    return "Internal Server Error";
  }
}

/* Envía la línea de estado y los encabezados de la respuesta.
   Reemplaza el write(nsd, "HTTP/1.1 200 OK\r\n\r\n", 19) del código de clase */
static void send_headers(int nsd, int status, const char *type, long length,
                         int keep_alive) {
  char hdr[512];
  int n;

  n = snprintf(hdr, sizeof(hdr),
               "HTTP/1.1 %d %s\r\n"
               "Content-Type: %s\r\n"
               "Content-Length: %ld\r\n"
               "Connection: %s\r\n"
               "\r\n",
               status, status_text(status), type, length,
               keep_alive ? "keep-alive" : "close");
  write_all(nsd, hdr, n);
}

/* Envía una respuesta de error con una página HTML pequeña */
static void send_error(int nsd, int status, int keep_alive) {
  char body[256];
  int n;

  n = snprintf(body, sizeof(body), "<html><body><h1>%d %s</h1></body></html>\n",
               status, status_text(status));
  send_headers(nsd, status, "text/html", n, keep_alive);
  write_all(nsd, body, n);
}

/* 400 Bad Request y cerrar la conexión */
static int bad_request(int nsd) {
  send_error(nsd, 400, 0);
  return 0;
}

int handle_client(int nsd) {
  char buf[BUFSZ];
  char path[PATH_MAX];
  char *ptrS, *ptrE, *end, *colon;
  char *method, *uri, *version;
  char *host = NULL, *connection = NULL, *agent = NULL;
  int nread, fd, status, keep_alive;
  long size;

  /* 1. Leer la solicitud (se deja 1 byte para el '\0') */
  nread = read(nsd, buf, BUFSZ - 1);
  if (nread <= 0)
    return 0; /* El cliente cerró la conexión */
  buf[nread] = '\0';

  /* 2. Toda solicitud termina con una línea vacía "\r\n\r\n".
        Si no está, la solicitud es demasiado grande o está mal formada */
  end = strstr(buf, "\r\n\r\n");
  if (end == NULL)
    return bad_request(nsd);
  end[2] = '\0'; /* Cortar justo después del último "\r\n" */

  /* 3. Línea de solicitud: "GET /index.html HTTP/1.1" */
  ptrE = strstr(buf, "\r\n");
  *ptrE = '\0';
  if (strlen(buf) >= MAX_LINE)
    return bad_request(nsd);

  method = buf;
  ptrS = strchr(method, ' '); /* Espacio entre método y URI */
  if (ptrS == NULL)
    return bad_request(nsd);
  *ptrS = '\0';

  uri = ptrS + 1;
  ptrS = strchr(uri, ' '); /* Espacio entre URI y versión */
  if (ptrS == NULL)
    return bad_request(nsd);
  *ptrS = '\0';

  version = ptrS + 1;

  /* 4. Encabezados: una línea "Nombre: valor" por cada uno */
  ptrS = ptrE + 2; /* Inicio del primer encabezado */
  while (*ptrS != '\0') {
    ptrE = strstr(ptrS, "\r\n");
    *ptrE = '\0';
    if (strlen(ptrS) >= MAX_HEADER)
      return bad_request(nsd);

    colon = strchr(ptrS, ':');
    if (colon == NULL)
      return bad_request(nsd); /* Encabezado sin ':' */
    *colon = '\0';
    colon++;
    while (*colon == ' ') /* Saltar espacios después de ':' */
      colon++;

    if (strcasecmp(ptrS, "Host") == 0)
      host = colon;
    else if (strcasecmp(ptrS, "Connection") == 0)
      connection = colon;
    else if (strcasecmp(ptrS, "User-Agent") == 0)
      agent = colon;

    ptrS = ptrE + 2; /* Siguiente encabezado */
  }

  /* 5. Conexión persistente: solo se soporta HTTP/1.1 */
  if (strcmp(version, "HTTP/1.1") != 0) {
    return bad_request(nsd); /* Rechaza cualquier versión que no sea HTTP/1.1 */
  }
  /* En HTTP/1.1 la conexión es persistente por defecto, salvo que pidan "close"
   */
  keep_alive = 1;
  if (connection != NULL && strcasecmp(connection, "close") == 0) {
    keep_alive = 0;
  }

  printf("%s %s | Host: %s | User-Agent: %s\n", method, uri, host ? host : "-",
         agent ? agent : "-");

  /* 6. Validaciones */
  if (uri[0] != '/' || strlen(uri) >= MAX_URI)
    return bad_request(nsd);

  if (host == NULL)
    return bad_request(nsd); /* HTTP/1.1 exige el encabezado Host */

  if (strcmp(method, "GET") != 0) {
    send_error(nsd, 405, keep_alive);
    return keep_alive;
  }

  /* 7. Quitar parámetros: "/index.html?x=1" -> "/index.html" */
  ptrS = strchr(uri, '?');
  if (ptrS != NULL)
    *ptrS = '\0';

  /* 8. Buscar el archivo dentro de www/ */
  status = file_open(uri, path, &fd, &size);
  if (status != 200) {
    send_error(nsd, status, keep_alive);
    return keep_alive;
  }

  /* 9. Enviar encabezados y luego el contenido del archivo */
  send_headers(nsd, 200, mime_type(path), size, keep_alive);
  if (file_send(nsd, fd) < 0)
    keep_alive = 0; /* El cliente se fue a mitad del envío */
  close(fd);

  return keep_alive;
}