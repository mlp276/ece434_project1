#include "process-file-lib.h"

int child_read_pfds[MAX_CHILD_PROCESSES][2]; // The pipe of data from each child process to this process
int child_write_pfds[MAX_CHILD_PROCESSES][2]; // The pipe of data from this process to each child process
int parent_read_pfd[2]; // The pipe of data from the parent process to this process
int parent_write_pfd[2]; // The pipe of data from this process to the parent
int is_root = 1; // Checks if this process is the root node
int exit_arg = -1; // The unique ID assigned to this process - to be modified when forking

int hidden_found; // The number of hidden keys found
int hidden_positions[MAX_HIDDEN_KEYS]; // The indices of the hidden keys

/* Returns the difference of nanoseconds between the start and the end */
int get_nanoseconds_diff(struct timespec start, struct timespec end)
{
    return (int)(end.tv_sec - start.tv_sec) * 1000000000LL + (end.tv_nsec - start.tv_nsec);
}

/* Gets the integer from the character */
int get_integer(const char *nptr)
{
    char *endptr;
    int res = strtol(nptr, &endptr, 10);
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


/* Generate a random array of integers */
void generate_random_array(int L, int H)
{
    srand(time(NULL));
    int *a = malloc(L * sizeof(int));
    for (int i = 0; i < L; i++)
        a[i] = (rand() % 10000) + 1;
    for (int i = 0; i < H; i++)
        a[rand() % L] = -(rand() % 100 + 1);
    FILE *f = fopen("input.txt", "w");
    fprintf(f, "%d\n", L);
    for (int i = 0; i < L; i++)
        fprintf(f, "%d\n", a[i]);
    fclose(f);
    free(a);
}

/* Handler for the secret number signal */
void secret_number_handler(int signum) {
    printf("ECE 434 Sp26: I am process %d with return arg %d. I received secret number signal %d.\n", getpid(), exit_arg, signum);
    exit(exit_arg);
}

/* Handler for SIGUSR1*/
void siguser1_handler(int signum, siginfo_t *info, void *ucontext) {
    // Get secret number from payload
    int secret_number = info->si_value.sival_int;

    // Register signal handler for signal secret_number
    struct sigaction sa_sigsecret;
    sa_sigsecret.sa_handler = secret_number_handler;
    sigaction(SIGUSR1, &sa_sigsecret, NULL);

    // Raise signal secret_number
    raise(secret_number);
}

/* Handler for SIGINT (experiment 1)*/
void sigint_handler(int signum) {
    printf("ECE 434 Sp26: I am process %d with parent process %d and return arg %d. I received SIGINT signal.\n", getpid(), getppid(), exit_arg);
}

/* Function that a child calls to let its parent decide its fate after sending its data */
void let_parent_decide_fate() {
    raise(SIGTSTP);

    // At this point, parent has delieverd SIGCONT
    // Sleep to let parent take actions based on its decision rules
    sleep(100);
}

/**
 * @brief Forks processes to process the array.
 * @param num_desc_processes The number of descendent processes to create.
 * @param arr The pointer to the array of integers.
 * @param L The number of integers in the array.
 * @param id The unique ID assigned to this process.
 * 
 */
void fork_processes(int num_desc_processes, int *arr, int L, int id)
{
    /* Assign global exit_arg variable */
    exit_arg = id;

    /* Register signal handlers for SIGSUSR1, SIGINT, and SIGCHLD*/
    struct sigaction sa_sigusr1, sa_sigint;

    sa_sigusr1.sa_sigaction = siguser1_handler;
    sa_sigusr1.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR1, &sa_sigusr1, NULL);

    if (SIGINT_EXPERIMENT == 1)
        sa_sigint.sa_handler = sigint_handler;
    else if (SIGINT_EXPERIMENT == 2)
        sa_sigint.sa_handler = SIG_IGN;
    sigaction(SIGINT, &sa_sigint, NULL);

    /* CHECK IF PROCESS IS A LEAF NODE */

    int l = 0;     // The left index of the array for this process
    int r = L - 1; // The right index of the array for this process

    /* Read from the parent the provided l and r indices for this process */
    if (!is_root)
    {
        read(parent_read_pfd[PIPE_READ_END], &l, sizeof(l));
        read(parent_read_pfd[PIPE_READ_END], &r, sizeof(r));
    }

    if (num_desc_processes == 0)
    {
        /* Process is a leaf node, do processing on given array */
        leaf(arr, l, r, parent_write_pfd[PIPE_WRITE_END], id);
    }
    /* Process is a non-leaf node, do processing to fork more children if possible */

    /* FORK UP TO THE MAXIMUM NUMBER OF CHILDREN */

    int num_children_to_fork = MAX_CHILD_PROCESSES;
    if (num_desc_processes < num_children_to_fork) 
    {
        /* This process will only fork as many processes as it is provided, up
            to the maximum number of children */
        num_children_to_fork = num_desc_processes;
    }
    
    /* The number of descendent processes to allocate to each child process equally */
    int num_processes_to_allocate_evenly = (num_desc_processes - num_children_to_fork) / MAX_CHILD_PROCESSES;

    /* The remainder of descendent processes after equal allocation */
    int num_processes_leftover = (num_desc_processes - num_children_to_fork) % MAX_CHILD_PROCESSES;

    pid_t process;
    for (int n = 0; n < num_children_to_fork; ++n)
    {
        /* INITIALIZE THE PIPES */

        /* Initialize the pipe for */
        if (pipe(child_read_pfds[n]) < 0)
        {
            /* Return error */
            perror("pipe");
            exit(1);
        }

        /* Initialize the write ends of the pipes */
        if (pipe(child_write_pfds[n]) < 0)
        {
            /* Return error */
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
        {
            /* In the child process */
            is_root = 0;

            /* RELATE THE PIPES OF THIS PROCESS */

            /* Associate reading from the parent process in this process with
                writing from the parent process to this process */
            parent_read_pfd[PIPE_READ_END] = child_write_pfds[n][PIPE_READ_END];
            parent_read_pfd[PIPE_WRITE_END] = child_write_pfds[n][PIPE_WRITE_END];

            /* Associate writing from this process to the parent process with
                reading from this process in the parent process */
            parent_write_pfd[PIPE_READ_END] = child_read_pfds[n][PIPE_READ_END];
            parent_write_pfd[PIPE_WRITE_END] = child_read_pfds[n][PIPE_WRITE_END];

            /* Determine the section of the array allocated for this process */
            int child_segment_length = (r - l + 1) / num_children_to_fork;
            int child_l = l + n * child_segment_length;
            int child_r = l + (n + 1) * child_segment_length - 1;
            if (n == num_children_to_fork - 1)
            {
                child_r = r;
            }

            /* Write the indices of the section from the parent process to this process */
            write(child_write_pfds[n][PIPE_WRITE_END], &child_l, sizeof(child_l));
            write(child_write_pfds[n][PIPE_WRITE_END], &child_r, sizeof(child_r));

            int new_num_processes; // The number of descendent processes to create
            if (n == num_children_to_fork - 1)
            {
                /* Last child process will get the leftover processes to allocate */
                new_num_processes = num_processes_to_allocate_evenly + num_processes_leftover;
            }
            else
            {
                /* Every other child process will get an equal number of processes */
                new_num_processes = num_processes_to_allocate_evenly;
            }

            /* The new index of this process */
            int new_id = MAX_CHILD_PROCESSES * id + n;

            printf("ECE 434 Sp26: I am process %d with return arg %d, and my parent is %d.\n", getpid(), new_id, getppid());

            /* Create more descendent processes for this process */
            fork_processes(new_num_processes, arr, L, new_id);

            exit(0);
        }
        /* In the parent process */
    }

    /* EXECUTE NON-LEAF OPERATIONS */

    non_leaf(num_children_to_fork, id);
}

/**
 * @brief Performs non-leaf operations.
 * @param num_processes The number of child processes of this process.
 * @param id The unique ID assigned to this process.
 * 
 */
void non_leaf(int num_children, int id)
{
    struct timespec start, end; // Calculate the total time of the non-leaf operations
    clock_gettime(CLOCK_MONOTONIC, &start); // Start of timer

    /* The data to send to the parent process */
    int max = INT_MIN;
    int sum = 0;
    double ave = 0;
    int count = 0;
    int bytes = 2 * sizeof(int); // Already sent l, r to child
    pid_t slowest_child = -1;
    double slowest_time = 0;
    double least_hidden_nodes = INT_MAX;
    double most_hidden_nodes = 0;
    pid_t pid = getpid();

    /* The aggregation of data from each child process */
    struct data *child_datas = malloc(num_children * sizeof(struct data));
    if (child_datas == NULL)
    {
        perror("malloc");
        exit(1);
    }

    /* POLL FROM THE CHILDREN */

    /* Create a struct pollfd for each pipe from the child processes to this process */
    struct pollfd *child_polls = malloc(num_children * sizeof(struct pollfd));
    if (child_polls == NULL)
    {
        perror("malloc");
        exit(1);
    }
    for (int n = 0; n < num_children; ++n)
    {
        child_polls[n].fd = child_read_pfds[n][PIPE_READ_END];
        child_polls[n].events = POLLIN;
    }

    /* Poll children until all children finish */
    int finished_children = 0;
    int timeout = 1000;
    while (finished_children < num_children)
    {
        int num_polled = poll(child_polls, num_children, timeout);
        if (num_polled < 0)
        {
            perror("poll");
            exit(1);
        }

        /* Loop over children to determine which ones finished */
        for (int n = 0; n < num_children; ++n)
        {
            if (child_polls[n].revents & POLLIN)
            {
                int b = read(child_polls[n].fd, &child_datas[n], sizeof(struct data));
                close(child_polls[n].fd);
                bytes += b; // Increment by the number of bytes read from this child
                finished_children += 1;
            }
        }
    }

    /* AGGREGATE RESULTS OF THE CHILDREN */

    for (int n = 0; n < num_children; ++n)
    {
        struct data child_data = child_datas[n];
        max = fmax(max, child_data.max);
        sum += child_data.sum;
        count += child_data.count;
        bytes += child_data.bytes;
        if (child_data.elapsed > slowest_time)
        {
            slowest_time = child_data.elapsed;
            slowest_child = child_data.pid;
        }
        if (child_data.num_hidden_nodes < least_hidden_nodes)
            least_hidden_nodes = child_data.num_hidden_nodes;
        else if (child_data.num_hidden_nodes > most_hidden_nodes)
            most_hidden_nodes = child_data.num_hidden_nodes;
    }
    ave = sum / count;

    clock_gettime(CLOCK_MONOTONIC, &end); // End of timer
    int elapsed = get_nanoseconds_diff(start, end);

    /* MAKE DECISIONS FOR ALL CHILDREN BASED ON HIDDEN NODE COUNT */

    for (int n = 0; n < num_children; ++n) {
        struct data child_data = child_datas[n];
        pid_t cpid = child_data.pid;
        double child_hidden_nodes = child_data.num_hidden_nodes;

        // All rules: deliver SIGCONT
        kill(cpid, SIGCONT);
        
        // Rule 1: Child with most hidden nodes
        // Do Nothing
        if (child_hidden_nodes == most_hidden_nodes) {}

        // Rule 2: Child in the middle
        // Send secret number to child
        else if (least_hidden_nodes < child_hidden_nodes && child_hidden_nodes < most_hidden_nodes) {
            int secret_number = rand() % 32 + 1;
            union sigval info;
            info.sival_int = secret_number;
            sigqueue(cpid, SIGUSR1, info);
        }

        // Rule 3: Child with least hidden nodes
        // Sleep 10 seconds and then send SIGINT to child
        else if (child_hidden_nodes == least_hidden_nodes) {
            sleep(10);
            kill(cpid, SIGINT);
        }
    }

    /* DELIEVER SIGQUIT TO ALL CHILDREN AFTER MAKING DECISIONS */
    sleep(20);
    for (int n = 0; n < num_children; ++n) {
        struct data child_data = child_datas[n];
        pid_t cpid = child_data.pid;
        kill(cpid, SIGQUIT);
    }

    /* RETRIEVE EXIT STATUSES FROM THE CHILDREN */

    pid_t cpid;
    int status;
    for (int n = 0; n < num_children; ++n)
    {
        cpid = waitpid(-1, &status, 0);
        explain_wait_status(cpid, status);
    }

    /* SEND AGGREGATED RESULTS TO PARENT (IF NOT ROOT) */
    
    struct data parent_data = { max, sum, ave, count, pid, elapsed, bytes, slowest_child, slowest_time, least_hidden_nodes };

    /* Check if it is the root node */
    if (is_root)
    {
        /* This process is the root node, print the final results */
        printf("\n--- FINAL RESULTS ---\n");
        printf("ECE 434 Sp26: I am process %d with return arg %d, and also am the root node.\n", getpid(), id);
        printf("Max = %d, Sum = %d, Average = %f, Count = %d\n", parent_data.max, parent_data.sum, parent_data.ave, parent_data.count);
        printf("IPC volume = %d bytes\n", parent_data.bytes);
        printf("Slowest child = %d with elapsed time = %.2f sec\n", parent_data.slowest_child, (double)parent_data.slowest_time / 1e9);
        exit(0);
    }

    int sent_bytes = write(parent_write_pfd[PIPE_WRITE_END], &parent_data, sizeof(parent_data));
    if (sent_bytes < 0)
    {
        /* Return error */
        perror("write");
        exit(1);
    }

    /* Pause self and await parent to decide fate */
    let_parent_decide_fate();

    /* Exit with its unique ID */
    exit(id);
}

/**
 * @brief Perform leaf operations of reducing a section of the array of integers.
 * @param arr The pointer to the array of integers.
 * @param l The left index of the section in the array to process.
 * @param r The right index of the section in the array to process.
 * @param fd The write fd of the pipe to send the data to the parent process
 * @param id The unique ID assigned to this process. 
 * 
 */
void leaf(int *arr, int l, int r, int fd, int id)
{
    struct timespec start, end; // Calculate the total time of the leaf operations
    clock_gettime(CLOCK_MONOTONIC, &start); // Start of timer

    /* REDUCE THE SECTION OF ARRAY */

    struct data result;
    result.max = arr[l];
    result.sum = 0;
    result.count = 0;
    result.bytes = 2 * sizeof(int); // Already received l, r from parent
    result.pid = getpid();
    result.slowest_child = -1;
    
    hidden_found = 0;

    int val;
    for (int i = l; i <= r; ++i)
    {
        val = arr[i];

        result.max = fmax(result.max, val);
        result.sum += val;
        result.count++;

        /* Find the hidden keys, which are negative integers */
        if (val < 0)
        {
            hidden_positions[hidden_found] = i;
            hidden_found++;
            printf("ECE 434 Sp26: I am process %d with return arg %d. I found the hidden key in position A[%d]\n",
                getpid(), id, i);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end); // End of timer
    int elapsed = get_nanoseconds_diff(start, end);
    result.elapsed = elapsed;
    result.slowest_time = 0;
    result.num_hidden_nodes = hidden_found;

    /* SEND RESULT TO THE PARENT */

    if (write(fd, &result, sizeof(result)) < 0)
    {
        /* Return error */
        perror("write");
        exit(1);
    }
    close(fd);

    printf("ECE 434 Sp26: I am process %d with return arg %d. "
           "Processed [%d, %d], found a total of %d hidden keys.\n",
           getpid(), id, l, r, hidden_found);

    /* Sleep so tree is visible (assignment requirement) */
    sleep(1);

    /* Pause self and await parent to decide fate */
    let_parent_decide_fate();

    /* Exit with its unique ID */
    exit(id);
}

/**
 * @brief Explains the wait status.
 * @param pid The PID of the process with this exit status.
 * @param status The exit status of the terminated process.
 * 
 */
void explain_wait_status(pid_t pid, int status) {
    if (WIFEXITED(status))
    {
        /* The process terminated naturally with a specific exit value */
        fprintf(stderr, "Child with PID = %ld exited naturally, status = %d\n", (long)pid, WEXITSTATUS(status));
    }
    else if (WIFSIGNALED(status))
    {
        /* The process was killed by a signal */
        fprintf(stderr, "Child with PID = %ld killed by signal: %d\n", (long)pid, WTERMSIG(status));
    }
    else if (WIFSTOPPED(status))
    {
        /* The process was stopped by a signal */
        fprintf(stderr, "Child with PID = %ld stopped by signal %d\n",(long)pid, WSTOPSIG(status));
    }
    else
    {
        /* The process encountered an unhandled case */
        fprintf(stderr, "%s: Internal error: Unhandled case, PID = %ld, status = %d\n", __func__, (long)pid, status);
    }
}
