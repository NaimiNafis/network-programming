/*

    --- コンパイルコマンド ---
    gcc task3.c libmynet.a -o task3


    --- 実行例１(fork) ---

    サーバーコマンド:
    ./task3 50000 0 5

    クライアントコマンド:
    telnet localhost 50000

    実行結果:
    Client is accepted [pid = 64825, thread_id = 0]
    Client is accepted [pid = 64826, thread_id = 1]
    Client is accepted [pid = 64827, thread_id = 2]
    Client is accepted [pid = 64828, thread_id = 3]
    Client is accepted [pid = 64829, thread_id = 4]

    --- 実行例２ (スレッド) ---

    サーバーコマンド:
    ./task3 12345 1 5

    クライアントコマンド:
    telnet localhost 12345

    実行結果:
    Client is accepted [pid = 72423, thread_id = 0]
    Client is accepted [pid = 72423, thread_id = 1]
    Client is accepted [pid = 72423, thread_id = 2]
    Client is accepted [pid = 72423, thread_id = 3]
    Client is accepted [pid = 72423, thread_id = 4]

*/

#include "mynet.h"
#include <sys/wait.h>
#include <pthread.h>
#include <signal.h>

#define BUFSIZE 50

struct thread_args {
    int sock;
    int thread_id;
};

void echo(int sock_listen, int thread_id);
void *echo_thread(void *arg);
void clean_exit(char *message);
void signal_handler(int sig);
void print_usage(char *program_name);

int main(int argc, char *argv[]) {
    int port_number;
    int parallel_type;
    int connection_limit;
    int sock_listen;
    pid_t child;
    struct thread_args *args;
    pthread_t tid;
    int i;

    if (argc != 4) {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, signal_handler);

    port_number = atoi(argv[1]);
    parallel_type = atoi(argv[2]);
    connection_limit = atoi(argv[3]);

    sock_listen = init_tcpserver(port_number, 5);
    if (sock_listen < 0) {
        clean_exit("Failed to initialize TCP server");
    }

    if (parallel_type == 0) {
        for (i = 0; i < connection_limit; i++) {
            child = fork();
            if (child < 0) {
                clean_exit("Fork failed");
                continue;
            } else if (child == 0) {
                echo(sock_listen, i);
                close(sock_listen);
                exit(EXIT_SUCCESS);
            }
        }
    } else if (parallel_type == 1) {
        for (i = 0; i < connection_limit; i++) {
            args = (struct thread_args *) malloc(sizeof(struct thread_args));
            if (args == NULL) {
                clean_exit("Memory allocation failed");
            }
            args->sock = sock_listen;
            args->thread_id = i;

            if (pthread_create(&tid, NULL, echo_thread, (void *) args) != 0) {
                clean_exit("Thread creation failed");
            }
            pthread_detach(tid);
        }
    }

    for (;;) pause();

    return 0;
}

void *echo_thread(void *arg) {
    struct thread_args *args = (struct thread_args *) arg;
    int sock_listen = args->sock;
    int thread_id = args->thread_id;
    free(arg);

    echo(sock_listen, thread_id);

    return NULL;
}

void echo(int sock_listen, int thread_id) {
    int sock_accepted;
    char buf[BUFSIZE];
    int strsize;

    while (1) {
        sock_accepted = accept(sock_listen, NULL, NULL);
        if (sock_accepted < 0) {
            perror("Accept failed");
            continue;
        }
        printf("Client is accepted [pid = %d, thread_id = %d]\n", getpid(), thread_id);
        do {
            strsize = recv(sock_accepted, buf, BUFSIZE, 0);
            if (strsize == -1) {
                perror("Receive failed");
                break;
            }
            if (send(sock_accepted, buf, strsize, 0) == -1) {
                perror("Send failed");
                break;
            }
        } while (buf[strsize - 1] != '\n');
        close(sock_accepted);
    }
}

void print_usage(char *program_name) {
    fprintf(stderr, "\nUsage: %s <port_number> <parallel_type> <connection_limit>\n\n"
                "Options:\n"
                "  <port_number>       Specifies the port number the server will listen on.\n"
                "                      This should be a value between 1024 and 65535.\n"
                "  <parallel_type>     Indicates the type of parallel processing to use:\n"
                "                      0 - Process-based parallelism (using fork)\n"
                "                      1 - Thread-based parallelism (using pthreads)\n"
                "  <connection_limit>  The maximum number of concurrent connections\n"
                "                      that the server will handle at one time.\n\n"
                "Example:\n"
                "  %s 8080 1 5     # Starts the server on port 8080 with thread-based parallelism\n"
                "                       # and suggested to test between 5~10 concurrent connections.\n\n",
                program_name, program_name);


}

void clean_exit(char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

void signal_handler(int sig) {
    printf("Received signal %d, shutting down...\n", sig);
    exit(EXIT_SUCCESS);
}
