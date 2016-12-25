// YLINK - the Ypsilon Executable Linker
// Copyright 2016 Zane Wagner. All rights reserved.
#include "stdio.h"
#include "notice.h"
#define LINEMAX  127
#define LINESIZE 128
#define ERROR 0xFFFF

char    *line;

main(int argc, int *argv) {
    unsigned int fd;
    fputs(VERSION, stderr);
    fputs(CRIGHT1, stderr);
    allocatevars();
    if ((fd = ask(argc, argv)) == ERROR) {
        abort(1);
    }
    readFile(fd);
    cleanup(fd);
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
    int input;
    if (argc != 2) {
        errout("Requires exactly one argument.");
        return ERROR;
    }
    getarg(1, line, LINESIZE, argc, argv);
    if(!(input = fopen(line, "r"))) {
        erroutf("Could not open file: ", line);
        return ERROR;
    }
	return input;
}

cleanup(unsigned int fd) {
    if (fd != 0) {
        fclose(fd);
    }
    return;
}

// === Reading OBJ Format =====================================================

readFile(unsigned int fd) {
    unsigned char recType;
    unsigned int length;
    while (1) {
        recType = read_u8(fd);
        if (feof(fd)) {
            fputs("eof", stderr);
            break;
        }
        if (ferror(fd)) {
            fputs("ferr", stderr);
            break;
        }
        length = read_u16(fd);
        do_record(recType, length, fd);
    }
    return;
}

prnhexint(unsigned int value, unsigned int fd) {
    prnhexch(value >> 8, fd);
    prnhexch(value & 0x00ff, fd);
    return;
}

prnhexch(unsigned char value, unsigned int fd) {
    unsigned char ch0;
    ch0 = (value & 0xf0) >> 4;
    if (ch0 < 10) {
        ch0 += 48;
    }
    else {
        ch0 += 55;
    }
    fputc(ch0, fd);
    ch0 = (value & 0x0f);
    if (ch0 < 10) {
        ch0 += 48;
    }
    else {
        ch0 += 55;
    }
    fputc(ch0, fd);
    return;
}

// === Binary Reading Routines ================================================
read_u8(unsigned int fd) {
    unsigned char ch;
    ch = _read(fd);
    return ch;
}

read_u16(unsigned int fd) {
    unsigned int i;
    i = (_read(fd) & 0x00ff);
    i += (_read(fd) & 0x00ff) << 8;
    return i;
}

// read string that is prefixed by length.
read_strpre(char* str, unsigned int fd) {
    unsigned char length, retval;
    char* next;
    next = str;
    retval = length = read_u8(fd);
    while (length > 0) {
        *next++ = read_u8(fd);
        length--;
    }
    *next++ = NULL;
    return retval + 1;
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

lineout(char *line, unsigned int fd) {
    fputs(line, fd);
    fputc(NEWLINE, fd);
}