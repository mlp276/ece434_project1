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

    if (L < 12000)
    {
        /* Return error, must be >= 12000 */
        fprintf(stderr, "Improper L argument: must be >= 12000");
        exit(1);
    }
    
    if (H < 150)
    {
        /* Return error, must be >= 150 */
        fprintf(stderr, "Improper H argument: must be >= 150");
        exit(1);
    }

    if (PN < 1)
    {
        /* Return error, must be >= 1 */
        fprintf(stderr, "Improper PN argument: must have at least 1 processes active");
        exit(1);
    }

    printf("L: %d, H: %d, PN: %d\n", L, H, PN);

    /* OPENING THE INPUT FILE AND WRITING ITS DATA TO PIPE */

    char *input_file_name = "input.txt";
    int input_file = open(input_file_name, O_RDONLY);
    int pipefds_parent[2];

    if (pipe(pipefds_parent) < 0)
    {
        /* Return error, pipe failed */
        perror("pipe");
        exit(1);
    }

    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    /* Write data from the input file to the pipe */
    while ((bytes_read = read(input_file, buffer, sizeof(buffer))) > 0)
    {
        write(pipefds_parent[PIPE_WRITE_END], buffer, bytes_read);
    }
    close(pipefds_parent[PIPE_WRITE_END]);
    close(input_file);

    /* FORK CHILDREN TO ALLOCATE RESOURCES */

    printf("Process: %d is the root node\n", getpid());
    fork_processes(PN);

    return 0;
}