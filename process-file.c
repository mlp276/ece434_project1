#include "process-file-lib.h"

int main(int argc, char *argv[])
{
    /* GETTING PROGRAM ARGUMENTS */
    
    char *program_name = argv[0]; // The name of the program
    
    if (argc != 4)
    {
        /* Return error, must provide 3 arguments */
        fprintf(stderr, "Usage: %s {L} {H} {PN}\n", program_name);
        exit(1);
    }

    int L;  // Number of positive integers in the input file
    int H;  // Number of hidden key integers in the input file
    int PN; // Number of children processes to fork

    /* Gather arguments, ensuring they are integers */
    L = get_integer(argv[1]);

    if (L < 12000)
    {
        /* Return error, must be >= 12000 */
        fprintf(stderr, "Improper L argument: must be >= 12000\n");
        exit(1);
    }
    
    H = get_integer(argv[2]);
    if (H < 150)
    {
        /* Return error, must be >= 150 */
        fprintf(stderr, "Improper H argument: must be >= 150\n");
        exit(1);
    }

    PN = get_integer(argv[3]);
    if (PN < 1)
    {
        /* Return error, must be >= 1 */
        fprintf(stderr, "Improper PN argument: must have at least 1 processes active\n");
        exit(1);
    }

    /* GENERATE RANDOM ARRAY IF NEEDED */

    if (!exists("input.txt"))
        generate_random_array(L, H);

    /* OPENING THE INPUT FILE AND WRITING ITS DATA TO ARRAY */

    FILE *file = fopen("input.txt", "r");
    if (file == NULL)
    {
        /* Return error */
        perror("fopen");
        exit(1);
    }

    int *arr = malloc(L * sizeof(int));
    if (arr == NULL)    {
        perror("malloc");
        exit(1);
    }
    int i = 0;

    /* fscanf() returns the number of items successfully read */
    while (i < L && fscanf(file, "%d", &arr[i]) == 1) i++;

    printf("ECE 434 Sp26: Main process PID = %d\n", getpid());

    /* FORK CHILDREN TO ALLOCATE RESOURCES */
    
    struct timespec start, end; // Calculate total time of the program
    clock_gettime(CLOCK_MONOTONIC, &start); // Start of timer
    
    /* Start forking processes from the root node */
    int root_id = 1;
    fork_processes(PN, arr, L, root_id);

    clock_gettime(CLOCK_MONOTONIC, &end); // End of timer

    long long elapsed = get_nanoseconds_diff(start, end);
    printf("Total runtime: %.9f sec\n", (double)elapsed / 1e9);

    /* END OF THE PROGRAM */

    fclose(file);

    return 0;
}