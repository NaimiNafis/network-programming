#include "mynet.h"
#include <sys/wait.h>

#define PRCS_LIMIT 10 /* プロセス数制限 */
#define BUFSIZE 50   /* バッファサイズ */

int main(int argc, char *argv[])
{
  int port_number;
  int sock_listen, sock_accepted;
  int n_process = 0;
  pid_t child;
  char buf[BUFSIZE];
  int strsize;

  /* 引数のチェックと使用法の表示 */
  if( argc != 2 ){
    fprintf(stderr,"Usage: %s Port_number\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  port_number = atoi(argv[1]);

  /* サーバの初期化 */
  sock_listen = init_tcpserver(port_number, 5);

  for(;;){

    /* クライアントの接続を受け付ける */
    sock_accepted = accept(sock_listen, NULL, NULL);

    if( (child=fork()) == 0 ){
      /* Child process */
      close(sock_accepted);
      do{
	/* 文字列をクライアントから受信する */
	if((strsize=recv(sock_accepted, buf, BUFSIZE, 0)) == -1){
	  exit_errmesg("recv()");
	}

	/* 文字列をクライアントに送信する */
	if(send(sock_accepted, buf, strsize, 0) == -1 ){
	  exit_errmesg("send()");
	}
      }while( buf[strsize-1] != '\n' ); /* 改行コードを受信するまで繰り返す */

      close(sock_accepted);
      exit(EXIT_SUCCESS);
    }
    else if( child >0 ){
      /* parent's process */
      n_process++;
      printf("Client is accepted.[%d]\n", child);
      close(sock_accepted);
    }
    else{
      /* fork()に失敗 */
      close(sock_listen);
      exit_errmesg("fork()");
    }

    /* ゾンビプロセスの回収 */
    if( n_process == PRCS_LIMIT ){
      child=wait(NULL); /* 制限数を超えたら 空きが出るまでブロック */
      n_process--;
    }

    while( (child=waitpid(-1, NULL, WNOHANG ))>0 ){
      n_process--;
    }

  }

  /* never reached */
}