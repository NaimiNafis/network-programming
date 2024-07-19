/*
  task5.c
  サーバーとクライアントの端末が見やすくように、デバッグ用のprintfステートメントは一応コメントアウトしています。
*/

#include "mynet.h"
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>

#define BUFSIZE 512
#define MAX_CLIENTS 30
#define DEFAULT_PORT 50001
#define TIMEOUT_SEC 5
#define MAX_RETRIES 3

typedef struct {
    int sock;
    char username[16];
} ClientInfo;

// ソケットをノンブロッキングモードに設定する関数
void set_nonblocking(int sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl(F_GETFL)");
        exit(EXIT_FAILURE);
    }
    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl(F_SETFL)");
        exit(EXIT_FAILURE);
    }
}

// クライアントの配列を初期化する関数
void init_client_array(ClientInfo clients[]) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].sock = 0;
    }
}

// HELOパケットをブロードキャストし、HERE応答を待ちます
int broadcast_helo(int udp_sock, struct sockaddr_in *broadcast_adrs, char *server_ip, size_t ip_len) {
    for (int retries = 0; retries < MAX_RETRIES; retries++) {
        char buffer[BUFSIZE];
        snprintf(buffer, sizeof(buffer), "HELO");
        Sendto(udp_sock, buffer, strlen(buffer), 0, (struct sockaddr *)broadcast_adrs, sizeof(*broadcast_adrs));
        // printf("Sent HELO packet, attempt %d\n", retries + 1);

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
                // printf("Received HERE from %s\n", inet_ntoa(from_adrs.sin_addr));
                close(udp_sock);
                return 1; // サーバーが見つかりました
            }
        }
    }
    return 0; // サーバーが見つかりません
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

        int maxfd = (tcp_sock > fileno(stdin)) ? tcp_sock : fileno(stdin);
        select(maxfd + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(tcp_sock, &readfds)) {
            int strsize = recv(tcp_sock, buf, BUFSIZE, 0);
            if (strsize <= 0) break;
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
    close(tcp_sock);
}

// クライアントからのメッセージを処理する関数
void process_client_message(int sockfd, ClientInfo clients[], int idx, char *buf) {
    char message[BUFSIZE + 50];
    memset(message, 0, sizeof(message));

    if (strncmp(buf, "JOIN ", 5) == 0) {
        strncpy(clients[idx].username, buf + 5, 15);
        clients[idx].username[15] = '\0';
        printf("%s joined the chat.\n", clients[idx].username);
    } else if (strncmp(buf, "POST ", 5) == 0) {
        snprintf(message, sizeof(message), "[%s] %s", clients[idx].username, buf + 5);
        printf("%s\n", message);
        for (int j = 0; j < MAX_CLIENTS; j++) {
            if (clients[j].sock > 0 && j != idx) {
                if (send(clients[j].sock, message, strlen(message), 0) < 0) {
                    perror("send error");
                }
            }
        }
    } else if (strcmp(buf, "QUIT") == 0) {
        printf("%s has left the chat.\n", clients[idx].username);
        close(sockfd);
        clients[idx].sock = 0;
    }
}

