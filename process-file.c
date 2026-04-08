#include "proc-common.h"

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

    L  = (int) get_integer(argv[1]);
    H  = (int) get_integer(argv[2]);
    PN = (int) get_integer(argv[3]);

    // if (L < 12000)
    // {
    //     /* Return error, must be >= 12000 */
    //     fprintf(stderr, "Improper L argument: must be >= 12000\n");
    //     exit(1);
    // }
    
    if (H < 150)
    {
        /* Return error, must be >= 150 */
        fprintf(stderr, "Improper H argument: must be >= 150\n");
        exit(1);
    }

    if (PN < 1)
    {
        /* Return error, must be >= 1 */
        fprintf(stderr, "Improper PN argument: must have at least 1 processes active\n");
        exit(1);
    }

    printf("L: %d, H: %d, PN: %d\n", L, H, PN);

    /* OPENING THE INPUT FILE AND WRITING ITS DATA TO PIPE */
    FILE *file = fopen("input.txt", "r");
    if (file == NULL) return 1;
    int arr[L];
    int i = 0;

    // fscanf returns the number of items successfully read
    while (i < L && fscanf(file, "%d", &arr[i]) == 1) i++;

    /* FORK CHILDREN TO ALLOCATE RESOURCES */

    printf("Process: %d is the root node\n", getpid());
    fork_processes(PN, L, -1, arr);

    return 0;
}