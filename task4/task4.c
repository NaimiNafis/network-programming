/*

    --------------------------------------------------
    
    コンパイルコマンド:
    gcc -I../mynet -L../mynet -o task4 task4.o -lmynet
    または、makefileを使用して make コマンドによるコンパイル

    サーバーコマンド:
    ./task4 -S -p 12345 -c 3

    クライアントコマンド:
    ./task4 -C -s localhost -p 12345

    --------------------------------------------------

    この課題４はスレッドを使用してN人でのチャットを実現する。
    オプション指定に関してははquizのサンプルプログラムから流用している。

    工夫した点:
    - スレッドを使用して、複数のクライアントが同時に接続できるようにした。
    - サーバーが起動中で、チャットルームが満員でない場合は自由に出入りできるようにした。
    - クライアントの入退室を他のクライアントに通知するようにした。

    苦労した点:
    - 新規クライアントの最初のメッセージが即座に表示されるようにすること。
    - ログアウト時のメッセージ通知の処理。
    - サーバーがダウンした際のクライアントの処理。
 */

#include <mynet.h>
#include <pthread.h>
#include <sys/select.h>

#define SERVER_LEN 256 /* サーバ名格納用バッファサイズ */
#define DEFAULT_PORT 12345 /* ポート番号既定値 */
#define DEFAULT_NCLIENT 3 /* 省略時のクライアント数 */
#define DEFAULT_MODE 'C' /* 省略時はクライアント */

#define NAMELENGTH 20 /* 名前の最大長 */
#define BUFLEN 1000 /* 通信バッファサイズ */

#define INVALID -1
#define LOGGED_OUT 0
#define LOGGED_IN 1

extern char *optarg;
extern int optind, opterr, optopt;

/* 各クライアントのユーザ情報を格納する構造体の定義 */
typedef struct {
    int sock;
    char name[NAMELENGTH];
    int state;
    pthread_t thread;
} client_info;

/* プライベート変数 */
static int N_client; /* クライアントの数 */
static client_info *Client; /* クライアントの情報 */
static int sock_listen; /* リスニングソケット */
static fd_set master_fds; /* マスターファイルディスクリプタセット */
static int max_sd; /* 最大ファイルディスクリプタ値 */

/* プライベート関数 */
static void broadcast(int sender_sock, const char *message);
static void handle_logout(int client_index);

/**
 * ソケット操作のラッパー関数である。処理に失敗した場合、exit()を呼び出してプログラム全体を終了させる。
 */
int Accept(int s, struct sockaddr *addr, socklen_t *addrlen) {
    int r;
    if ((r = accept(s, addr, addrlen)) == -1) {
        exit_errmesg("accept()");
    }
    return (r);
}

int Send(int s, void *buf, size_t len, int flags) {
    int r;
    if ((r = send(s, buf, len, flags)) == -1) {
        exit_errmesg("send()");
    }
    return (r);
}

int Recv(int s, void *buf, size_t len, int flags) {
    int r;
    if ((r = recv(s, buf, len, flags)) == -1) {
        exit_errmesg("recv()");
    }
    return r;
}

/**
 * 新しいクライアントのログインを処理する関数である。
 * 新しいクライアントが接続すると、その名前を受信し、クライアントリストに追加する。
 */
static void *client_login(void *arg) 
{
    int client_sock = *(int *)arg;
    free(arg);
    char name[NAMELENGTH];

    // クライアントの名前を受信する
    int len = recv(client_sock, name, NAMELENGTH - 1, 0);
    if (len > 0) {
        name[len] = '\0';
    } else {
        printf("Failed to receive client name, closing socket %d\n", client_sock);
        close(client_sock);
        return NULL;
    }

    for (int i = 0; i < N_client; i++) {
        if (Client[i].state == LOGGED_OUT) {
            Client[i].sock = client_sock;
            snprintf(Client[i].name, NAMELENGTH, "%s", name);
            Client[i].state = LOGGED_IN;
            FD_SET(client_sock, &master_fds);
            if (client_sock > max_sd) {
                max_sd = client_sock;
            }
            printf("Client %s connected on socket %d\n", name, client_sock);
            char enter_msg[BUFLEN];
            snprintf(enter_msg, BUFLEN, "Client %s has entered the chat.\n", name);
            broadcast(client_sock, enter_msg);
            return NULL;
        }
    }

    // 空きスロットがない場合
    char *message = "Sorry. The Server is currently full at the moment.\n";
    send(client_sock, message, strlen(message), 0);
    close(client_sock);
    return NULL;
}

