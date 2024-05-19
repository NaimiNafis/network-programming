#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 50000 /* ポート番号 ←適当に書き換える */
#define BUFSIZE 50 /* バッファサイズ */

int main()
{
    struct sockaddr_in my_adrs;

    int sock_listen, sock_accepted;

    char buf[BUFSIZE];
    int strsize;

    /* サーバ(自分自身)の情報をsockaddr_in構造体に格納する */
    memset(&my_adrs, 0, sizeof(my_adrs));
    my_adrs.sin_family = AF_INET;
    my_adrs.sin_port = htons(PORT);
    my_adrs.sin_addr.s_addr = htonl(INADDR_ANY);

    /* 待ち受け用ソケットをSTREAMモードで作成する */
    if ((sock_listen = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "socket()");
        exit(EXIT_FAILURE);
    }

    /* 待ち受け用のソケットに自分自身のアドレス情報を結びつける */
    if (bind(sock_listen, (struct sockaddr*)&my_adrs, sizeof(my_adrs)) == -1) {
        fprintf(stderr, "bind()");
        exit(EXIT_FAILURE);
    }

    /* クライアントからの接続を受け付ける準備をする */
    if (listen(sock_listen, 10) == -1) {
        fprintf(stderr, "listen()");
        exit(EXIT_FAILURE);
    }

    /* クライアントの接続を受け付ける */
    sock_accepted = accept(sock_listen, NULL, NULL);

    close(sock_listen);

    do {
        /* 文字列をクライアントから受信する */
        if ((strsize = recv(sock_accepted, buf, BUFSIZE, 0)) == -1) {
            fprintf(stderr, "recv()");
            exit(EXIT_FAILURE);
        }

        /* 文字列をクライアントに送信する */
        if (send(sock_accepted, buf, strsize, 0) == -1) {
            fprintf(stderr, "send()");
            exit(EXIT_FAILURE);
        }
    } while (buf[strsize - 1] != '\n'); /* 改行コードを受信するまで繰り返す */

    close(sock_accepted);

    exit(EXIT_SUCCESS);
}