void handle_server(int udp_sock, int tcp_sock, ClientInfo clients[], char *username) {
    struct sockaddr_in from_adrs, client_adrs;
    socklen_t from_len;
    fd_set readfds;
    char buf[BUFSIZE];
    int client_sock, i, strsize;

    printf("Now, I am a server.\n");

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(udp_sock, &readfds);
        FD_SET(tcp_sock, &readfds);
        FD_SET(fileno(stdin), &readfds); // サーバー自身の入力を監視する
        int maxfd = udp_sock > tcp_sock ? udp_sock : tcp_sock;
        maxfd = maxfd > fileno(stdin) ? maxfd : fileno(stdin);

        for (i = 0; i < MAX_CLIENTS; i++) {
            int sockfd = clients[i].sock;
            if (sockfd > 0) {
                FD_SET(sockfd, &readfds);
            }
            if (sockfd > maxfd) {
                maxfd = sockfd;
            }
        }

        select(maxfd + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(udp_sock, &readfds)) {
            from_len = sizeof(from_adrs);
            strsize = recvfrom(udp_sock, buf, BUFSIZE, 0, (struct sockaddr *)&from_adrs, &from_len);
            if (strsize > 0) {
                buf[strsize] = '\0';
                if (strncmp(buf, "HELO", 4) == 0) {
                    sendto(udp_sock, "HERE", 5, 0, (struct sockaddr *)&from_adrs, from_len);
                    // printf("Sent HERE in response to HELO from %s:%d\n", inet_ntoa(from_adrs.sin_addr), ntohs(from_adrs.sin_port));
                }
            }
        }

        if (FD_ISSET(tcp_sock, &readfds)) {
            from_len = sizeof(client_adrs);
            if ((client_sock = accept(tcp_sock, (struct sockaddr *)&client_adrs, &from_len)) != -1) {
                set_nonblocking(client_sock);
                for (i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i].sock == 0) {
                        clients[i].sock = client_sock;
                        // printf("New connection, socket fd is %d, ip is : %s, port : %d, client index: %d\n",
                        //        client_sock, inet_ntoa(client_adrs.sin_addr), ntohs(client_adrs.sin_port), i);
                        break;
                    }
                }
                if (i == MAX_CLIENTS) {
                    // printf("Max clients reached, rejecting new connection\n");
                    close(client_sock);
                }
            } else {
                perror("accept error");
            }
        }

        if (FD_ISSET(fileno(stdin), &readfds)) {
            memset(buf, 0, BUFSIZE);
            if (fgets(buf, BUFSIZE, stdin) != NULL) {
                buf[strlen(buf) - 1] = '\0';
                char sendbuf[BUFSIZE];
                snprintf(sendbuf, BUFSIZE, "MESG [%s] %s", username, buf);
                // printf("%s\n", sendbuf); // サーバーの端末にメッセージを表示する
                for (i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i].sock > 0) {
                        if (send(clients[i].sock, sendbuf, strlen(sendbuf), 0) < 0) {
                            perror("send error");
                            close(clients[i].sock);
                            clients[i].sock = 0;
                        }
                    }
                }
            }
        }

        for (i = 0; i < MAX_CLIENTS; i++) {
            int sockfd = clients[i].sock;
            if (sockfd > 0 && FD_ISSET(sockfd, &readfds)) {
                if ((strsize = recv(sockfd, buf, BUFSIZE, 0)) <= 0) {
                    if (strsize == 0) {
                        // printf("Host disconnected normally, client index: %d\n", i);
                    } else {
                        perror("recv error");
                    }
                    close(sockfd);
                    clients[i].sock = 0;
                    // printf("Socket %d closed\n", sockfd);
                } else {
                    buf[strsize] = '\0';
                    process_client_message(sockfd, clients, i, buf);
                }
            }
        }
    }
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

    // Display program banner
    printf("Task5 (22122063) Naimi Nafis\n");

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
    int server_found = broadcast_helo(udp_sock, &broadcast_adrs, server_ip, sizeof(server_ip));

    if (server_found) {
        // クライアントとしての動作
        int tcp_sock = init_tcpclient(server_ip, DEFAULT_PORT);

        // JOINメッセージを送信します
        char joinMsg[BUFSIZE];
        snprintf(joinMsg, sizeof(joinMsg), "JOIN %s", username);
        send(tcp_sock, joinMsg, strlen(joinMsg), 0);

        // クライアント操作を処理します
        handle_client(tcp_sock, username);
    } else {
        // サーバとしての動作
        ClientInfo clients[MAX_CLIENTS];
        init_client_array(clients);

        // UDPサーバソケットの初期化
        udp_sock = init_udpserver(DEFAULT_PORT);
        set_nonblocking(udp_sock);

        // TCPサーバソケットの初期化
        int tcp_sock = init_tcpserver(DEFAULT_PORT, MAX_CLIENTS);
        set_nonblocking(tcp_sock);

        handle_server(udp_sock, tcp_sock, clients, username);
    }

    return 0;
}
