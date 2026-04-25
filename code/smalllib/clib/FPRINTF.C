#include "stdio.h"

// fprintf(fd, ctlstring, arg, arg, ...) - Formatted print.
// printf(ctlstring, arg, arg, ...) - Formatted print to stdout.
// Both backed by _print() in cmn_prnt.c.
int fprintf(int fd, char *fmt, ...) {
    return(_print(fd, &fmt));
}

int printf(char *fmt, ...) {
    return(_print(stdout, &fmt));
}

