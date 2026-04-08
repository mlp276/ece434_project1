#include "proc-common.h"

int child_read_pfds[NUM_CHILDREN][2];
int child_write_pfds[NUM_CHILDREN][2];
int parent_read_pfd[2];
int parent_write_pfd[2];

void explain_wait_status(pid_t pid, int status) {
  if (WIFEXITED(status)) {
    fprintf(stderr,"Child with PID = %ld exited naturally, status = %d\n", (long)pid, WEXITSTATUS(status));
  }
  else if (WIFSIGNALED(status)) {
    fprintf(stderr,"Child with PID = %ld killed by signal: %d\n", (long)pid, WTERMSIG(status));
  }
  else if (WIFSTOPPED(status)) {
    fprintf(stderr,"Child with PID = %ld stopped by signal %d\n",(long)pid,WSTOPSIG(status));
  }
  else {
    fprintf(stderr, "%s: Internal error: Unhandled case, PID = %ld, status = %d\n", __func__, (long)pid, status);
  }
}

long get_integer(const char *nptr)
{
    char *endptr;
    long res = strtol(nptr, &endptr, 10);
    if (errno == ERANGE)
    {
        /* Return error, given string is not an integer */
        perror("strtol");
        exit(1);
    }
    else if (endptr == nptr)
    {
        // printf(stderr, "%s must be an integer.\n", nptr);
        exit(1);
    }
    return res;
}

void fork_processes(int num_processes, int L, int n, int *arr, int id)
{
    /* CHECK IF PROCESS IS A LEAF NODE */
    int l = 0, r = L - 1;
    if (n >= 0)
    {
        read(parent_read_pfd[PIPE_READ_END], &l, sizeof(l));
        read(parent_read_pfd[PIPE_READ_END], &r, sizeof(r));
    }
    // printf("In process %d, l = %d, r = %d\n", getpid(), l, r);

    if (num_processes == 0)
    { /* Process is a leaf node, do processing on given pipe */
        // printf("Leaf node processing for process %d\n", getpid());
        process_subarray(arr, l, r, parent_write_pfd[PIPE_WRITE_END], id);
    }

    /* Fork up to the max number of child processes */    
    int num_children_to_fork = NUM_CHILDREN;
    if (num_processes < num_children_to_fork) 
    {
        num_children_to_fork = num_processes;
    }
    
    int num_processes_to_allocate_evenly = (num_processes - num_children_to_fork) / NUM_CHILDREN;
    int num_processes_leftover = (num_processes - num_children_to_fork) % NUM_CHILDREN;

    pid_t process;
    for (int n = 0; n < num_children_to_fork; ++n)
    {
        /* INITIALIZE THE PIPES */

        /* Initialize the read ends of the pipes */
        if (pipe(child_read_pfds[n]) < 0)
        {
            perror("pipe");
            exit(1);
        }

        /* Initialize the write ends of the pipes */
        if (pipe(child_write_pfds[n]) < 0)
        {
            perror("pipe");
            exit(1);
        }

        /* FORK THE CHILDREN */

        process = fork();
        if (process < 0)
        {
            perror("fork");
            exit(1);  
        }
        else if (process == 0)
        { /* In the child process */
            // printf("Process %d created, parent is %d\n", getpid(), getppid());

            /* Associate reading from parent with writing from parent to child */
            parent_read_pfd[PIPE_READ_END] = child_write_pfds[n][PIPE_READ_END];
            parent_read_pfd[PIPE_WRITE_END] = child_write_pfds[n][PIPE_WRITE_END];

            /* Associate writing to parent with reading from child in parent */
            parent_write_pfd[PIPE_READ_END] = child_read_pfds[n][PIPE_READ_END];
            parent_write_pfd[PIPE_WRITE_END] = child_read_pfds[n][PIPE_WRITE_END];

            int child_segment_length = (r - l + 1) / num_children_to_fork;
            int child_l = l + n * child_segment_length;
            int child_r = l + (n + 1) * child_segment_length - 1;
            if (n == num_children_to_fork - 1) child_r = r;
            write(child_write_pfds[n][PIPE_WRITE_END], &child_l, sizeof(child_l));
            write(child_write_pfds[n][PIPE_WRITE_END], &child_r, sizeof(child_r));

            int new_num_processes;
            if (n == num_children_to_fork - 1)
            { /* Last child process will get the leftover processes to allocate */
                new_num_processes = num_processes_to_allocate_evenly + num_processes_leftover;
            }
            else
            { /* Every other child process will get an equal number of processes */
                new_num_processes = num_processes_to_allocate_evenly;
            }

            int new_id = NUM_CHILDREN * id + n;
            printf("ECE 434 Sp26: I'm process %d with return arg %d and my parent is %d.\n", getpid(), new_id, getppid());
            fork_processes(new_num_processes, L, n, arr, new_id);
            exit(0);
        }
    }

    /* EXECUTE NON-LEAF FUNCTIONALITIES */

    // printf("process %d, n = %d\n", getpid(), n);
    non_leaf(num_children_to_fork, n, id);
}

void non_leaf(int num_children, int n, int id)
{
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    /* Data to send to the parent */
    int mx = 0;
    int sum = 0;
    double ave = 0;
    int numel = 0;

    struct pollfd* child_polls = malloc(num_children * sizeof(struct pollfd));
    struct data* child_data = malloc(num_children * sizeof(struct data));
    if (child_polls == NULL || child_data == NULL)
    {
        perror("malloc");
        exit(1);
    }

    /* Create a struct pollfd for each pipe from child */
    for (int n = 0; n < num_children; n++)
    {
        child_polls[n].fd = child_read_pfds[n][PIPE_READ_END];
        child_polls[n].events = POLLIN;
    }

    int finished_children = 0;
    int timeout = 1000;

    // printf("process %d has %d children\n", getpid(), num_children);

    // Poll children until all children finish
    while (finished_children < num_children)
    {
        int num_polled = poll(child_polls, num_children, timeout);
        // printf("polled in process %d\n", getpid());
        if (num_polled == -1)
        {
            perror("poll");
            exit(1);
        }

        // Loop over children to determine which ones finished
        for (int n = 0; n < num_children; n++)
        {
            if (child_polls[n].revents & POLLIN)
            {
                read(child_polls[n].fd, &child_data[n], sizeof(struct data));
                // printf("Retrieved data in process %d\n", getpid());
                close(child_polls[n].fd);
                finished_children += 1;
            }
        }
    }

    pid_t pid;
    int status;
    for (int i = 0; i < num_children; ++i)
    {
        pid = waitpid(-1, &status, 0);
        explain_wait_status(pid, status);
    }

    /* Aggregate results from all the children */
    for (int i = 0; i < num_children; i++) {
        struct data d = child_data[i];
        mx = fmax(mx, d.mx);
        sum += d.sum;
        numel += d.numel;
    }
    ave = sum / numel;


    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = end.tv_sec - start.tv_sec;

    /* Send data to the parent */
    struct data parent_data = { mx, sum, ave, numel, elapsed };

    /* Check if it is the root node */
    if (n < 0)
    {
        /* FINAL RESULTS PRINTED */
        printf("max = %d, sum = %d, count = %d, average = %f\n", parent_data.mx, parent_data.sum, parent_data.numel, parent_data.ave);
        return;
    }
    if (write(parent_write_pfd[PIPE_WRITE_END], &parent_data, sizeof(parent_data)) == -1)
    {
        perror("write");
        exit(1);
    }
    // printf("data sent in process %d\n", getpid());

    // printf("process %d finished\n", getpid());
    // // printf("Non-leaf process %d finished.\n", getpid());
    exit(id);
}