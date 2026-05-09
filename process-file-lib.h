#ifndef _PROCESS_FILE_LIB_H_
#define _PROCESS_FILE_LIB_H_

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define PIPE_READ_END 0
#define PIPE_WRITE_END 1

#define MAX_CHILD_PROCESSES 2
#define MAX_HIDDEN_KEYS 150
#define MAX_TOTAL_PROCESSES 50

#define SIGINT_EXPERIMENT 1

/* The data to send from the non-leaf node to the parent process */
struct data
{
    int max;                // The maximum integer of the input file
    int sum;                // The sum of the integers in the input file
    double ave;             // The average of the integers in the input file
    int count;              // The number of integers in the input file
    pid_t pid;              // The PID of the process that sends this data
    long long elapsed;      // The time (in ns) elapsed for the process
    int bytes;              // The number of bytes transferred to other processes through pipes
    pid_t slowest_child;    // The PID of the slowest child process
    long long slowest_time; // The time (in ns) of the slowest child process
    int num_hidden_nodes;   // The number of hidden nodes found by this process
};

long long get_nanoseconds_diff(struct timespec start, struct timespec end);
int get_integer(const char *nptr);
int exists(char *fname);
void generate_random_array(int L, int H);

void fork_processes(int num_desc_processes, int *arr, int L, int id);
void non_leaf(int num_children_to_fork, int id);
void leaf(int *arr, int l, int r, int fd, int id);

void explain_wait_status(pid_t pid, int status);

#endif