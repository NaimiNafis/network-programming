#include "mynet.h"
#include "quiz.h"
#include <stdlib.h>
#include <unistd.h>

void quiz_server(int port_number, int n_client)
{
    int sock_listen;

    /* サーバの初期化 */
    sock_listen = init_tcpserver(port_number, 10);

    /* クライアントの接続 */
    init_client(sock_listen, n_client);

    close(sock_listen);

    /* メインループ */
    message_loop();
}
