#include "mynet.h"

int Accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
    int r;
    if ((r = accept(s, addr, addrlen)) == -1) {
        exit_errmesg("accept()");
    }
    return (r);
}

int Send(int s, void *buf, size_t len, int flags)
{
    int r;
    if ((r = send(s, buf, len, flags)) == -1) {
        exit_errmesg("send()");
    }
    return (r);
}

int Recv(int s, void *buf, size_t len, int flags)
{
    int r;
    if ((r = recv(s, buf, len, flags)) == -1) {
        exit_errmesg("recv()");
    }
    return (r);
}