/**
 * クライアント情報を初期化する関数である。
 * 指定されたクライアント数分のメモリを確保し、初期状態に設定する。
 */
static void init_client(int n_client) {
    N_client = n_client;
    if ((Client = (client_info *)malloc(N_client * sizeof(client_info))) == NULL) {
        exit_errmesg("malloc()");
    }
    for (int i = 0; i < N_client; i++) {
        Client[i].state = LOGGED_OUT;
        Client[i].sock = INVALID;
    }
}

/**
 * すべてのクライアントにメッセージを送信する関数である。
 */
static void broadcast(int sender_sock, const char *message) {
    for (int i = 0; i < N_client; i++) {
        if (Client[i].state == LOGGED_IN && Client[i].sock != sender_sock) {
            Send(Client[i].sock, (void *)message, strlen(message), 0);
        }
    }
}

/**
 * クライアントのログアウトを処理し、他のクライアントに通知する関数である。
 */
static void handle_logout(int client_index) {
    char message[BUFLEN];
    snprintf(message, BUFLEN, "\nClient %s has logged out.\n", Client[client_index].name);
    broadcast(Client[client_index].sock, message);
    close(Client[client_index].sock);
    FD_CLR(Client[client_index].sock, &master_fds);
    Client[client_index].sock = INVALID;
    Client[client_index].state = LOGGED_OUT;
    printf("Client %s disconnected.\n", Client[client_index].name);
}

/**
 * サーバのメインループを実行する関数である。
 * 新しいクライアントの接続を受け入れ、既存のクライアントからのメッセージを処理する。
 */
static void server_loop() {
    fd_set read_fds;
    int activity;

    while (1) {
        read_fds = master_fds;

        activity = select(max_sd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0) {
            exit_errmesg("select()");
        }

        if (FD_ISSET(sock_listen, &read_fds)) {
            int *new_sock = malloc(sizeof(int));
            *new_sock = Accept(sock_listen, NULL, NULL);
            pthread_t tid;
            pthread_create(&tid, NULL, client_login, (void *)new_sock);
        }

        for (int i = 0; i < N_client; i++) {
            int sd = Client[i].sock;
            if (sd != INVALID && FD_ISSET(sd, &read_fds)) {
                char buf[BUFLEN];
                int len = recv(sd, buf, BUFLEN - 1, 0);
                if (len <= 0) {
                    handle_logout(i);
                } else {
                    buf[len] = '\0';
                    printf("Received from %s: %s\n", Client[i].name, buf);
                    char message[BUFLEN];
                    snprintf(message, BUFLEN, "%s: %s", Client[i].name, buf);
                    broadcast(sd, message);
                }
            }
        }
    }
}

/**
 * サーバを開始する関数である。指定されたポート番号でサーバを初期化し、
 * クライアント接続を待ち受ける。
 */
void chat_server(int port_number, int n_client) {
    sock_listen = init_tcpserver(port_number, 10);
    init_client(n_client);

    FD_ZERO(&master_fds);
    FD_SET(sock_listen, &master_fds);
    max_sd = sock_listen;

    server_loop();

    close(sock_listen);
}

/**
 * クライアントを動作させる関数である。標準入力のメッセージをサーバへ送信し、
 * サーバから受信したメッセージを標準出力に表示する。
 * サーバが接続を遮断した場合、プログラムを終了させる。
 */
