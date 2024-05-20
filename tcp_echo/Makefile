#
# Makefile for tcp_echo
#
MYLIBDIR=../mynet
MYLIB=-lmynet -lpthread
CFLAGS=-I${MYLIBDIR} -L${MYLIBDIR}

all: echo_server2 echo_client2 echofork echothread

echo_server2: echo_server2.o
	${CC} ${CFLAGS} -o $@ $^ ${MYLIB}

echo_client2: echo_client2.o
	${CC} ${CFLAGS} -o $@ $^ ${MYLIB}

echofork: echofork.o
	${CC} ${CFLAGS} -o $@ $^ ${MYLIB}

echothread: echothread.o
	${CC} ${CFLAGS} -o $@ $^ ${MYLIB}

clean:
	${RM} *.o echo_server2 echo_client2 echofork echothread *~