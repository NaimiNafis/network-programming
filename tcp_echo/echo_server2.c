#include "mynet.h"

#define PORT 50000 /* ポート番号 */
#define BUFSIZE 50 /* バッファサイズ */

int main() {
    int sock_listen, sock_accepted;
    char buf[BUFSIZE];
    int strsize;

    /* 待ち受け用ソケットを作成し初期化する */
    sock_listen = init_tcpserver(PORT, 10);  /* Use the library function to initialize the server */

    /* クライアントの接続を受け付ける */
    if ((sock_accepted = accept(sock_listen, NULL, NULL)) == -1) {
        exit_errmesg("accept()");
    }

    close(sock_listen);

    do {
        /* 文字列をクライアントから受信する */
        if ((strsize = recv(sock_accepted, buf, BUFSIZE, 0)) == -1) {
            exit_errmesg("recv()");
        }

        /* 文字列をクライアントに送信する */
        if (send(sock_accepted, buf, strsize, 0) == -1) {
            exit_errmesg("send()");
        }
    } while (buf[strsize - 1] != '\n'); /* 改行コードを受信するまで繰り返す */

    close(sock_accepted);

    exit(EXIT_SUCCESS);
}
