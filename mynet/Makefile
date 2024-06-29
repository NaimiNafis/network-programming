#
# Makefile for libmynet
#
OBJS = init_tcpserver.o init_tcpclient.o init_udpserver.o init_udpclient.o other.o
AR = ar -qc

libmynet.a : ${OBJS}
	${RM} $@
	${AR} $@ ${OBJS}

${OBJS}: mynet.h

clean:
	${RM} *.o
