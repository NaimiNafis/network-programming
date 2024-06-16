#include "mynet.h"
#define BUFSIZE 200   /* Buffer size for receiving set to 200 bytes */

int main(int argc, char *argv[])
{
    struct sockaddr_in from_adrs;
    socklen_t from_len;
    int sock;
    char buf[BUFSIZE];  // Buffer for receiving messages
    int strsize;

    /* Check command line arguments */
    if (argc != 2) {
        fprintf(stderr,"Usage: %s Port_number\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Initialize UDP server */
    sock = init_udpserver((in_port_t)atoi(argv[1]));

    for (;;) {
        from_len = sizeof(from_adrs);

        /* Receive a string from the client */
        strsize = recvfrom(sock, buf, BUFSIZE, 0, (struct sockaddr *)&from_adrs, &from_len);

        if (strsize == -1) {
            perror("recvfrom()");
            exit(EXIT_FAILURE);
        }

        buf[strsize] = '\0';  // Null-terminate the received string
        printf("Received %d bytes: %s\n", strsize, buf);

        /* Echo the string back to the client */
        sendto(sock, buf, strsize, 0, (struct sockaddr *)&from_adrs, sizeof(from_adrs));
    }

    close(sock);             /* Close the socket */

    exit(EXIT_SUCCESS);
}
