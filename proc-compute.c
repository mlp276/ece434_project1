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
        }
    }

    //finish timing
    clock_gettime(CLOCK_MONOTONIC, &end);
    result.elapsed = end.tv_sec - start.tv_sec; // Maybe include nanosecond precision

    // printf("Sending max = %d, sum = %d, count = %d, ave = %f\n", result.max, result.sum, result.count, result.ave);


    // Send result to parent
    if (write(pipe_fd_write, &result, sizeof(result)) < 0)
    {
        perror("write to pipe failed");
        exit(1);
    }

    close(pipe_fd_write);

    // Print debug
    printf("ECE 434 Sp26: I’m process %d and my parent is %d. "
           "Processed [%d, %d], found %d hidden keys.\n",
           getpid(), getppid(), l, r, result.hidden_found);

    // Sleep so tree is visible (assignment requirement)
    sleep(1);

    // // printf("leaf process %d finished.\n", getpid());


    exit(id);
}
