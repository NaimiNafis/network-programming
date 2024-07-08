/*
  task5_client.c
*/

#include "mynet.h"
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>

#define BUFSIZE 512
#define TIMEOUT_SEC 10
#define MAX_RETRIES 3
#define DEFAULT_PORT 50001

// パケットタイプを定義します
enum PacketType {
    HELLO,
    HERE,
    JOIN,
    POST,
    MESSAGE,
    QUIT
};

// HELOパケットをブロードキャストし、HERE応答を待ちます
void broadcast_helo(int udp_sock, struct sockaddr_in *broadcast_adrs, char *server_ip, size_t ip_len) {
    for (int retries = 0; retries < MAX_RETRIES; retries++) {
        char buffer[BUFSIZE];
        snprintf(buffer, sizeof(buffer), "HELO");
        Sendto(udp_sock, buffer, strlen(buffer), 0, (struct sockaddr *)broadcast_adrs, sizeof(*broadcast_adrs));
        printf("Sent HELO packet, attempt %d\n", retries + 1);

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(udp_sock, &readfds);
        struct timeval timeout = {TIMEOUT_SEC, 0};

        int ret = select(udp_sock + 1, &readfds, NULL, NULL, &timeout);
        if (ret > 0) {
            struct sockaddr_in from_adrs;
            socklen_t from_len = sizeof(from_adrs);
            int strsize = Recvfrom(udp_sock, buffer, BUFSIZE, 0, (struct sockaddr *)&from_adrs, &from_len);
            buffer[strsize] = '\0';
            if (strncmp(buffer, "HERE", 4) == 0) {
                strncpy(server_ip, inet_ntoa(from_adrs.sin_addr), ip_len);
                printf("Received HERE from %s\n", inet_ntoa(from_adrs.sin_addr));
                close(udp_sock);
                return;
            }
        }
    }

    fprintf(stderr, "No response from server. Exiting.\n");
    exit(EXIT_FAILURE);
}

// クライアントの操作を処理します
void handle_client(int tcp_sock, char *username) {
    char buf[BUFSIZE];
    memset(buf, 0, BUFSIZE);
    printf("\nWelcome to the chatroom, %s!\n", username);
    printf("You can start chatting now or wait for others to join!\nTo exit, you can type 'QUIT'.\n\n");

    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(tcp_sock, &readfds);
        FD_SET(fileno(stdin), &readfds);

        if (select(tcp_sock + 1, &readfds, NULL, NULL, NULL) > 0) {
            if (FD_ISSET(tcp_sock, &readfds)) {
                memset(buf, 0, BUFSIZE);
                int strsize = recv(tcp_sock, buf, BUFSIZE - 1, 0);
                if (strsize <= 0) {
                    printf("Server disconnected.\n");
                    break;
                }
                buf[strsize] = '\0';
                printf("%s\n", buf);
            }
            if (FD_ISSET(fileno(stdin), &readfds)) {
                memset(buf, 0, BUFSIZE);
                if (fgets(buf, BUFSIZE, stdin) != NULL) {
                    buf[strlen(buf) - 1] = '\0';
                    if (strcmp(buf, "QUIT") == 0) {
                        send(tcp_sock, "QUIT", 4, 0);
                        break;
                    }
                    char sendbuf[BUFSIZE];
                    snprintf(sendbuf, BUFSIZE, "POST %s", buf);
                    send(tcp_sock, sendbuf, strlen(sendbuf), 0);
                }
            }
        }
    }
    close(tcp_sock);
}

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: %s username [port_number]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char username[16];
    strncpy(username, argv[1], 15);
    username[15] = '\0';
    in_port_t port_number = (argc == 3) ? (in_port_t)atoi(argv[2]) : DEFAULT_PORT;

    // UDPクライアントソケットを初期化します
    int udp_sock = init_udpclient();
    int broadcast_sw = 1;

    // UDPソケットでブロードキャストを有効にします
    if (setsockopt(udp_sock, SOL_SOCKET, SO_BROADCAST, &broadcast_sw, sizeof(broadcast_sw)) == -1) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // ブロードキャストアドレスを設定します
    struct sockaddr_in broadcast_adrs;
    memset(&broadcast_adrs, 0, sizeof(broadcast_adrs));
    broadcast_adrs.sin_family = AF_INET;
    broadcast_adrs.sin_port = htons(DEFAULT_PORT); // ブロードキャストには常にサーバポートを使用
    broadcast_adrs.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    // HELOパケットを送信し、HERE応答を待ちます
    char server_ip[20];
    broadcast_helo(udp_sock, &broadcast_adrs, server_ip, sizeof(server_ip));

    // TCPクライアントソケットを初期化し、サーバーに接続します
    int tcp_sock = init_tcpclient(server_ip, DEFAULT_PORT);

    // JOINメッセージを送信します
    char joinMsg[BUFSIZE];
    snprintf(joinMsg, sizeof(joinMsg), "JOIN %s", username);
    send(tcp_sock, joinMsg, strlen(joinMsg), 0);

    // クライアント操作を処理します
    handle_client(tcp_sock, username);

    return 0;
}

