#
# Makefile for libmynet
#
OBJS = init_tcpserver.o init_tcpclient.o other.o
AR = ar -qc

libmynet.a : ${OBJS}
	${RM} $@
	${AR} $@ ${OBJS}

${OBJS}: mynet.h

task3 : task3.c libmynet.a
	gcc task3.c libmynet.a -o $@
	
clean:
	${RM} *.o
