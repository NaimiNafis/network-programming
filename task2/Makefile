MYLIBDIR=../mynet
MYLIB=-lmynet
CFLAGS=-I${MYLIBDIR} -L${MYLIBDIR}
OBJS=task2.o

all: task2

task2: ${OBJS}
	${CC} ${CFLAGS} -o $@ $^ ${MYLIB}

clean:
	${RM} *.o task2 *~
