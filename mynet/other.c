#include <stdio.h>
#include <stdlib.h>

#include "mynet.h"

void exit_errmesg(const char *errmesg)
{
  perror(errmesg);
  exit(EXIT_FAILURE);
}

int Sendto(
  int sock,
  const void *s_buf,
  size_t strsize,
  int flags,
  const struct sockaddr *to,
  socklen_t tolen)
{
  int r;
  if ((r = sendto(sock, s_buf, strsize, 0, to, tolen)) == -1) {
      exit_errmesg("sendto()");
  }

  return (r);
}

int Recvfrom(
  int sock,
  void *r_buf,
  size_t len,
  int flags,
  struct sockaddr *from,
  socklen_t *fromlen)
{
  int r;
  if ((r = recvfrom(sock, r_buf, len, 0, from, fromlen)) == -1) {
      exit_errmesg("recvfrom()");
  }

  return (r);
}