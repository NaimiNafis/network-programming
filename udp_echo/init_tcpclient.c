#include "mynet.h"

int init_tcpclient(char *servername, in_port_t serverport)
{
  struct hostent *server_host;
  struct sockaddr_in server_adrs;
  int sock;

  /* サーバ名をアドレス(hostent構造体)に変換する */
  if((server_host = gethostbyname( servername )) == NULL){
    exit_errmesg("gethostbyname()");
  }

  /* サーバの情報をsockaddr_in構造体に格納する */
  memset(&server_adrs, 0, sizeof(server_adrs));
  server_adrs.sin_family = AF_INET;
  server_adrs.sin_port = htons(serverport);
  memcpy(&server_adrs.sin_addr, server_host->h_addr_list[0], server_host->h_length);

  /* ソケットをSTREAMモードで作成する */
  if((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1){
    exit_errmesg("socket()");
  }

  /* ソケットにサーバの情報を対応づけてサーバに接続する */
  if(connect( sock, (struct sockaddr *)&server_adrs, sizeof(server_adrs) )== -1){
    exit_errmesg("connect()");
  }

  return(sock);
}
