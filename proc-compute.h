#ifndef _PROC_COMPUTE_H_
#define _PROC_COMPUTE_H_

#include "proc-common.h"
#include <poll.h>

#define MAX_HIDDEN 150

typedef struct {
    int max;                    // local max
    int sum;                    // sum of elements
    double ave;                 // average (computed locally)
    int count;                  // number of elements processed
    double elapsed;             // time taken (seconds)
    int bytes;                  // IPC bytes communicated
    pid_t pid;                  // PID

    int hidden_found;           // number of hidden keys found
    int hidden_positions[MAX_HIDDEN]; // indices of hidden keys
} result_t;

void process_subarray(int *arr, int l, int r, int pipe_fd_write, int id);

#endif
