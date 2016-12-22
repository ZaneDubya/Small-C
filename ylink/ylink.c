// YLINK - the Ypsilon Executable Linker
// Copyright 2016 Zane Wagner. All rights reserved.
#include "stdio.h"
#include "notice.h"
#include "omformat.c"
#define LINEMAX  127
#define LINESIZE 128
#define ERROR 0

char    *line;
int     input;

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
    if(!(input = fopen(line, "r"))) {
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
    if (input != 0) {
        fclose(input);
    }
    return;
}

// === Reading OBJ Format =====================================================

readFile() {
    unsigned int length;
    unsigned char recType, ch;
    while (1) {
        recType = _read(input);
        if (feof(input)) {
            fputs("eof", stderr);
            break;
        }
        if (ferror(input)) {
            fputs("ferr", stderr);
            break;
        }
        length = (_read(input) & 0x00ff);
        length += (_read(input) & 0x00ff) << 8;
        prnhexch(recType, stdout);
        fputc(' ', stdout);
        prnhexch(length / 256, stdout);
        prnhexch(length & 0x00ff, stdout);
        fputc(' ', stdout);
        while (length-- > 0) {
            ch = _read(input);
            prnhexch(ch, stdout);
        }
        fputc(NEWLINE, stdout);
    }
    return;
}

prnhexch(unsigned char ch, int fd) {
    unsigned char ch0;
    ch0 = (ch & 0xf0) >> 4;
    if (ch0 < 10) {
        ch0 += 48;
    }
    else {
        ch0 += 55;
    }
    fputc(ch0, fd);
    ch0 = (ch & 0x0f);
    if (ch0 < 10) {
        ch0 += 48;
    }
    else {
        ch0 += 55;
    }
    fputc(ch0, fd);
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