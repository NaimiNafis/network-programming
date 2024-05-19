#include <stdio.h>
#include <stdlib.h>

void exit_errmesg(char *errmesg)
{
  perror(errmesg);
  exit(EXIT_FAILURE);
}
