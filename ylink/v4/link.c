// YLINK - the Ypsilon Executable Linker
// Copyright 2016 Zane Wagner. All rights reserved.
#include "stdio.h"
#include "notice.h"
#include "link.h"

#define LINEMAX  127
#define LINESIZE 128
#define OBJF_MAX 8

char *line;
char *exeFile;
int objf_ptrs[OBJF_MAX];
int objf_cnt;
byte isLibrary = 0, hasDependancy = 0, idxLibMod = 0;

main(int argc, int *argv) {
  int i;
  puts(VERSION);
  Setup();
  RdArgs(argc, argv);
  Initialize();
  ReadObjfs();
}

Setup() {
  line = AllocVar(LINESIZE, 1);
  AllocLnkMemory();
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

RdArgs(int argc, int *argv) {
  int i;
  if (argc == 1) {
    fatal("No argments passed.");
  }
  for (i = 1; i < argc; i++) {
    char* c;
    getarg(i, line, LINESIZE, argc, argv);
    c = line;
    if (*c == '-') {
      // option -x=xxx
      if (*(c+2) != '=') {
        fatalf("Missing '=' in option %s", c);
      }
      switch (*(c+1)) {
        case 'e':
          RdArgExe(c+3);
          break;
        default:
          fatalf("Could not parse option %s", c);
          break;
      }
    }
    else {
      // read in object files
      char *start, *end;
      start = end = c;
      while (1) {
        if ((*end == 0) || (*end == ',')) {
          RdArgObjf(start, end);
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

RdArgObjf(char *start, char *end) {
  char *objf;
  if (objf_cnt == OBJF_MAX) {
    fatalf("  Error: max of %u input obj files.", objf_cnt);
  }
  objf_ptrs[objf_cnt] = AllocVar(end - start + 1, 1);
  objf = objf_ptrs[objf_cnt];
  while (start != end) {
    *objf++ = *start++;
  }
  *objf = 0;
  objf_cnt += 1;
}

RdArgExe(char *str) {
  exeFile = AllocVar(strlen(str) + 1, 1);
  strcpy(exeFile, str);
}

// === Reading OBJ Format =====================================================

ReadObjfs() {
  uint i, fd;
  for (i = 0; i < objf_cnt; i++) {
    printf("  Reading %s... ", objf_ptrs[i]);
    if (!(fd = fopen(objf_ptrs[i], "r"))) {
      fatalf("Could not open file '%s'", objf_ptrs[i]);
    }
    ReadFile(fd);
    Cleanup(fd);
    puts("Done.");
  }
}

ReadFile(uint fd) {
  uint outfd;
  byte recType;
  uint length;
  outfd = fopen(LINKTXT, "a");
  isLibrary = idxLibMod = 0; // reset library vars.
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
  fclose(outfd);
  return;
}

Cleanup(uint fd) {
  if (fd != 0) {
    fclose(fd);
  }
  return;
}

// === Write Hexidecimal numbers ==============================================

write_x16(uint fd, uint value) {
  write_x8(fd, value >> 8);
  write_x8(fd, value & 0x00ff);
  return;
}

write_x8(uint fd, byte value) {
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

fatal(char *str) {
  errout(str);
  abort(1);
}

fatalf(char *format, char *arg) {
  erroutf(format, arg);
  abort(1);
}

errout(char *str) {
    fputs("    Error: ", stderr);
    fputs(str, stderr);
    fputc(NEWLINE, stderr);
}

erroutf(char *format, char *arg) {
    fputs("    Error: ", stderr); 
    fprintf(stderr, format, arg);
    fputc(NEWLINE, stderr);
}

// === Memory Management ======================================================

AllocVar(int nItems, int itemSize) {
  int result;
  result = calloc(nItems, itemSize);
  if (result == 0) {
    printf("Could not allocate mem: %u x %u\n", nItems, itemSize);
    abort(0);
  }
}
