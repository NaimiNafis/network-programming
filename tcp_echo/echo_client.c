/*
   Proxyを介してHTTPサーバと対話するクライアントプログラム
　　（利用者が自らHTTPを入力する必要がある）
*/
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PROXYPORT 50000 /* プロキシサーバのポート番号 */
#define BUFSIZE 102400 /* バッファサイズ */

int main(int argc, char **argv)
{
    struct hostent* server_host;
    struct sockaddr_in proxy_adrs;

    int tcpsock;

    char proxyname[] = "localhost"; /* プロキシサーバ */
    char k_buf[BUFSIZE], s_buf[BUFSIZE], r_buf[BUFSIZE];
    int strsize;

    /* サーバ名をアドレス(hostent構造体)に変換する */
    if ((server_host = gethostbyname(proxyname)) == NULL) {
        fprintf(stderr, "gethostbyname()");
        exit(EXIT_FAILURE);
    }

    /* サーバの情報をsockaddr_in構造体に格納する */
    memset(&proxy_adrs, 0, sizeof(proxy_adrs));
    proxy_adrs.sin_family = AF_INET;
    proxy_adrs.sin_port = htons(PROXYPORT);
    memcpy(&proxy_adrs.sin_addr, server_host->h_addr_list[0], server_host->h_length);

    /* ソケットをSTREAMモードで作成する */
    if ((tcpsock = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "socket()");
        exit(EXIT_FAILURE);
    }

    /* ソケットにサーバの情報を対応づけてサーバに接続する */
    if (connect(tcpsock, (struct sockaddr*)&proxy_adrs, sizeof(proxy_adrs)) < 0) {
        fprintf(stderr, "connect");
        exit(EXIT_FAILURE);
    }

    /* キーボードから文字列を入力してサーバに送信 */
    while (*fgets(k_buf, BUFSIZE, stdin) != '\n') { /* 空行が入力されるまで繰り返し */
        strsize = strlen(k_buf);
        k_buf[strsize - 1] = 0; /* 末尾の改行コードを消す */
        snprintf(s_buf, BUFSIZE, "%s\r\n", k_buf); /* HTTPの改行コードは \r\n */

        /* 文字列をサーバに送信する */
        if (send(tcpsock, s_buf, strsize + 1, 0) == -1) {
            fprintf(stderr, "send()");
            exit(EXIT_FAILURE);
        }
    }

    send(tcpsock, "\r\n", 2, 0); /* HTTPのメソッド（コマンド）の終わりは空行 */

    /* サーバから文字列を受信する */
    if ((strsize = recv(tcpsock, r_buf, BUFSIZE - 1, 0)) == -1) {
        fprintf(stderr, "recv()");
        exit(EXIT_FAILURE);
    }
    r_buf[strsize] = '\0';

    printf("%s", r_buf); /* 受信した文字列を画面に書く */

    close(tcpsock); /* ソケットを閉じる */
    exit(EXIT_SUCCESS);
}
