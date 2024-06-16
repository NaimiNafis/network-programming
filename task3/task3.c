/*

    --- コンパイルコマンド ---
    gcc -I../mynet -L../mynet -o task3 task3.o -lmynet
    または、makefileを使用して make コマンドによるコンパイル

    --- 実行例１(fork) ---

    サーバーコマンド:
    ./task3 50000 0 5

    クライアントコマンド:
    telnet localhost 50000

    実行結果:
    Client is accepted [pid = 84610, thread_id = 0]
    Client is accepted [pid = 84611, thread_id = 0]
    Client is accepted [pid = 84612, thread_id = 0]
    Client is accepted [pid = 84613, thread_id = 0]
    Client is accepted [pid = 84614, thread_id = 0]

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

    --- 実行例3 (argument error) ---

    サーバーコマンド:
    ./task3

    実行結果:
    Usage: ./task3 <port_number> <parallel_type> <connection_limit>

    Options:
    <port_number>       Specifies the port number the server will listen on.
                        This should be a value between 1024 and 65535.
    <parallel_type>     Indicates the type of parallel processing to use:
                        0 - Process-based parallelism (using fork)
                        1 - Thread-based parallelism (using pthreads)
    <connection_limit>  The maximum number of concurrent connections
                        that the server will handle at one time.

    Example:
    ./task3 8080 1 5     # Starts the server on port 8080 with thread-based parallelism
                        # and suggested to test between 5~10 concurrent connections.

*/

#include "mynet.h"
#include <sys/wait.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFSIZE 50

struct thread_args {
    int sock;
    int thread_id;
};

void echo(int sock_listen);
void *echo_thread(void *arg);
void clean_exit(char *message);
void signal_handler(int sig);
void print_usage(char *program_name);

int sock_listen;
int thread_id = 0;

int main(int argc, char *argv[]) {
    int port_number;
    int parallel_type;
    int connection_limit;
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

    sock_listen = init_tcpserver(port_number,5 );
    if (sock_listen < 0) {
        clean_exit("Failed to initialize TCP server");
    }

    if (parallel_type == 0) {
        for (i = 0; i < connection_limit; i++) {
            child = fork();
            if (child < 0) {
                clean_exit("Fork failed");
            } else if (child == 0) {
                // Child process
                echo(sock_listen);
                exit(EXIT_SUCCESS);
            }
        }
        // Parent process
        close(sock_listen);
        while (wait(NULL) > 0);
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
        }
        pthread_exit(NULL);
    }

    return 0;
}

void *echo_thread(void *arg) {
    struct thread_args *args = (struct thread_args *) arg;
    int sock_listen = args->sock;
    free(arg);

    pthread_detach(pthread_self());

    echo(sock_listen);

    return NULL;
}

void echo(int sock_listen) {
    int sock_accepted;
    char buf[BUFSIZE];
    int strsize;
    int this_thread_id = thread_id++;

    for (;;) {
        sock_accepted = accept(sock_listen, NULL, NULL);
        if (sock_accepted < 0) {
            perror("Accept failed");
            continue;
        }
        printf("Client is accepted [pid = %d, thread_id = %d]\n", getpid(), this_thread_id);
        do {
            if ((strsize = recv(sock_accepted, buf, BUFSIZE, 0)) == -1) {
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
    close(sock_listen);
    exit(EXIT_SUCCESS);
}





