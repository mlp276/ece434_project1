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
    int pipefds[2];
    char buffer[BUFFER_SIZE];
    size_t bytes_read;

    if (pipe(pipefds) < 0)
    {
        /* Return error, pipe failed */
        perror("pipe");
        exit(1);
    }

    /* Write data from the input file to the pipe */
    while ((bytes_read = read(input_file, buffer, sizeof(buffer))) > 0)
    {
        write(pipefds[PIPE_WRITE_END], buffer, bytes_read);
    }
    close(pipefds[PIPE_WRITE_END]);

    /* Read the data from the pipe and print that data */
    while ((bytes_read = read(pipefds[PIPE_READ_END], buffer, sizeof(buffer))) > 0)
    {
        write(STDOUT_FILENO, buffer, bytes_read);
    }
    close(pipefds[PIPE_READ_END]);

    close(input_file);

    return 0;
}