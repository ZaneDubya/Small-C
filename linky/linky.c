// LINKY - the Ypsilon Executable Linker
// Copyright 2016 Zane Wagner. All rights reserved.
#define VERSION "Ypsilon Executable Linker, Version 0.01\n"
#define CRIGHT1 "Copyright 2016 Zane Wagner. All rights reserved.\n\n"
#include <stdio.h>
#define LINEMAX  127
#define LINESIZE 128

#define ERROR 1
char errParsingArgs[] = "Error parsing argument:\n";
char *line;

main(int argc, int *argv) {
    fputs(VERSION, stderr);
    fputs(CRIGHT1, stderr);
    allocatevars();
    if (ask(argc, argv) == ERROR) {
        fputs(errParsingArgs, stderr);
    }
}

ask(int argc, int *argv) {
    int i;
    i = 1;
    while (getarg(i++, line, LINESIZE, argc, argv) != EOF) {
        fputs(line, stderr);	
    }
	return;
}

allocatevars() {
    line = calloc(LINESIZE, 1);
}