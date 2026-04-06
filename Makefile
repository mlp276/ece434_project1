# A simple Makefile

CC=gcc
CFLAGS=-Wall

all: process-file

process-file: process-file.o proc-common.o
	$(CC) -o process-file process-file.o proc-common.o

proc-common.o: proc-common.c proc-common.h
	$(CC) $(CFLAGS) -o proc-common.o -c proc-common.c

process-file.o: process-file.c proc-common.h
	$(CC) $(CFLAGS) -o process-file.o -c process-file.c

clean:
	rm -f process-file proc-common.o process-file.o
