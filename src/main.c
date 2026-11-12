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

  if (argc >= 2)
    portno = atoi(argv[1]);

  if (portno <= 0 || portno > 65535) {
    fprintf(stderr, "Uso: %s [puerto]\n", argv[0]);
    return 1;
  }

  /* Si el cliente cierra mientras le enviamos datos, write() devuelve -1
     en vez de matar el proceso con la señal SIGPIPE */
  signal(SIGPIPE, SIG_IGN);

  /* Socket maestro */
  sd = server_init(portno, QLEN);

  /* 1. Crear la instancia de epoll */
  epfd = epoll_create1(0);
  if (epfd < 0) {
    perror("ERROR en epoll_create1");
    return 1;
  }

  /* 2. Registrar el socket maestro: EPOLLIN = hay un cliente esperando */
  ev.events = EPOLLIN;
  ev.data.fd = sd;
  if (epoll_ctl(epfd, EPOLL_CTL_ADD, sd, &ev) < 0) {
    perror("ERROR en epoll_ctl");
    return 1;
  }

  printf("Servidor escuchando en el puerto %d\n", portno);

  /* 3. Bucle de eventos */
  while (1) {
    /* Duerme hasta que algún socket tenga algo; -1 = sin límite de tiempo */
    nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
    if (nfds < 0) {
      perror("ERROR en epoll_wait");
      continue;
    }

    for (i = 0; i < nfds; i++) {
      fd = events[i].data.fd;

      if (fd == sd) {
        /* Evento en el maestro: aceptar al cliente y registrarlo en epoll */
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
        /* Evento en un cliente: llegó una solicitud (o se desconectó).
           handle_client devuelve 0 si hay que cerrar la conexión */
        if (handle_client(fd) == 0) {
          epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
          close(fd);
        }
        /* Si devuelve 1 (keep-alive) el socket sigue en epoll
           esperando la siguiente solicitud del mismo cliente */
      }
    }
  }

  return 0;
}