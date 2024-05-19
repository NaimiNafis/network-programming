/*
    --- コンパイルコマンド ---
    gcc task1.c -o task1


    --- 実行例１ ---

    コマンド:
    ./task1 http://www.google.com/

    実行結果:
    Content length
    -> Unknown (`Content-Length` field not found in response header)
    Server software running at `www.google.com`
    -> gws

    --- 実行例２ ---

    コマンド:
    ./task1 http://www.google.com/ invalid-proxy 9999

    実行結果:
    Failed to resolve host: invalid-proxy


    --- 実行例３ ---

    コマンド:
    ./task1 http://www.google.com/404

    実行結果:
    Server returned 404 Not Found; Requested content `http://www.google.com/404` does not exist.

    Content length
    -> 1564
    Server software running at `www.google.com`
    -> Unknown (`Server` field not found in response header)

    --- 実行例４ ---

    コマンド:
    ./task1 http://www.is.kit.ac.jp

    実行結果:
    Content length
    -> 233
    Server software running at `www.is.kit.ac.jp`
    -> Apache

    --- 実行例５ ---

    コマンド:
    ./task1 http://www.is.kit.ac.jp 172.16.61.44 5000

    実行結果:
    Failed to establish connection: Operation timed out

*/

#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFSIZE 1024

ssize_t gethostnamefromurl(char *buf, char *result)
{
    char *begin = strstr(buf, "://");
    if (begin == NULL) return -1;
    begin += 3;

    char *end = strstr(begin, "/");
    if (end == NULL) end = strstr(begin, "#");
    if (end == NULL) end = strstr(begin, "?");
    if (end == NULL) end = begin + strlen(begin);

    ssize_t size = end - begin;
    strncpy(result, begin, size);
    return size;
}

ssize_t get_value_from_response(char *response, char *key, char *result)
{
    char *cur = strstr(response, key);
    if (cur == NULL) return -1;

    cur += strlen(key);
    cur += strspn(cur, " :");
    ssize_t size = strcspn(cur, " \r\n");
    strncpy(result, cur, size);
    result[size] = 0;

    return size;
}

int get_status(char *response)
{
    char *cur = response;
    cur += strcspn(cur, " ");
    cur += strspn(cur, " ");

    char status[4];
    strncpy(status, cur, 3);
    status[3] = 0;
    return atoi(status);
}

int main(int argc, char **argv)
{
    char *content_url = argv[1];
    char content_host_name[256];
    if (gethostnamefromurl(content_url, content_host_name) == -1) {
        fprintf(stderr, "Failed to get host of content. URL may be malformed.");
        exit(EXIT_FAILURE);
    }

    char *server_host_name;
    int server_port;

    int use_proxy = argc > 2;
    if (use_proxy) {
        server_host_name = argv[2];
        server_port = atoi(argv[3]);
    } else {
        server_host_name = content_host_name;
        server_port = 80;
    }

    struct hostent* server_host;
    struct sockaddr_in server_adrs;

    int tcpsock;

    char k_buf[BUFSIZE], s_buf[BUFSIZE], r_buf[BUFSIZE];
    int strsize;

    if ((server_host = gethostbyname(server_host_name)) == NULL) {
        fprintf(stderr, "Failed to resolve host of `%s`. exit.", server_host_name);
        exit(EXIT_FAILURE);
    }

    memset(&server_adrs, 0, sizeof(server_adrs));
    server_adrs.sin_family = AF_INET;
    server_adrs.sin_port = htons(server_port);
    memcpy(&server_adrs.sin_addr, server_host->h_addr_list[0], server_host->h_length);

    if ((tcpsock = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Failed to create socket. exit.\n");
        exit(EXIT_FAILURE);
    }

    if (connect(tcpsock, (struct sockaddr*) &server_adrs, sizeof(server_adrs)) < 0) {
        /* サーバーに接続できなかった場合 */
        fprintf(stderr, "Failed to establish connection with `%s`. exit.\n", server_host_name);
        exit(EXIT_FAILURE);
    }

    s_buf[0] = 0;
    strsize = 0;
    strsize += snprintf(k_buf, BUFSIZE, "GET %s HTTP/1.1\r\n", content_url);
    strncat(s_buf, k_buf, BUFSIZE);
    strsize += snprintf(k_buf, BUFSIZE, "Host: %s\r\n\r\n", content_host_name);
    strncat(s_buf, k_buf, BUFSIZE);

    if (send(tcpsock, s_buf, strsize + 1, 0) == -1) {
        fprintf(stderr, "Failed to send request.");
        exit(EXIT_FAILURE);
    }

    if ((strsize = recv(tcpsock, r_buf, BUFSIZE - 1, 0)) == -1) {
        fprintf(stderr, "Failed to receive response.");
        exit(EXIT_FAILURE);
    }
    r_buf[strsize] = '\0';

    int status = get_status(r_buf);
    if (status == 404) {
        /* 指定したコンテンツが存在しなかった場合 -> サーバーが404 Not Foundを返したときと解釈 */
        printf("Server returned 404 Not Found; Requested content `%s` does not exist.\n\n", content_url);
    }

    ssize_t size;

    size = get_value_from_response(r_buf, "Content-Length", k_buf);
    printf("Content length\n-> ");
    if (size >= 0) {
        printf("%s\n", k_buf);
    } else {
        printf("Unknown (`Content-Length` field not found in response header)\n");
    }

    size = get_value_from_response(r_buf, "Server", k_buf);
    printf("Server software running at `%s`\n-> ", content_host_name);
    if (size >= 0) {
        printf("%s\n", k_buf);
    } else {
        printf("Unknown (`Server` field not found in response header)\n");
    }

    close(tcpsock);
    exit(EXIT_SUCCESS);
}
