#include "mynet.h"
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

#define BUFSIZE 512
#define MAX_CLIENTS 30
#define DEFAULT_PORT 50001
#define BUFFER_SIZE 1024

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

// クライアントからのメッセージを処理する関数
void process_client_message(int sockfd, ClientInfo clients[], int idx, char *buf) {
    printf("Debug: Processing message from client [%d]: '%s'\n", idx, buf);
    char message[BUFSIZE + 50];
    memset(message, 0, sizeof(message));

    if (strncmp(buf, "JOIN ", 5) == 0) {
        strncpy(clients[idx].username, buf + 5, 15);
        clients[idx].username[15] = '\0';
        printf("%s joined the chat.\n", clients[idx].username);
    } else if (strncmp(buf, "POST ", 5) == 0) {
        snprintf(message, sizeof(message), "[%s] %s\n", clients[idx].username, buf + 5);
        printf("Broadcasting message: %s\n", message);
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

// サーバーのメイン関数
void server_main(int port_number) {
    int udp_sock, tcp_sock, client_sock;
    struct sockaddr_in from_adrs, client_adrs;
    socklen_t from_len;
    int max_sd, activity, strsize, i;
    char buf[BUFSIZE];
    fd_set readfds;
    ClientInfo clients[MAX_CLIENTS];

    udp_sock = init_udpserver(port_number);
    tcp_sock = init_tcpserver(port_number, MAX_CLIENTS);
    set_nonblocking(udp_sock);
    set_nonblocking(tcp_sock);

    init_client_array(clients);
    printf("Server is ready on port %d.\n", port_number);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(udp_sock, &readfds);
        FD_SET(tcp_sock, &readfds);
        max_sd = (udp_sock > tcp_sock) ? udp_sock : tcp_sock;

        for (i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].sock > 0) {
                FD_SET(clients[i].sock, &readfds);
                if (clients[i].sock > max_sd) {
                    max_sd = clients[i].sock;
                }
            }
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0 && errno != EINTR) {
            perror("select error");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(udp_sock, &readfds)) {
            from_len = sizeof(from_adrs);
            if ((strsize = recvfrom(udp_sock, buf, BUFSIZE, 0, (struct sockaddr *)&from_adrs, &from_len)) > 0) {
                buf[strsize] = '\0';
                if (strcmp(buf, "HELO") == 0) {
                    sendto(udp_sock, "HERE", 5, 0, (struct sockaddr *)&from_adrs, from_len);
                    printf("Sent HERE in response to HELO from %s:%d\n", inet_ntoa(from_adrs.sin_addr), ntohs(from_adrs.sin_port));
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
                        printf("New connection, socket fd is %d, ip is : %s, port : %d, client index: %d\n",
                               client_sock, inet_ntoa(client_adrs.sin_addr), ntohs(client_adrs.sin_port), i);
                        break;
                    }
                }
                if (i == MAX_CLIENTS) {
                    printf("Max clients reached, rejecting new connection\n");
                    close(client_sock);
                }
            } else {
                perror("accept error");
            }
        }

        for (i = 0; i < MAX_CLIENTS; i++) {
            int sockfd = clients[i].sock;
            if (FD_ISSET(sockfd, &readfds)) {
                if ((strsize = recv(sockfd, buf, BUFSIZE, 0)) <= 0) {
                    if (strsize == 0) {
                        printf("Host disconnected normally, client index: %d\n", i);
                    } else {
                        perror("recv error");
                    }
                    close(sockfd);
                    clients[i].sock = 0;
                    printf("Socket %d closed\n", sockfd);
                } else {
                    buf[strsize] = '\0';
                    process_client_message(sockfd, clients, i, buf);
                }
            }
        }
    }
}

// クライアントのメイン関数
void client_main(const char *server_ip, int port_number) {
    int sock;
    struct sockaddr_in serv_addr;
    char message[BUFFER_SIZE];
    int str_len;

    printf("Creating socket...\n");
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        exit_errmesg("socket() error");
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(server_ip);
    serv_addr.sin_port = htons(port_number);

    printf("Connecting to server...\n");
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        exit_errmesg("connect() error");
    }

    while (1) {
        fputs("Input message (Q to quit): ", stdout);
        fgets(message, BUFFER_SIZE, stdin);

        if (!strcmp(message, "Q\n")) {
            break;
        }

        printf("Sending message to server...\n");
        if (write(sock, message, strlen(message)) == -1) {
            exit_errmesg("write() error");
        }
        str_len = read(sock, message, BUFFER_SIZE - 1);
        if (str_len == -1) {
            exit_errmesg("read() error");
        }

        message[str_len] = '\0';
        printf("Message from server: %s\n", message);
    }

    close(sock);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <server|client> [options]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[1], "server") == 0) {
        int port_number = (argc == 3) ? atoi(argv[2]) : DEFAULT_PORT;
        server_main(port_number);
    } else if (strcmp(argv[1], "client") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s client <server_ip> [port_number]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
        const char *server_ip = argv[2];
        int port_number = (argc == 4) ? atoi(argv[3]) : DEFAULT_PORT;
        client_main(server_ip, port_number);
    } else {
        fprintf(stderr, "Invalid mode: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    return 0;
}
