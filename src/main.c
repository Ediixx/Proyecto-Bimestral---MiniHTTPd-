#include "http.h"
#include "server.h"

#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_EVENTS 64

int main(int argc, char *argv[]) {
  struct sockaddr_in cli_addr;
  socklen_t clilen;
  struct epoll_event ev, events[MAX_EVENTS];
  int portno = 80;
  int sd, nsd, epfd, nfds, i, fd;

  /* si pasan el puerto por argumento lo usamos, sino queda el 80 */
  if (argc >= 2)
    portno = atoi(argv[1]);

  /* los puertos solo van de 1 a 65535 */
  if (portno <= 0 || portno > 65535) {
    fprintf(stderr, "Uso: %s [puerto]\n", argv[0]);
    return 1;
  }

  /* si el cliente se desconecta mientras le mandamos datos el servidor
     se caia por la señal SIGPIPE, con esto la ignoramos */
  signal(SIGPIPE, SIG_IGN);

  /* socket maestro (el que escucha) */
  sd = server_init(portno, QLEN);

  /* 1. creamos el epoll */
  epfd = epoll_create1(0);
  if (epfd < 0) {
    perror("ERROR en epoll_create1");
    return 1;
  }

  /* 2. metemos el socket maestro al epoll, EPOLLIN avisa cuando llega un
     cliente nuevo */
  ev.events = EPOLLIN;
  ev.data.fd = sd;
  if (epoll_ctl(epfd, EPOLL_CTL_ADD, sd, &ev) < 0) {
    perror("ERROR en epoll_ctl");
    return 1;
  }

  printf("Servidor escuchando en el puerto %d\n", portno);

  /* 3. bucle principal de eventos */
  while (1) {
    /* se queda esperando hasta que algun socket tenga algo, el -1 es que
       espere sin limite de tiempo */
    nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
    if (nfds < 0) {
      perror("ERROR en epoll_wait");
      continue;
    }

    /* recorremos solo los sockets que tienen eventos */
    for (i = 0; i < nfds; i++) {
      fd = events[i].data.fd;

      if (fd == sd) {
        /* evento en el maestro = cliente nuevo, lo aceptamos y lo metemos
           al epoll */
        clilen = sizeof(cli_addr);
        nsd = accept(sd, (struct sockaddr *)&cli_addr, &clilen);
        if (nsd < 0) {
          perror("Error al aceptar");
          continue;
        }
        ev.events = EPOLLIN;
        ev.data.fd = nsd;
        epoll_ctl(epfd, EPOLL_CTL_ADD, nsd, &ev);
      } else {
        /* evento en un cliente = mando una peticion o se desconecto.
           handle_client devuelve 0 si hay q cerrar la conexion */
        if (handle_client(fd) == 0) {
          epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
          close(fd);
        }
        /* si devuelve 1 (keep-alive) no cerramos, el socket sigue en el
           epoll esperando la otra peticion del mismo cliente */
      }
    }
  }

  return 0;
}
