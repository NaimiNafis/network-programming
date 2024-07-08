#include "mynet.h"
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFSIZE 512
#define MAX_CLIENTS 10

typedef struct {
    int sock;
    char username[16];
} ClientInfo;

void set_nonblocking(int sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}

int main(int argc, char *argv[]) {
    struct sockaddr_in from_adrs, client_adrs;
    socklen_t from_len;
    int udp_sock, tcp_sock, client_sock;
    int max_sd, activity, strsize, i;
    char buf[BUFSIZE];
    char message[BUFSIZE + 16];
    fd_set readfds;
    ClientInfo clients[MAX_CLIENTS];

    in_port_t port_number = (argc == 2) ? (in_port_t)atoi(argv[1]) : 50001;

    // Initialize the UDP server socket
    udp_sock = init_udpserver(port_number);
    set_nonblocking(udp_sock);

    // Initialize the TCP server socket
    tcp_sock = init_tcpserver(port_number, MAX_CLIENTS);
    set_nonblocking(tcp_sock);

    // Initialize clients array
    for (i = 0; i < MAX_CLIENTS; i++) {
        clients[i].sock = 0;
    }

    printf("Server is ready on port %d.\n", port_number);

    while (1) {
        // Clear the socket set
        FD_ZERO(&readfds);

        // Add UDP socket to set
        FD_SET(udp_sock, &readfds);
        max_sd = udp_sock;

        // Add TCP socket to set
        FD_SET(tcp_sock, &readfds);
        if (tcp_sock > max_sd) {
            max_sd = tcp_sock;
        }

        // Add client sockets to set
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].sock > 0) {
                FD_SET(clients[i].sock, &readfds);
                if (clients[i].sock > max_sd) {
                    max_sd = clients[i].sock;
                }
            }
        }

        // Wait for activity on one of the sockets
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if (activity < 0) {
            exit_errmesg("select()");
        }

        // Check for incoming "HELO" packet on UDP socket
        if (FD_ISSET(udp_sock, &readfds)) {
            from_len = sizeof(from_adrs);
            strsize = Recvfrom(udp_sock, buf, BUFSIZE, 0, (struct sockaddr *)&from_adrs, &from_len);
            buf[strsize] = '\0';
            if (strcmp(buf, "HELO") == 0) {
                Sendto(udp_sock, "HERE", 5, 0, (struct sockaddr *)&from_adrs, from_len);
            }
        }

        // Check for incoming TCP connection
        if (FD_ISSET(tcp_sock, &readfds)) {
            from_len = sizeof(client_adrs);
            if ((client_sock = accept(tcp_sock, (struct sockaddr *)&client_adrs, &from_len)) < 0) {
                exit_errmesg("accept()");
            }

            // Add new client to clients array
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].sock == 0) {
                    clients[i].sock = client_sock;
                    printf("New connection, socket fd is %d, ip is : %s, port : %d\n",
                           client_sock, inet_ntoa(client_adrs.sin_addr), ntohs(client_adrs.sin_port));
                    break;
                }
            }
        }

        // Check for I/O operations on client sockets
        for (i = 0; i < MAX_CLIENTS; i++) {
            int sockfd = clients[i].sock;

            if (FD_ISSET(sockfd, &readfds)) {
                if ((strsize = read(sockfd, buf, BUFSIZE)) == 0) {
                    // Client disconnected
                    getpeername(sockfd, (struct sockaddr *)&client_adrs, (socklen_t *)&from_len);
                    printf("Host disconnected, ip %s, port %d\n",
                           inet_ntoa(client_adrs.sin_addr), ntohs(client_adrs.sin_port));
                    close(sockfd);
                    clients[i].sock = 0;
                } else {
                    buf[strsize] = '\0';
                    if (strncmp(buf, "JOIN ", 5) == 0) {
                        strncpy(clients[i].username, buf + 5, 15);
                        clients[i].username[15] = '\0';
                        printf("%s joined.\n", clients[i].username);
                    } else if (strncmp(buf, "POST ", 5) == 0) {
                        snprintf(message, sizeof(message), "[%s] %s", clients[i].username, buf + 5);
                        printf("%s", message);
                        // Broadcast message to all clients
                        for (int j = 0; j < MAX_CLIENTS; j++) {
                            if (clients[j].sock > 0) {
                                send(clients[j].sock, message, strlen(message), 0);
                            }
                        }
                    } else if (strcmp(buf, "QUIT") == 0) {
                        // Client sent QUIT message
                        printf("%s quit.\n", clients[i].username);
                        close(sockfd);
                        clients[i].sock = 0;
                    }
                }
            }
        }
    }

    close(udp_sock);
    close(tcp_sock);
    return 0;
}
