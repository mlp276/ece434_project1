#include "proc-common.h"

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
        fprintf(stderr, "%s must be an integer.\n", nptr);
        exit(1);
    }
    return res;
}

/***
 * Create a full binary tree of input depth (including root depth)
 * 
 * 
 */
void binary_fork_processes(int depth)
{
    if (depth == 0)
    { /* Process is a leaf node, do processing on given pipe */
        /* INSERT LEAF NODE PROCESSING HERE */
        printf("I'm process: %d (leaf node), and my parent is: %d\n", getpid(), getppid());
        exit(0);
    }

    pid_t process;

    /* Fork first child process */
    process = fork();
    if (process < 0)
    {
        perror("fork");
        exit(1);
    }
    else if (process == 0)
    { /* Child process 1 */
        binary_fork_processes(depth - 1);
        exit(0);
    }

    /* Fork second child process */
    process = fork();
    if (process < 0)
    {
        perror("fork");
        exit(1);
    }
    else if (process == 0)
    { /* Child process 2 */
        binary_fork_processes(depth - 1);
        exit(0);
    }

    /* Process is an internal node, do processing on retrieving information */
    printf("I'm process: %d (internal node)\n", getpid());

    /* INSERT INTERNAL NODE PROCESSING HERE */
    /* Note: Possibly replace wait() with poll since pipes will be involved */

    int status;
    for (int i = 0; i < 2; ++i)
    {
        wait(&status);
    }
}
