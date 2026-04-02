#include "proc-common.h"

long get_integer(const char *nptr)
{
    char *endptr;
    long res = strtol(nptr, &endptr, 10);
    if (errno == ERANGE)
    {
        /* Return error, L is not an integer */
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