CC=gcc
CFLAGS=-Wall
LIBS=-lm

all: process-file

process-file: process-file.o process-file-lib.o
	$(CC) $(CFLAGS) -o process-file process-file.o process-file-lib.o $(LIBS)

process-file-lib.o: process-file-lib.c process-file-lib.h
	$(CC) $(CFLAGS) -o process-file-lib.o -c process-file-lib.c $(LIBS)

process-file.o: process-file.c process-file-lib.h
	$(CC) $(CFLAGS) -o process-file.o -c process-file.c $(LIBS)

clean:
	rm -f process-file process-file-lib.o process-file.o output.txt
