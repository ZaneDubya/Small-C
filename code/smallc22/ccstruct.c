// ============================================================================
// Small-C Compiler -- Support for Structures, physically grouped collection of
//                     variables placed under one name in a block of memory,
//                     allowing the variables to be accessed via one pointer,
//                     or a declaration which returns the same address.
// Copyright 2026 Zane Wagner.
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

#include "stdio.h"
#include "cc.h"
#include "ccstruct.h"

extern char *symtab, *locptr, *lptr, ssname[];
extern int ch, eof, rettype, rettypeSubPtr;
char *structdata, *structdatnext, *structmemnext;

// forward declarations for this file:
int doStruct(int kind);
int findStructByName(int kind, char *sname);

void initStructs() {
  structdata = calloc(STRUCTDATSZ, 1);
  structdatnext = STRDAT_START;
  structmemnext = STRMEM_START;
}

// Handle a struct or union keyword at global scope.
// Called from parseTopLvl() after 'struct'/'union' has been matched.
int doStructBlock(int kind) {
  int sp;
  char *p;
  sp = doStruct(kind);
  if (sp != -1 && !endst()) {
    blanks();
    p = lptr;
    if (alpha(*p)) {
      while (an(*p)) p++;
      while (*p == ' ' || *p == '\t') p++;
      if (*p == '(') {
        rettype = TYPE_STRUCT;
        rettypeSubPtr = sp;
        dofunction(GLOBAL);
        rettype = TYPE_INT;
        rettypeSubPtr = 0;
        return;
      }
    }
    parseGlbDecl(TYPE_STRUCT, GLOBAL, sp);
  }
  reqSemicolon();
}

// Define a single struct or union if '{' follows the tag name,
// otherwise look up an existing definition by tag name.
// kind: KIND_STRUCT or KIND_UNION
// @return pointer to tag definition, or -1 on error.
int doStruct(int kind) {
  int i, totalsize, typeSubIdx;
  blanks();
  if (symname(ssname)) {
    if (isreserved(ssname)) {
      error("reserved keyword used as name");
      return -1;
    }
    blanks();
  }
  else {
    illname();
    return -1;
  }
  if (isMatch("{") == 0) {
    return findStructByName(kind, ssname);
  }
  putint(structmemnext, structdatnext + STRDAT_MBEG, 2);
  i = 0;
  while (an(ssname[i])) {
    structdatnext[STRDAT_NAME + i] = ssname[i++];
    if (STRDAT_NAME + i == STRDAT_MAX) {
      break;
    }
  }
  structdatnext[STRDAT_NAME + i] = 0;
  structdatnext[STRDAT_KIND] = kind;
  totalsize = 0;
  while (isMatch("}") == 0) {
    int id, sz, type;
    if (ch == 0 && eof) {
      error("no final }");
      return;
    }
    if ((type = dotype(&typeSubIdx)) != 0) {
      while (1) {
        parseLocDecl(type, typeSubIdx, IDENT_ARRAY, &id, &sz);
        if (getStructMember(structdatnext, ssname)) {
          error("duplicate member name");
          abort(1);
        }
        if (kind == KIND_UNION) {
          putint(0, structmemnext + STRMEM_OFFSET, 2);
          if (sz > totalsize) totalsize = sz;
        }
        else {
          putint(totalsize, structmemnext + STRMEM_OFFSET, 2);
          totalsize += sz;
        }
        structmemnext[STRMEM_IDENT] = id;
        structmemnext[STRMEM_TYPE] = type;
        putint(sz, structmemnext + STRMEM_SIZE, 2);
        i = 0;
        while (an(ssname[i])) {
          structmemnext[STRMEM_NAME + i] = ssname[i++];
          if (STRMEM_NAME + i == STRMEM_CLASSPTR) {
            break;
          }
        }
        structmemnext[STRMEM_NAME + i] = 0;
        if (type == TYPE_STRUCT) {
          putint(typeSubIdx, structmemnext + STRMEM_CLASSPTR, 2);
        }
        structmemnext += STRMEM_MAX;
        if (isMatch(",") == 0)
          break;
        if (endst())
          break;
      }
      reqSemicolon();
    }
    else {
      error("tag may only contain members");
      abort(1);
    }
  }
  putint(totalsize, structdatnext + STRDAT_SIZE, 2);
  putint(structmemnext, structdatnext + STRDAT_MEND, 2);
  structdatnext += STRDAT_MAX;
  return structdatnext - STRDAT_MAX;
}

// Find a tag definition by name and kind (KIND_STRUCT or KIND_UNION).
// @param kind  KIND_STRUCT or KIND_UNION
// @param sname the tag name to search for
// @return pointer to tag data if found, else -1
int findStructByName(int kind, char *sname) {
  char *current, *end;
  current = STRDAT_START;
  end = structdatnext;
  while (current < end) {
    if (current[STRDAT_KIND] == kind
        && strcmp(current + STRDAT_NAME, sname) == 0) {
      return current;
    }
    current += STRDAT_MAX;
  }
  return -1;
}

// Determine if the next token is a tag name matching the given kind.
// Consumes the tag name from input if found.
// kind: KIND_STRUCT or KIND_UNION
// @return pointer to tag data if found, else -1
int getStructPtr(int kind) {
  char *current, *end;
  int len;
  current = STRDAT_START;
  end = structdatnext;
  while (current < end) {
    if (current[STRDAT_KIND] == kind) {
      len = strlen(current + STRDAT_NAME);
      if (amatch(current + STRDAT_NAME, len)) {
        return current;
      }
    }
    current += STRDAT_MAX;
  }
  return -1;
}

int getStructSize(char *basestruct) {
  return getint(basestruct + STRDAT_SIZE, 2);
}

int getStructMember(char *basestruct, char *sname) {
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

// prints debug information about all struct definitions and all members
/*printallstructs() {
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
}*/
