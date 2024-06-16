MYLIBDIR=../mynet
MYLIB=-lmynet
CFLAGS=-I${MYLIBDIR} -L${MYLIBDIR}
OBJS=task4.o

all: task4

task4: ${OBJS}
	${CC} ${CFLAGS} -o $@ $^ ${MYLIB}

clean:
	${RM} *.o task4 *~
