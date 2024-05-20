#include "mynet.h"
#include <pthread.h>

#define N  5  /* 生成するスレッド数 */
#define BUFSIZE 500   /* バッファサイズ */

void * echo_thread(void *arg);

/* スレッド関数の引数 */
struct myarg {
  int sock; /* acceptしたソケット */
  int id;      /* スレッドの通し番号 */
};

int main(int argc, char *argv[])
{
  int port_number;
  int sock_listen, sock_accepted;
  int i;
  struct myarg *tharg;
  pthread_t tid;

  /* 引数のチェックと使用法の表示 */
  if( argc != 2 ){
    fprintf(stderr,"Usage: %s Port_number\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  port_number = atoi(argv[1]); /* 引数の取得 */

  /* サーバの初期化 */
  sock_listen = init_tcpserver(port_number, 5);

  for(i=0; i<N; i++){

    /* クライアントの接続を受け付ける */
    sock_accepted = accept(sock_listen, NULL, NULL);

    /* スレッド関数の引数を用意する */
    if( (tharg = (struct myarg *)malloc(sizeof(struct myarg)))==NULL ){
      exit_errmesg("malloc()");
    }

    tharg->sock = sock_accepted;
    tharg->id = i;
    
    /* スレッドを生成する */
    if( pthread_create(&tid, NULL, echo_thread, (void *)tharg) != 0 ){
      exit_errmesg("pthread_create()");
    }

  }

  pthread_exit(NULL);
}

/* スレッドの本体 */
void * echo_thread(void *arg)
{
  struct myarg *tharg;
  char r_buf[BUFSIZE], s_buf[BUFSIZE];
  int strsize;

  tharg = (struct myarg *)arg;
  pthread_detach(pthread_self()); /* スレッドの分離(終了を待たない) */

  do{
    /* 文字列をクライアントから受信する */
    if((strsize=recv(tharg->sock, r_buf, BUFSIZE, 0)) == -1){
      exit_errmesg("recv()");
    }

    /* クライアントに送り返す文字列を作成する */
    snprintf(s_buf, BUFSIZE, "[Thread #%d] %s\n", tharg->id, r_buf);

    /* 文字列をクライアントに送信する */
    if( send(tharg->sock, s_buf, strlen(s_buf), 0) == -1){
      exit_errmesg("send()");
    }
  }while( r_buf[strsize-1] != '\n' ); /* 改行コードを受信するまで繰り返す */

  close(tharg->sock);   /* ソケットを閉じる */
  free(tharg);   /* 引数用のメモリを開放する */
  return(NULL);
}