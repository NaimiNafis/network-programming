# # Makefile for task5_server and task5_client

# MYLIBDIR=../mynet
# CFLAGS=-I${MYLIBDIR}

# SERVER_SRC=task5_server.c
# CLIENT_SRC=task5_client.c
# MYNET_SRC=${MYLIBDIR}/init_udpclient.c ${MYLIBDIR}/init_udpserver.c ${MYLIBDIR}/init_tcpclient.c ${MYLIBDIR}/init_tcpserver.c ${MYLIBDIR}/other.c

# SERVER_OBJ=$(SERVER_SRC:.c=.o) $(MYNET_SRC:.c=.o)
# CLIENT_OBJ=$(CLIENT_SRC:.c=.o) $(MYNET_SRC:.c=.o)

# all: task5_server task5_client

# task5_server: ${SERVER_OBJ}
# 	${CC} ${CFLAGS} -o $@ ${SERVER_OBJ}

# task5_client: ${CLIENT_OBJ}
# 	${CC} ${CFLAGS} -o $@ ${CLIENT_OBJ}

# %.o: %.c
# 	${CC} ${CFLAGS} -c -o $@ $<

# clean:
# 	rm -f *.o task5_server task5_client

# Makefile for the combined server and client file

MYLIBDIR=../mynet
CFLAGS=-I${MYLIBDIR}
SRC=task5.c
MYNET_SRC=${MYLIBDIR}/init_udpclient.c ${MYLIBDIR}/init_udpserver.c ${MYLIBDIR}/init_tcpclient.c ${MYLIBDIR}/init_tcpserver.c ${MYLIBDIR}/other.c

OBJ=$(SRC:.c=.o) $(MYNET_SRC:.c=.o)

all: task5

task5: ${OBJ}
	${CC} ${CFLAGS} -o $@ ${OBJ} -lpthread

%.o: %.c
	${CC} ${CFLAGS} -c -o $@ $<

clean:
	rm -f *.o task5
