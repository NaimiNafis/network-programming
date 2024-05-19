#
# Makefile for libmynet
#
OBJS = init_tcpserver.o init_tcpclient.o other.o
AR = ar -qc

libmynet.a : ${OBJS}
	${RM} $@
	${AR} $@ ${OBJS}

${OBJS}: mynet.h

clean:
	${RM} *.o
