#include "http.h"
#include "files.h"
#include "mime.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

/* pasa el codigo numerico al texto del estado */
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

/* manda la linea de estado y los encabezados de la respuesta
   (reemplaza el write("HTTP/1.1 200 OK...") q teniamos en clase) */
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

/* manda una respuesta de error con una paginita html */
static void send_error(int nsd, int status, int keep_alive) {
  char body[256];
  int n;

  n = snprintf(body, sizeof(body), "<html><body><h1>%d %s</h1></body></html>\n",
               status, status_text(status));
  send_headers(nsd, status, "text/html", n, keep_alive);
  write_all(nsd, body, n);
}

/* responde 400 y cierra la conexion */
static int bad_request(int nsd) {
  send_error(nsd, 400, 0);
  return 0;
}

/* atiende UNA peticion del cliente. Devuelve 1 si la conexion sigue abierta
   (keep-alive) o 0 si hay q cerrarla */
int handle_client(int nsd) {
  char buf[BUFSZ];
  char path[PATH_MAX];
  char *ptrS, *ptrE, *end, *colon;
  char *method, *uri, *version;
  char *host = NULL, *connection = NULL, *agent = NULL;
  int nread, fd, status, keep_alive;
  long size;

  /* 1. leemos la peticion (dejamos 1 byte para el '\0') */
  nread = read(nsd, buf, BUFSZ - 1);
  if (nread <= 0)
    return 0; /* el cliente cerro la conexion */
  buf[nread] = '\0';

  /* 2. toda peticion termina con una linea vacia "\r\n\r\n", si no esta es
     pq esta mal o es muy grande */
  end = strstr(buf, "\r\n\r\n");
  if (end == NULL)
    return bad_request(nsd);
  end[2] = '\0'; /* cortamos despues del ultimo "\r\n" */

  /* 3. linea de la peticion: "GET /index.html HTTP/1.1" */
  ptrE = strstr(buf, "\r\n");
  *ptrE = '\0';
  if (strlen(buf) >= MAX_LINE)
    return bad_request(nsd);

  method = buf;
  ptrS = strchr(method, ' '); /* espacio entre metodo y uri */
  if (ptrS == NULL)
    return bad_request(nsd);
  *ptrS = '\0';

  uri = ptrS + 1;
  ptrS = strchr(uri, ' '); /* espacio entre uri y version */
  if (ptrS == NULL)
    return bad_request(nsd);
  *ptrS = '\0';

  version = ptrS + 1;

  /* 4. encabezados: una linea "Nombre: valor" cada uno */
  ptrS = ptrE + 2; /* aqui empieza el primer encabezado */
  while (*ptrS != '\0') {
    ptrE = strstr(ptrS, "\r\n");
    *ptrE = '\0';
    if (strlen(ptrS) >= MAX_HEADER)
      return bad_request(nsd);

    colon = strchr(ptrS, ':');
    if (colon == NULL)
      return bad_request(nsd); /* encabezado sin ':' esta mal */
    *colon = '\0';
    colon++;
    while (*colon == ' ') /* saltamos los espacios despues de ':' */
      colon++;

    /* solo guardamos los 3 q piden: Host, Connection y User-Agent */
    if (strcasecmp(ptrS, "Host") == 0)
      host = colon;
    else if (strcasecmp(ptrS, "Connection") == 0)
      connection = colon;
    else if (strcasecmp(ptrS, "User-Agent") == 0)
      agent = colon;

    ptrS = ptrE + 2; /* pasamos al siguiente encabezado */
  }

  /* 5. conexion persistente: solo aceptamos HTTP/1.1 */
  if (strcmp(version, "HTTP/1.1") != 0) {
    return bad_request(nsd); /* cualquier otra version se rechaza */
  }
  /* en HTTP/1.1 la conexion queda abierta por defecto, solo se cierra si
     piden "Connection: close" */
  keep_alive = 1;
  if (connection != NULL && strcasecmp(connection, "close") == 0) {
    keep_alive = 0;
  }

  /* mostramos en consola lo q llego (para ver q funciona) */
  printf("%s %s | Host: %s | User-Agent: %s\n", method, uri, host ? host : "-",
         agent ? agent : "-");

  /* 6. validaciones */
  if (uri[0] != '/' || strlen(uri) >= MAX_URI)
    return bad_request(nsd);

  if (host == NULL)
    return bad_request(nsd); /* HTTP/1.1 obliga a mandar el Host */

  /* solo se permite GET, lo demas es 405 */
  if (strcmp(method, "GET") != 0) {
    send_error(nsd, 405, keep_alive);
    return keep_alive;
  }

  /* 7. quitamos los parametros: "/index.html?x=1" -> "/index.html" */
  ptrS = strchr(uri, '?');
  if (ptrS != NULL)
    *ptrS = '\0';

  /* 8. buscamos el archivo dentro de www/ */
  status = file_open(uri, path, &fd, &size);
  if (status != 200) {
    send_error(nsd, status, keep_alive);
    return keep_alive;
  }

  /* 9. mandamos los encabezados y despues el archivo */
  send_headers(nsd, 200, mime_type(path), size, keep_alive);
  if (file_send(nsd, fd) < 0)
    keep_alive = 0; /* el cliente se fue a la mitad del envio */
  close(fd);

  return keep_alive;
}
