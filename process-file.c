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

    int depth;
    if (PN == 1) depth = 1;
    else depth = (int) ceil(log2(PN));
    int max_leaf_nodes = (int) ceil(pow(2, depth));

    printf("depth: %d, max_leaf_nodes: %d\n", depth, max_leaf_nodes);

    binary_fork_processes(depth);

    return 0;
}