// ============================================================================
// Small-C Compiler -- Support for Structures, physically grouped collection of
//                     variables placed under one name in a block of memory,
//                     allowing the variables to be accessed via one pointer,
//                     or a declaration which returns the same address.
// Copyright 2017, 2018 Zane Wagner
// All rights reserved.
// ----------------------------------------------------------------------------
// Notice of Public Domain Status
// The source code for the Small-C Compiler and runtime libraries (CP/M & DOS),
// Small-Mac Assembler (CP/M), Small-Assembler (DOS), Small-Tools programs and
// Small-Windows library to which I hold copyrights are hereby available for
// royalty free use in private or commerical endeavors. The only obligation
// being that the users retain the original copyright notices and credit all
// prior authors in derivative versions.
// Zane Wagner
// ============================================================================

// Structure Definitions are stored in the Structure Table (structdata).
// Declared structures are stored in the Symbol Tables, like other variables.
// Struct decls have values IDENT == IDENT_VARIABLE/IDENT_ARRAY/IDENT_POINTER,
//                        TYPE == TYPE_STRUCT,
//                        CLASS == STATIC/AUTOMATIC/EXTERNAL
//                        CLASSIDX == index of structure definition.

#include <stdio.h>
#include "cc.h"
#include "ccstruct.h"

extern char *symtab, *locptr, ssname[];
char *structdata, *structdatnext, *structmemnext;

initStructs() {
  structdata = calloc(STRUCTDATSZ, 1);
  structdatnext = STRDAT_START;
  structmemnext = STRMEM_START;
}

/**
 * define a single struct. this will only be called from parse()
 */
doStruct() {
  int i, totalsize, typeSubIdx;
  blanks();
  if (symname(ssname)) {
    blanks();
  }
  else {
    illname();
    return;
  }
  if (IsMatch("{") == 0) {
    error("no open bracket");
    abort(1);
  }
  putint(structmemnext, structdatnext + STRDAT_MBEG, 2);
  // copy name
  i = 0;
  while (an(ssname[i])) {
    structdatnext[STRDAT_NAME + i] = ssname[i++];
    if (STRDAT_NAME + i == STRDAT_MAX) {
      break;
    }
  }
  structdatnext[STRDAT_NAME + i] = 0;
  totalsize = 0;
  while (IsMatch("}") == 0) {
    int id, sz, type;
    if (ch == 0 && eof) {
      error("no final }");
      return;
    }
    if ((type = dotype(&typeSubIdx)) != 0) {
      while (1) {
        parseLocalDeclare(type, typeSubIdx, IDENT_ARRAY, &id, &sz);
        if (getStructMember(structdatnext, ssname)) {
          error("duplicate struct member name");
          abort(1);
        }
        putint(totalsize, structmemnext + STRMEM_OFFSET, 2);
        structmemnext[STRMEM_IDENT] = id;
        structmemnext[STRMEM_TYPE] = type;
        putint(sz, structmemnext + STRMEM_SIZE, 2);
        totalsize += sz;
        // copy name
        i = 0;
        while (an(ssname[i])) {
          structmemnext[STRMEM_NAME + i] = ssname[i++];
          if (STRMEM_NAME + i == STRMEM_MAX) {
            break;
          }
        }
        structmemnext[STRMEM_NAME + i] = 0;
        structmemnext += STRMEM_MAX;
        if (IsMatch(",") == 0)
          break;
        if (endst()) 
          break;
      }
      ReqSemicolon();
    }
    else {
      error("struct may only contain members");
      abort(1);
    }
  }
  ReqSemicolon();
  putint(totalsize, structdatnext + STRDAT_SIZE, 2);
  putint(structmemnext, structdatnext + STRDAT_MEND, 2);
  structdatnext += STRDAT_MAX;
}

/**
 * determine if 'sname' is the tag name of a struct definition
 * @param sname
 * @return pointer to struct data if found, else -1
 */
getStructPtr() {
  char *current, *end;
  int i, len;
  current = STRDAT_START;
  end = structdatnext;
  i = 0;
  while (current < end) {
    len = strlen(current + STRDAT_NAME);
    if (amatch(current + STRDAT_NAME, len)) {
      return current;
    }
    current += STRDAT_MAX;
    i += 1;
  }
  return -1;
}

getStructSize(char *basestruct) {
  return getint(basestruct + STRDAT_SIZE, 2);
}

getStructMember(char *basestruct, char *sname) {
  char *current, *end;
  current = getint(basestruct + STRDAT_MBEG, 2);
  end = structmemnext;
  // printf("    searching from %x to %x\n", current, end);
  while (current < end) {
    // printf("      cmp %s and %s\n", current + STRMEM_NAME, sname);
    if (strcmp(current + STRMEM_NAME, sname) == 0) {
      return current;
    }
    current += STRMEM_MAX;
  }
  return 0;
}

/**
 * prints debug information about all struct definitions and all members
 */
printallstructs() {
  char *istruct, *istructmem, *istructend;
  int ident, type, size, offset;
  istruct = STRDAT_START;
  istructmem = STRMEM_START;
  while (istruct < structdatnext) {
    istructmem = getint(istruct + STRDAT_MBEG, 2);
    istructend = getint(istruct + STRDAT_MEND, 2);
    size = getint(istruct + STRDAT_SIZE, 2);
    printf("struct %s defined at %x (sz=%u)\n", istruct + STRDAT_NAME, istruct, size);
    while (istructmem < istructend) {
      ident = istructmem[STRMEM_IDENT];
      type = istructmem[STRMEM_TYPE];
      size = getint(istructmem + STRMEM_SIZE, 2);
      offset = getint(istructmem + STRMEM_OFFSET, 2);
      printf("  member %s at %x (sz=%u id=%u typ=%u ofs=%u)\n", 
        istructmem + STRMEM_NAME, istructmem, size, ident, type, offset);
      istructmem += STRMEM_MAX;
    }
    istruct += STRDAT_MAX;
  }
}
