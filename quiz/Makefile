MYLIBDIR=../mynet
MYLIB=-lmynet
CFLAGS=-I${MYLIBDIR} -L${MYLIBDIR}
OBJS=quiz.o quiz_server.o quiz_client.o quiz_util.o question.o quiz_common.o

all: quiz

quiz: ${OBJS}
	${CC} ${CFLAGS} -o $@ $^ ${MYLIB}

clean:
	${RM} *.o quiz *~