void chat_client(char *servername, int port_number) 
{
    int sock;
    char s_buf[BUFLEN], r_buf[BUFLEN], confirm[3];
    int strsize;
    fd_set mask, readfds;

    /* サーバに接続する */
    sock = init_tcpclient(servername, port_number);
    printf("Connected.\n");

    // ユーザ名を入力させる
    printf("Enter your name: ");
    fgets(s_buf, NAMELENGTH, stdin);
    s_buf[strcspn(s_buf, "\n")] = 0; // 改行文字を削除

    // ユーザ名をサーバに送信する
    Send(sock, s_buf, strlen(s_buf), 0);

    // ウェルカムメッセージ
    printf("Welcome to the room %s! Wait for others to enter the room.\n\n", s_buf);

    /* ビットマスクの準備 */
    FD_ZERO(&mask);
    FD_SET(0, &mask);
    FD_SET(sock, &mask);

    while (1) {

        /* 受信データの有無をチェック */
        readfds = mask;
        select(sock + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(sock, &readfds)) {
            /* サーバから文字列を受信する */
            strsize = Recv(sock, r_buf, BUFLEN - 1, 0);
            if (strsize <= 0) {
                printf("\nServer has closed the connection. Exiting...\n");
                close(sock);
                exit(EXIT_FAILURE);
            }
            r_buf[strsize] = '\0';
            printf("%s\n", r_buf);
            fflush(stdout);
        }

        if (FD_ISSET(0, &readfds)) {
            /* キーボードから文字列を入力する */
            fgets(s_buf, BUFLEN, stdin);
            s_buf[strcspn(s_buf, "\n")] = 0;

            if (strcmp(s_buf, "logout") == 0) {
                while (1) {
                    printf("Are you sure you want to logout? (y/n): ");
                    fgets(confirm, sizeof(confirm), stdin);
                    confirm[strcspn(confirm, "\n")] = 0;

                    if (strcmp(confirm, "y") == 0) {
                        Send(sock, "logout", strlen("logout"), 0);
                        close(sock);
                        printf("Disconnected.\n");
                        exit(EXIT_SUCCESS);
                    } else if (strcmp(confirm, "n") == 0) {
                        printf("Logout canceled.\n");
                        break;
                    } else {
                        printf("Invalid input. Please type 'y' or 'n'.\n");
                    }
                }
                continue;
            }

            strsize = strlen(s_buf);
            Send(sock, s_buf, strsize, 0);
            fflush(stdout);
        }
    }
}

/**
 * プログラムのエントリーポイントである。コマンドライン引数を解析し、
 * サーバモードまたはクライアントモードを設定して実行する。
 */
int main(int argc, char *argv[])
{
    int port_number = DEFAULT_PORT;
    int num_client = DEFAULT_NCLIENT;
    char servername[SERVER_LEN] = "localhost";
    char mode = DEFAULT_MODE;
    int c;

    /* オプション文字列の取得 */
    opterr = 0;
    while (1) {
        c = getopt(argc, argv, "SCs:p:c:h");
        if (c == -1)
            break;

        switch (c) {
        case 'S': /* サーバモードにする */
            mode = 'S';
            break;

        case 'C': /* クライアントモードにする */
            mode = 'C';
            break;

        case 's': /* サーバ名の指定 */
            snprintf(servername, SERVER_LEN, "%s", optarg);
            break;

        case 'p': /* ポート番号の指定 */
            port_number = atoi(optarg);
            break;

        case 'c': /* クライアントの数 */
            num_client = atoi(optarg);
            break;

        case '?':
            fprintf(stderr, "Unknown option '%c'\n", optopt);
        case 'h':
            fprintf(stderr, "Usage(Server): %s -S -p port_number -c num_client\n", argv[0]);
            fprintf(stderr, "Usage(Client): %s -C -s server_name -p port_number\n", argv[0]);
            exit(EXIT_FAILURE);
            break;
        }
    }

    switch (mode) {
    case 'S':
        chat_server(port_number, num_client);
        break;
    case 'C':
        chat_client(servername, port_number);
        break;
    }

    exit(EXIT_SUCCESS);
}
