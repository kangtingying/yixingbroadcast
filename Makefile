SHELL = /bin/sh

OBJS =  main.o db.o server.o
CFLAG = -Wall -g
CC = gcc
INCLUDE =
LIBS = -lsqlite3 -lpthread

main:${OBJS}
	${CC} ${CFLAGS} ${INCLUDES} -o $@ ${OBJS} ${LIBS}

clean:
	-rm -f *.o core *.core

.cpp.o:
	${CC} ${CFLAGS} ${INCLUDES} -c $<
