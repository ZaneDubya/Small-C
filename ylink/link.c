// YLINK - the Ypsilon Executable Linker
// Copyright 2016 Zane Wagner. All rights reserved.
#include "stdio.h"
#include "notice.h"
#include "link.h"

#define LINEMAX  127
#define LINESIZE 128
#define ERROR 0xFFFF
#define OBJF_MAX 8
#define LINKTXT "link.txt"

char *line;
char *exeFile;
int objf_ptrs[OBJF_MAX];
int objf_cnt;
extern byte isLibrary;

main(int argc, int *argv) {
  int i;
  puts(VERSION);
  Setup();
  if (GetArgs(argc, argv) == ERROR) {
    abort(0);
  }
  Initialize();
  for (i = 0; i < objf_cnt; i++) {
    uint fd;
    printf("  Reading %s... ", objf_ptrs[i]);
    if (!(fd = fopen(objf_ptrs[i], "r"))) {
      puts("Could not open file.");
      abort(0);
    }
    ReadFile(fd);
    Cleanup(fd);
    puts("Done.");
  }
}

Setup() {
  line = AllocVar(LINESIZE, 1);
  exeFile = 0;
}

Initialize() {
  unlink(LINKTXT);
  if (exeFile == 0) {
    printf("  No -e parameter. Output file will be out.exe.\n");
    exeFile = AllocVar(8, 1);
    strcpy(exeFile, "out.exe");
  }
}

AllocVar(int nItems, int itemSize) {
  int result;
  result = calloc(nItems, itemSize);
  if (result == 0) {
    printf("Could not allocate mem: %u x %u\n", nItems, itemSize);
    abort(0);
  }
}

GetArgs(int argc, int *argv) {
  int i;
  if (argc == 1) {
    errout("No argments passed.");
    return ERROR;
  }
  for (i = 1; i < argc; i++) {
    char* c;
    getarg(i, line, LINESIZE, argc, argv);
    c = line;
    if (*c == '-') {
      // option -x=xxx
      if (*(c+2) != '=') {
        erroutf("Missing '=' in option %s\n", c);
        return ERROR;
      }
      switch (*(c+1)) {
        case 'e':
          RdOptE(c+3);
          break;
        default:
          erroutf("Could not parse option %s\n", c);
          return ERROR;
          break;
      }
    }
    else {
      // read in object files
      char *start, *end;
      start = end = c;
      while (1) {
        if ((*end == 0) || (*end == ',')) {
          AddObjf(start, end);
          if (*end == 0) {
            break;
          }
          start = ++end;
        }
        end++;
      }
      c = end;
    }
  }
}

AddObjf(char *start, char *end) {
  char *objf;
  if (objf_cnt == OBJF_MAX) {
    printf("  Error: max of %u input obj files.", objf_cnt);
    abort(0);
  }
  objf_ptrs[objf_cnt] = AllocVar(end - start + 1, 1);
  objf = objf_ptrs[objf_cnt];
  while (start != end) {
    *objf++ = *start++;
  }
  *objf = 0;
  objf_cnt += 1;
}

RdOptE(char *str) {
  exeFile = AllocVar(strlen(str) + 1, 1);
  strcpy(exeFile, str);
}

Cleanup(uint fd) {
  if (fd != 0) {
    fclose(fd);
  }
  return;
}

// === Reading OBJ Format =====================================================

ReadFile(uint fd) {
  uint outfd;
  byte recType;
  uint length;
  outfd = fopen(LINKTXT, "a");
  isLibrary = 0;
  while (1) {
    recType = read_u8(fd);
    if (feof(fd)) {
      break;
    }
    if (ferror(fd)) {
      break;
    }
    length = read_u16(fd);
    do_record(outfd, recType, length, fd);
  }
  if (isLibrary) {
    writeLibData(outfd);
  }
  fclose(outfd);
  return;
}

puthexint(uint fd, uint value) {
  puthexch(fd, value >> 8);
  puthexch(fd, value & 0x00ff);
  return;
}

puthexch(uint fd, byte value) {
  byte ch0;
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
read_u8(uint fd) {
    byte ch;
    ch = _read(fd);
    return ch;
}

read_u16(uint fd) {
    uint i;
    i = (_read(fd) & 0x00ff);
    i += (_read(fd) & 0x00ff) << 8;
    return i;
}

// read string that is prefixed by length.
read_strpre(char* str, uint fd) {
    byte length, retval;
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

offsetfd(uint fd, uint base[], uint offset[]) {
    bseek(fd, base, 0);
    bseek(fd, offset, 1);
}

// === Error Routines =========================================================

errout(char *str) {
    fputs("    Error: ", stderr); 
    lineout(str, stderr);
}

erroutf(char *format, char *arg) {
    fputs("    Error: ", stderr); 
    printf(format, arg);
}

lineout(char *str, uint fd) {
    fputs(str, fd);
    fputc(NEWLINE, fd);
}