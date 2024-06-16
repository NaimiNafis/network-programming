#include "mynet.h"
#include "quiz.h"
#include <stdlib.h>
#include <sys/select.h>

#define NAMELENGTH 20 /* ログイン名の長さ制限 */
#define BUFLEN 500 /* 通信バッファサイズ */

/* 各クライアントのユーザ情報を格納する構造体の定義 */
typedef struct {
    int sock;
    char name[NAMELENGTH];
} client_info;

/* プライベート変数 */
static int N_client; /* クライアントの数 */
static client_info *Client; /* クライアントの情報 */
static int Max_sd; /* ディスクリプタ最大値 */
static int *Ranking; /* 順位の保存用 */
static char Buf[BUFLEN]; /* 通信用バッファ */

/* プライベート関数 */
static int client_login(int sock_listen);
static void send_question(char *question);
static void receive_answer();
static void send_result();
static char *chop_nl(char *s);

void init_client(int sock_listen, int n_client)
{
    N_client = n_client;

    /* クライアント情報の保存用構造体の初期化 */
    if ((Client = (client_info *) malloc(N_client * sizeof(client_info))) == NULL) {
        exit_errmesg("malloc()");
    }

    if ((Ranking = (int *) malloc(N_client * sizeof(int))) == NULL) {
        exit_errmesg("malloc()");
    }

    /* クライアントのログイン処理 */
    Max_sd = client_login(sock_listen);
}

void message_loop()
{
    char *question;

    for (;;) {
        /* 問題文の作成 */
        question = make_question();

        /* 問題の送信 */
        send_question(question);

        /* 解答の受信 */
        receive_answer();

        /* 結果の表示 */
        send_result();
    }
}

static int client_login(int sock_listen)
{
    int client_id, sock_accepted;
    static char prompt[] = "Input your name: ";
    char loginname[NAMELENGTH];
    int strsize;

    for (client_id = 0; client_id < N_client; client_id++) {
        /* クライアントの接続を受け付ける */
        sock_accepted = Accept(sock_listen, NULL, NULL);
        printf("Client[%d] connected.\n", client_id);

        /* ログインプロンプトを送信 */
        Send(sock_accepted, prompt, strlen(prompt), 0);

        /* ログイン名を受信 */
        strsize = Recv(sock_accepted, loginname, NAMELENGTH - 1, 0);
        loginname[strsize] = '\0';
        chop_nl(loginname);

        /* ユーザ情報を保存 */
        Client[client_id].sock = sock_accepted;
        strncpy(Client[client_id].name, loginname, NAMELENGTH);
    }

    return (sock_accepted);
}

static void send_question(char *question)
{
    int client_id;

    for (client_id = 0; client_id < N_client; client_id++) {
        Send(Client[client_id].sock, question, strlen(question), 0);
    }
}

static void receive_answer()
{
    fd_set mask, readfds;
    int client_id;
    int answered;
    int strsize;
    static char right_ans[] = "Your answer is right!\n";
    static char wrong_ans[] = "Your answer is wrong. Answer again.\n";

    /* ビットマスクの準備 */
    FD_ZERO(&mask);
    for (client_id = 0; client_id < N_client; client_id++) {
        FD_SET(Client[client_id].sock, &mask);
    }

    answered = 0;
    while (answered < N_client) {

        /* 受信データの有無をチェック */
        readfds = mask;
        select(Max_sd + 1, &readfds, NULL, NULL, NULL);

        for (client_id = 0; client_id < N_client; client_id++) {

            if (FD_ISSET(Client[client_id].sock, &readfds)) {

                strsize = Recv(Client[client_id].sock, Buf, BUFLEN - 1, 0);
                Buf[strsize] = '\0';

                if (check_answer(Buf)) { /* 解答が正しければ */
                    Ranking[answered] = client_id;
                    Send(Client[client_id].sock, right_ans, strlen(right_ans), 0);
                    answered++;
                } else {
                    Send(Client[client_id].sock, wrong_ans, strlen(wrong_ans), 0);
                }
            }
        }
    }
}

static void send_result()
{
    int rank, client_id;
    int len;

    for (rank = 0; rank < N_client; rank++) {
        /* 順位を表す文字列を作成 */
        len = snprintf(Buf, BUFLEN, "[%d]\t%s\n", rank + 1, Client[Ranking[rank]].name);

        /* 順位データを送信する */
        for (client_id = 0; client_id < N_client; client_id++) {
            Send(Client[client_id].sock, Buf, len, 0);
        }
    }
}

static char *chop_nl(char *s)
{
    int len;
    len = strlen(s);
    if (s[len - 1] == '\n') {
        s[len - 1] = '\0';
    }
    return (s);
}
