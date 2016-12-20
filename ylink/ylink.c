// YLINK - the Ypsilon Executable Linker
// Copyright 2016 Zane Wagner. All rights reserved.
#include "stdio.h"
#include "notice.h"
#include "omformat.c"
#define LINEMAX  127
#define LINESIZE 128
#define ERROR 0

char    *line;
int     fdObjectFile;

main(int argc, int *argv) {
    fputs(VERSION, stderr);
    fputs(CRIGHT1, stderr);
    allocatevars();
    if (ask(argc, argv) == ERROR) {
        abort(1);
    }
    readFile();
    cleanup();
}

allocatevars() {
    line = allocvar(LINESIZE, 1);
}

allocvar(int nItems, int itemSize) {
    int result;
    result = calloc(nItems, itemSize);
    if (result == 0) {
        errout("Could not allocate memory.");
        abort(100);
    }
}

ask(int argc, int *argv) {
    int i;
    if (argc != 2) {
        errout("Requires exactly one argument.");
        return ERROR;
    }
    getarg(1, line, LINESIZE, argc, argv);
    if(!(fdObjectFile = fopen(line, "r"))) {
        erroutf("Could not open file: ", line);
        return ERROR;
    }
    i = 1;
    /*while (getarg(i++, line, LINESIZE, argc, argv) != EOF) {
        lineout(line);
        lineout('\n');
    }*/
	return;
}

cleanup() {
    if (fdObjectFile != 0) {
        fclose(fdObjectFile);
    }
    return;
}

readFile() {
    char ch;
    while (!feof(fdObjectFile) &&!ferror(fdObjectFile)) {
        ch = fgetc(fdObjectFile);
        putchar(ch);
    }
    return;
}

// === Error Routines =========================================================

errout(char *line) {
    fputs("*** Error: ", stderr); 
    lineout(line, stderr);
}

erroutf(char *format, char *arg) {
    fputs("*** Error: ", stderr); 
    fprintf(stderr, format, arg);
    lineout(line, stderr);
}

lineout(char *line, int fd) {
    fputs(line, fd);
    fputc(NEWLINE, fd);
}