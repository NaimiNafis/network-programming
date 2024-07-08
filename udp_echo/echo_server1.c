/*
  echo_server1.c (UDP版)
*/
#include "mynet.h"
#include <arpa/inet.h>

#define BUFSIZE 512   /* バッファサイズ */

void show_adrsinfo(struct sockaddr_in *adrs_in);

int main(int argc, char *argv[])
{
    struct sockaddr_in from_adrs;
    int sock;
    socklen_t from_len;

    char buf[BUFSIZE];
    int strsize;

    /* 引数のチェックと使用法の表示 */
    if( argc != 2 ){
    fprintf(stderr,"Usage: %s Port_number\n", argv[0]);
    exit(EXIT_FAILURE);
    }

    /* UDPサーバの初期化 */
    sock = init_udpserver((in_port_t)atoi(argv[1]));

    for(;;){
    /* 文字列をクライアントから受信する */
    from_len = sizeof(from_adrs);
    strsize = Recvfrom(sock, buf, BUFSIZE, 0, (struct sockaddr *)&from_adrs, &from_len);

    show_adrsinfo(&from_adrs);

    /* 文字列をクライアントに送信する */
    Sendto(sock, buf, strsize, 0, (struct sockaddr *)&from_adrs, sizeof(from_adrs));
    }

    close(sock);

    exit(EXIT_SUCCESS);
}

void show_adrsinfo(struct sockaddr_in *adrs_in)
{
    int port_number;
    char ip_adrs[20];

    strncpy(ip_adrs, inet_ntoa(adrs_in->sin_addr), 20);
    port_number = ntohs(adrs_in->sin_port);

    printf("%s[%d]\n", ip_adrs, port_number);
}
