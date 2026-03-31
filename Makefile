# A simple Makefile

CC=gcc
CFLAGS=-Wall -02

all: fork-example

fork-example: fork-example.o proc-common.o
	$(CC) fork-example fork-example.o proc-common.o

proc-common.o: proc-common.c proc-common.h
	$(CC) $(CFLAGS) -o proc-common.o -c proc-common.c

fork-example.o: fork-example.c proc-common.h
	$(CC) $(CFLAGS) -o fork-example.o -c fork-example.c

clean:
	rm -f fork-example proc-common.o fork-example.o
