MYLIBDIR=../mynet
MYLIB=-lmynet
CFLAGS=-I${MYLIBDIR} -L${MYLIBDIR}
OBJS=task3.o

all: task3

task3: ${OBJS}
	${CC} ${CFLAGS} -o $@ $^ ${MYLIB}

clean:
	${RM} *.o task3 *~
