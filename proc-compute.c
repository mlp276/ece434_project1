#include "proc-compute.h"

void process_subarray(int *arr, int l, int r, int pipe_fd_write, int id)
{
    //timing
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    result_t result;

    result.max = arr[l];
    result.sum = 0;
    result.count = 0;
    result.hidden_found = 0;
    result.bytes = 2;       // received l, r from parent
    result.pid = getpid();

    for (int i = l; i <= r; i++)
    {
        int val = arr[i];

        // Max
        if (val > result.max)
            result.max = val;

        // Sum + count
        result.sum += val;
        result.count++;

        // Find the hidden keys which are negative values
        if (val < 0)
        {
            result.hidden_positions[result.hidden_found] = i;
            result.hidden_found++;
            printf("ECE 434 Sp26: I am process: %d with return arg %d. I found the hidden key in position A[%d]\n",
                    getpid(), id, i);
        }
    }

    //finish timing
    clock_gettime(CLOCK_MONOTONIC, &end);
    result.elapsed = end.tv_sec - start.tv_sec + (end.tv_nsec - start.tv_nsec)/1e9; 

    // printf("Sending max = %d, sum = %d, count = %d, ave = %f\n", result.max, result.sum, result.count, result.ave);


    // Send result to parent
    if (write(pipe_fd_write, &result, sizeof(result)) < 0)
    {
        perror("write to pipe failed");
        exit(1);
    }

    close(pipe_fd_write);

    // Print debug
    printf("ECE 434 Sp26: I’m process %d and with return arg %d. "
           "Processed [%d, %d], found a total of %d hidden keys.\n",
           getpid(), id, l, r, result.hidden_found);

    // Sleep so tree is visible (assignment requirement)
    sleep(1);

    // // printf("leaf process %d finished.\n", getpid());


    exit(id);
}
