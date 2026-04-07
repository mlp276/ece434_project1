#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <time.h>
#include "proc-common.h"

// Create struct to store data to send/receive from pipes
    struct data {
        int mx;
        int sum;
        double ave;
        int numel;
        double elapsed;
    };

// Max function
int max(int a, int b) {
    return (a > b ? a : b);
}

// Read data from child in read end of pipe
void read_data(int fd, struct data *child_data) {
    if (read(fd, &child_data->mx, sizeof(child_data->mx)) == -1) {
        perror("read");
        exit(1);
    }
    if (read(fd, &child_data->sum, sizeof(child_data->sum)) == -1) {
        perror("read");
        exit(1);
    }
    if (read(fd, &child_data->ave, sizeof(child_data->ave)) == -1) {
        perror("read");
        exit(1);
    }
    if (read(fd, &child_data->numel, sizeof(child_data->numel)) == -1) {
        perror("read");
        exit(1);
    }
    if (read(fd, &child_data->elapsed, sizeof(child_data->elapsed)) == -1) {
        perror("read");
        exit(1);
    }
}

void write_data(int fd, struct data *parent_data) {
    if (write(fd, &parent_data->mx, sizeof(parent_data->mx)) == -1) {
        perror("write");
        exit(1);
    }
    if (write(fd, &parent_data->sum, sizeof(parent_data->sum)) == -1) {
        perror("write");
        exit(1);
    }
    if (write(fd, &parent_data->ave, sizeof(parent_data->ave)) == -1) {
        perror("write");
        exit(1);
    }
    if (write(fd, &parent_data->numel, sizeof(parent_data->numel)) == -1) {
        perror("write");
        exit(1);
    }
    if (write(fd, &parent_data->elapsed, sizeof(parent_data->elapsed)) == -1) {
        perror("write");
        exit(1);
    }
}

// Important: below is just pseudocode
// Assumptions:
//  - Pipe between process and parent is created
//  - Pipes between process and its children are created
//  - Process already provided the subarray it computes in the form of endpoints [l, r]
void non_leaf() {
    // Collecting elapsed time
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Everything here should already be initialized
    int l, r;
    int num_children;
    int read_pfds[num_children][2];
    int write_pfds[num_children][2];
    int parent_pfd[2];

    // Data to send to parent
    int mx = 0; // max
    int sum = 0;
    double ave = 0;
    int numel = 0;

    // Other variables
    struct pollfd *child_polls = malloc(num_children*sizeof(struct pollfd));    // Used to poll read ends of all children
    struct data *child_data = malloc(num_children*sizeof(struct data)); // Array of child data
    if (child_polls == NULL || child_data == NULL) {
        perror("malloc");
        exit(1);
    }
    int child_seg_length = (r - l + 1)/num_children; // Evenly split range [l, r] across children

    // Loop over children
    for (int i = 0; i < num_children; i++) {
        // Assign segment to each child
        int child_l = l + i*child_seg_length;
        int child_r = l + (i + 1)*child_seg_length - 1;

        if (i == num_children - 1)  // Assign any remainder to last segment if entire range not covered
            child_r = r;
        
        // Create a struct pollfd for each child
        child_polls[i].fd = read_pfds[i][PIPE_READ_END];
        child_polls[i].events = POLLIN;
    }

    int finished_children = 0;
    int timeout = 10000;    // 10 second timeout

    // Poll children until all children finish
    while (finished_children < num_children) {
        int num_polled = poll(&child_polls, num_children, timeout);
        if (num_polled == -1) { // Poll failed
            perror("poll");
            exit(1);
        }

        // Loop over children to determine which ones finished
        for (int i = 0; i < num_children; i++) {
            if (child_polls[i].revents & POLLIN) {  // Child has sent data
                read_data(child_polls[i].fd, &child_data[i]);
                close(child_polls[i].fd);
                finished_children += 1;
            }
        }
    }

    // Aggregate results
    for (int i = 0; i < num_children; i++) {
        struct data d = child_data[i];
        mx = max(mx, d.mx);
        sum += d.sum;
        numel += d.numel;
    }
    ave = sum/numel;

    // Return data to parent
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = end.tv_sec - start.tv_sec; // Maybe include nanosecond precision
    struct data parent_data = {mx, sum, ave, numel, elapsed};
    write_data(parent_pfd[PIPE_WRITE_END], &parent_data);
}