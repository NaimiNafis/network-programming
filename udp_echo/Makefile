#
# Makefile for udp_echo
#
MYLIBDIR=../mynet
CFLAGS=-I${MYLIBDIR}

all: echo_server echo_client echo_server1 echo_client1 echo_client2 client server

echo_server: echo_server.o ${MYLIBDIR}/init_udpserver.o ${MYLIBDIR}/other.o
	${CC} ${CFLAGS} -o $@ $^

echo_client: echo_client.o ${MYLIBDIR}/init_udpclient.o ${MYLIBDIR}/other.o
	${CC} ${CFLAGS} -o $@ $^

echo_server1: echo_server1.o ${MYLIBDIR}/init_udpserver.o ${MYLIBDIR}/other.o
	${CC} ${CFLAGS} -o $@ $^

echo_client1: echo_client1.o ${MYLIBDIR}/init_udpclient.o ${MYLIBDIR}/other.o
	${CC} ${CFLAGS} -o $@ $^

echo_client2: echo_client2.o ${MYLIBDIR}/init_udpclient.o ${MYLIBDIR}/other.o
	${CC} ${CFLAGS} -o $@ $^

client: client.o ${MYLIBDIR}/init_udpclient.o ${MYLIBDIR}/init_tcpclient.o ${MYLIBDIR}/other.o
	${CC} ${CFLAGS} -o $@ $^

server: server.o ${MYLIBDIR}/init_udpserver.o ${MYLIBDIR}/init_tcpserver.o ${MYLIBDIR}/other.o
	${CC} ${CFLAGS} -o $@ $^

clean:
	${RM} *.o echo_server echo_client echo_server1 echo_client1 echo_client2 client server *~
