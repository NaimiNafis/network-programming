/*
  mynet.h
*/
#ifndef MYNET_H_
#define MYNET_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

int init_tcpserver(in_port_t myport, int backlog);
int init_tcpclient(char *servername, in_port_t serverport);
void exit_errmesg(char *errmesg);
#endif  /* MYNET_H_ */
