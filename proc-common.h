#ifndef _PROC_COMMON_H_
#define _PROC_COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>

#define PIPE_READ_END 0
#define PIPE_WRITE_END 1
#define BUFFER_SIZE 1024

long get_integer(const char *nptr);
void binary_fork_processes(int depth);

#endif