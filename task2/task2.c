/*

    --- コンパイルコマンド ---
    gcc -I../mynet -L../mynet -o task2 task2.o -lmynet
    または、makefileを使用して make コマンドによるコンパイル

    --- 実行例１ ---

    サーバーコマンド:
    ./task2 50000

    クライアントコマンド:
    telnet localhost 50000

    実行結果:
    Trying 127.0.0.1...
    Connected to localhost.
    Escape character is '^]'.
    password: wrong_password
    Sorry, password is incorrect.
    Connection closed by foreign host.

    --- 実行例２ ---

    サーバーコマンド:
    ./task2 50000

    クライアントコマンド:
    telnet localhost 50000

    実行結果:
    Trying 127.0.0.1...
    Connected to localhost.
    Escape character is '^]'.
    password: password
    Welcome, authorized personnel.
    > list
    aaa.txt
    bbb.txt
    ccc.txt
    > type aaa.txt
    This file is aaa.txt.
    Thank you!
    > type bbb.txt
    This file is bbb.txt.
    Thank you!
    > type ccc.txt
    This file is ccc.txt.
    Thank you!
    > type ../ddd.txt
    Sorry, using `..` as parent directory is not allowed.
    > exit
    Connection closed by foreign host.

*/

#include "mynet.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFSIZE 1024
#define PASSWORD "password"

void handle_client(int sock);

int main(int argc, char const *argv[]) {
    in_port_t port;
    if (argc == 1) {
        port = 50000;
    } else {
        port = atoi(argv[1]);
    }

    int sock_listen = init_tcpserver(port, 10);

    while (1) {
        int sock = accept(sock_listen, NULL, NULL);
        if (sock == -1) {
            perror("accept");
            continue;
        }

        handle_client(sock);
        close(sock);
    }

    return 0;
}

void handle_client(int sock) {
    char buf[BUFSIZE];
    int strsize;
    FILE *fp;

    // Ask for the password
    sprintf(buf, "password: ");
    send(sock, buf, strlen(buf), 0);

    // Receive the password
    if ((strsize = recv(sock, buf, BUFSIZE, 0)) == -1) {
        perror("recv");
        return;
    }

    buf[strsize] = '\0';

    if (strncmp(buf, PASSWORD, strlen(PASSWORD)) != 0) {
        sprintf(buf, "Sorry, password is incorrect.\r\n");
        send(sock, buf, strlen(buf), 0);
        return;
    }

    sprintf(buf, "Welcome, authorized personnel.\r\n");
    send(sock, buf, strlen(buf), 0);

    while (1) {
        // Prompt
        send(sock, "> ", 2, 0);

        // Receive command
        if ((strsize = recv(sock, buf, BUFSIZE, 0)) == -1) {
            perror("recv");
            return;
        }

        buf[strsize] = '\0';

        if (strncmp(buf, "list", 4) == 0) {
            // List all .txt files in ~/work/ directory
            fp = popen("ls ~/work/*.txt 2>/dev/null | xargs -n 1 basename", "r");
            if (fp == NULL) {
                sprintf(buf, "Directory not found.\r\n");
                send(sock, buf, strlen(buf), 0);
                continue;
            }

            while (fgets(buf, BUFSIZE, fp) != NULL) {
                send(sock, buf, strlen(buf), 0);
            }

            if (pclose(fp) == -1) {
                sprintf(buf, "Failed to close command stream.\r\n");
                send(sock, buf, strlen(buf), 0);
            }
        } else if (strncmp(buf, "type", 4) == 0) {
            char *cur = buf + 4;
            cur += strspn(cur, " ");
            int size = strcspn(cur, " \r\n");
            cur[size] = '\0';

            // Prevent directory traversal
            if (strstr(cur, "..") != NULL) {
                sprintf(buf, "Sorry, using `..` as parent directory is not allowed.\r\n");
                send(sock, buf, strlen(buf), 0);
                continue;
            }

            char command[256];
            sprintf(command, "cat ~/work/%s 2>/dev/null", cur);
            fp = popen(command, "r");
            if (fp == NULL) {
                sprintf(buf, "No such file.\r\n");
                send(sock, buf, strlen(buf), 0);
                continue;
            }

            while (fgets(buf, BUFSIZE, fp) != NULL) {
                send(sock, buf, strlen(buf), 0);
            }

            if (pclose(fp) == -1) {
                sprintf(buf, "Failed to close command stream.\r\n");
                send(sock, buf, strlen(buf), 0);
            }

            // Add a new line after displaying the file content
            sprintf(buf, "\r\n");
            send(sock, buf, strlen(buf), 0);
        } else if (strncmp(buf, "exit", 4) == 0) {
            break;
        } else {
            sprintf(buf, "Command not found.\r\n");
            send(sock, buf, strlen(buf), 0);
        }
    }
}




