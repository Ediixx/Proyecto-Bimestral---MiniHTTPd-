#include "server.h"

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static void error(const char *msg) {
  perror(msg);
  exit(1);
}

/* Crea, enlaza y pone a escuchar el socket maestro */
int server_init(int portno, int qlen) {
  int sockfd, opt = 1;
  struct sockaddr_in serv_addr;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    error("ERROR al abrir el socket");

  /* Permite reiniciar el servidor sin esperar al TIME_WAIT */
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    error("ERROR en bind");

  if (listen(sockfd, qlen) < 0)
    error("ERROR al escuchar");

  return sockfd;
}