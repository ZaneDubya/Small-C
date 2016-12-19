// YLINK - the Ypsilon Executable Linker
// Copyright 2016 Zane Wagner. All rights reserved.
#define VERSION "Ypsilon Executable Linker, Version 0.01\n"
#define CRIGHT1 "Copyright 2016 Zane Wagner. All rights reserved.\n"
#include <stdio.h>
#define LINEMAX  127
#define LINESIZE 128
#define ERROR 0

char    errParsingArgs[] = "Could not parse argument.",
        errNoArguments[] = "Requires exactly one argument.",
        errCouldNotOpen[] = "Could not open file: ";
char *line;

int     fdObjectFile;

main(int argc, int *argv) {
    fputs(VERSION, stderr);
    fputs(CRIGHT1, stderr);
    allocatevars();
    if (ask(argc, argv) == ERROR) {
        return;
    }
    readFile();
    if (fdObjectFile != 0) {
        fclose(fdObjectFile);
    }
}

allocatevars() {
    line = calloc(LINESIZE, 1);
}

ask(int argc, int *argv) {
    int i;
    if (argc != 2) {
        errout(errNoArguments);
        return ERROR;
    }
    getarg(1, line, LINESIZE, argc, argv);
    if(!(fdObjectFile = fopen(line, "r"))) {
        erroutf(errCouldNotOpen, line);
        return ERROR;
    }
    i = 1;
    /*while (getarg(i++, line, LINESIZE, argc, argv) != EOF) {
        lineout(line);
        lineout('\n');
    }*/
	return;
}

readFile() {
    char ch;
    while ((ch = fgetc(fdObjectFile)) != EOF) {
        putchar(ch);
    }
    return;
}

lineout(char *line, int fd) {
    fputs(line, fd);
    fputc(NEWLINE, fd);
}

errout(char *line) {
    fputs("*** Error: ", stderr); 
    lineout(line, stderr);
}

erroutf(char *format, char *arg) {
    fputs("*** Error: ", stderr); 
    fprintf(stderr, format, arg);
    lineout(line, stderr);
}