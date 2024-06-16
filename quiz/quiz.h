#ifndef QUIZ_H_
#define QUIZ_H_
/*
  quiz.h
*/
#include "mynet.h"

/* サーバメインルーチン */
void quiz_server(int port_number, int n_client);

/* クライアントメインルーチン */
void quiz_client(char *servername, int port_number);

/* クライアントの初期化 */
void init_client(int sock_listen, int n_client);

/* メインループ */
void message_loop();

/* 問題文作成関数 */
char *make_question();

/* 解答チェック関数 */
int check_answer(char *answer);

/* Accept関数(エラー処理つき) */
int Accept(int s, struct sockaddr *addr, socklen_t *addrlen);

/* 送信関数(エラー処理つき) */
int Send(int s, void *buf, size_t len, int flags);

/* 受信関数(エラー処理つき) */
int Recv(int s, void *buf, size_t len, int flags);

#endif /* QUIZ_H_ */
