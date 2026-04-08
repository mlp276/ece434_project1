#ifndef _PROC_COMMON_H_
#define _PROC_COMMON_H_

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <poll.h>
#include <time.h>

#define PIPE_READ_END 0
#define PIPE_WRITE_END 1
#define BUFFER_SIZE 1024
#define NUM_CHILDREN 2

struct data {
    int mx;
    int sum;
    double ave;
    int numel;
    double elapsed;
};

long get_integer(const char *nptr);
void fork_processes(int num_processes);
void non_leaf();

#endif