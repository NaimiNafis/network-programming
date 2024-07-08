#include "mynet.h"
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>

#define BUFSIZE 512
#define TIMEOUT_SEC 10
#define MAX_RETRIES 3

int main(int argc, char *argv[]) {
    struct sockaddr_in broadcast_adrs, from_adrs;
    socklen_t from_len;
    int udp_sock, tcp_sock, broadcast_sw = 1, strsize, retries = 0;
    char buf[BUFSIZE], username[16], server_ip[20];
    fd_set mask, readfds;
    struct timeval timeout;

    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: %s username [port_number]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    strncpy(username, argv[1], 15);
    username[15] = '\0';
    in_port_t port_number = (argc == 3) ? (in_port_t)atoi(argv[2]) : 50001;

    // Initialize the UDP client socket
    udp_sock = init_udpclient();

    // Enable broadcast on the UDP socket
    if (setsockopt(udp_sock, SOL_SOCKET, SO_BROADCAST, &broadcast_sw, sizeof(broadcast_sw)) == -1) {
        exit_errmesg("setsockopt()");
    }

    // Set up broadcast address
    set_sockaddr_in_broadcast(&broadcast_adrs, port_number);

    // Send HELO packets and wait for HERE response
    for (retries = 0; retries < MAX_RETRIES; retries++) {
        Sendto(udp_sock, "HELO", 5, 0, (struct sockaddr *)&broadcast_adrs, sizeof(broadcast_adrs));
        
        FD_ZERO(&mask);
        FD_SET(udp_sock, &mask);
        readfds = mask;
        timeout.tv_sec = TIMEOUT_SEC;
        timeout.tv_usec = 0;

        if (select(udp_sock + 1, &readfds, NULL, NULL, &timeout) > 0) {
            from_len = sizeof(from_adrs);
            strsize = Recvfrom(udp_sock, buf, BUFSIZE, 0, (struct sockaddr *)&from_adrs, &from_len);
            buf[strsize] = '\0';
            if (strcmp(buf, "HERE") == 0) {
                strncpy(server_ip, inet_ntoa(from_adrs.sin_addr), sizeof(server_ip));
                close(udp_sock);
                break;
            }
        }
    }

    if (retries == MAX_RETRIES) {
        fprintf(stderr, "No response from server. Exiting.\n");
        exit(EXIT_FAILURE);
    }

    // Initialize the TCP client socket and connect to the server
    tcp_sock = init_tcpclient(server_ip, port_number);

    // Send JOIN message
    snprintf(buf, sizeof(buf), "JOIN %s", username);
    send(tcp_sock, buf, strlen(buf), 0);

    // Main loop: receive messages from server and send user input to server
    for (;;) {
        FD_ZERO(&mask);
        FD_SET(tcp_sock, &mask);
        FD_SET(fileno(stdin), &mask);

        readfds = mask;
        if (select(tcp_sock + 1, &readfds, NULL, NULL, NULL) > 0) {
            if (FD_ISSET(tcp_sock, &readfds)) {
                if ((strsize = recv(tcp_sock, buf, BUFSIZE - 1, 0)) <= 0) {
                    break;
                }
                buf[strsize] = '\0';
                printf("%s\n", buf);
            }
            if (FD_ISSET(fileno(stdin), &readfds)) {
                fgets(buf, BUFSIZE, stdin);
                strsize = strlen(buf);
                if (strncmp(buf, "QUIT", 4) == 0) {
                    send(tcp_sock, "QUIT", 4, 0);
                    break;
                } else {
                    snprintf(buf, BUFSIZE, "POST %s", buf);
                    send(tcp_sock, buf, strsize + 5, 0); // 5 is length of "POST "
                }
            }
        }
    }

    close(tcp_sock);
    return 0;
}
