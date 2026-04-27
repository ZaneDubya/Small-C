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

#include <stdio.h>
#include "cc.h"

extern char *symtab, *locptr, *lptr, ssname[];
extern int ch, eof, rettype, rettypeSubPtr, lastPtrDepth;
extern int lastNdim, lastStrides[];
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
// return pointer to tag definition, or -1 on error.
int doStruct(int kind) {
  int i, totalsize, typeSubIdx;
  int cur_unit_offset, next_free_bit;
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
  /* Copy struct tag name; bound prevents writing past STRDAT_NAME's slot. */
  for (i = 0; an(ssname[i]) && i < STRDAT_MAX - STRDAT_NAME; i++)
    structdatnext[STRDAT_NAME + i] = ssname[i];
  structdatnext[STRDAT_NAME + i] = 0;
  structdatnext[STRDAT_KIND] = (char)kind;
  totalsize = 0;
  cur_unit_offset = -1;
  next_free_bit = 0;
  while (isMatch("}") == 0) {
    int id, sz, type;
    int is_bf, bit_width, bit_off, byte_off, skip_entry;
    if (ch == 0 && eof) {
      error("no final }");
      return;
    }
    if ((type = dotype(&typeSubIdx)) != 0) {
      while (1) {
        is_bf = bit_width = bit_off = byte_off = skip_entry = 0; /* one GETw1n 0, five stores */

        /* Detect unnamed bit-field: type followed directly by ':' */
        blanks();
        if (ch == ':') {
          ssname[0] = 0;
          id = IDENT_VARIABLE;
          sz = BPW;
          lastPtrDepth = 0;
        }
        else {
          parseLocDecl(type, typeSubIdx, IDENT_ARRAY, &id, &sz);
        }

        /* Duplicate-name check (skip for unnamed members) */
        if (ssname[0] && getStructMember(structdatnext, ssname)) {
          error("duplicate member name");
          abort(1);
        }

        /* Check for bit-field width specifier ':' */
        if (isMatch(":")) {
          is_bf = 1;
          if (lastPtrDepth != 0) {
            error("bit-field cannot be a pointer");
            bit_width = 1;
          }
          else if (id == IDENT_ARRAY) {
            error("bit-field cannot be an array");
            bit_width = 1;
          }
          else if (type == TYPE_LONG || type == TYPE_ULONG) {
            error("bit-field cannot have long type");
            bit_width = 1;
          }
          else if (type == TYPE_STRUCT) {
            error("bit-field cannot have struct type");
            bit_width = 1;
          }
          else if (type == TYPE_VOID) {
            error("bit-field cannot have void type");
            bit_width = 1;
          }
          else {
            isConstExpr(&bit_width);
          }
          if (bit_width < 0) {
            error("negative bit-field width");
            bit_width = 1;
          }
          if (bit_width > 16) {
            error("bit-field width exceeds 16");
            bit_width = 16;
          }
          if (bit_width == 0 && ssname[0]) {
            error("named bit-field cannot have zero width");
            bit_width = 1;
          }
        }

        /* Layout and emit */
        if (is_bf) {
          if (bit_width == 0) {
            /* Unnamed zero-width: force-align to next allocation unit */
            if (kind == KIND_STRUCT && cur_unit_offset != -1) {
              totalsize = cur_unit_offset + BPW;
              cur_unit_offset = -1;
              next_free_bit = 0;
            }
            skip_entry = 1;
          }
          else if (kind == KIND_UNION) {
            byte_off = 0;
            bit_off  = 0;
            if (BPW > totalsize) totalsize = BPW;
            if (!ssname[0]) skip_entry = 1;
          }
          else {
            /* STRUCT: pack into current unit or open a new one */
            if (cur_unit_offset == -1 || next_free_bit + bit_width > 16) {
              if (cur_unit_offset == -1)
                cur_unit_offset = totalsize;
              else
                cur_unit_offset = cur_unit_offset + BPW;
              totalsize = cur_unit_offset + BPW;
              next_free_bit = 0;
            }
            byte_off = cur_unit_offset;
            bit_off  = next_free_bit;
            next_free_bit += bit_width;
            if (!ssname[0]) skip_entry = 1;
          }
        }
        else {
          /* Normal member: close any open bit-field unit (STRUCT only) */
          if (kind == KIND_STRUCT && cur_unit_offset != -1) {
            totalsize = cur_unit_offset + BPW;
            cur_unit_offset = -1;
            next_free_bit = 0;
          }
        }

        if (!skip_entry) {
          if (is_bf) {
            putint(byte_off, structmemnext + STRMEM_OFFSET, 2);
            structmemnext[STRMEM_IDENT]    = (char)id;
            structmemnext[STRMEM_TYPE]     = (char)type;
            putint(BPW, structmemnext + STRMEM_SIZE, 2);
            structmemnext[STRMEM_BITWIDTH] = (char)bit_width;
            structmemnext[STRMEM_BITOFF]   = (char)bit_off;
          }
          else {
            if (kind == KIND_UNION) {
              putint(0, structmemnext + STRMEM_OFFSET, 2);
              if (sz > totalsize) totalsize = sz;
            }
            else {
              putint(totalsize, structmemnext + STRMEM_OFFSET, 2);
              totalsize += sz;
            }
            structmemnext[STRMEM_IDENT] = (char)id;
            structmemnext[STRMEM_TYPE]  = (char)type;
            putint(sz, structmemnext + STRMEM_SIZE, 2);
            if (type == TYPE_STRUCT) {
              putint(typeSubIdx, structmemnext + STRMEM_CLASSPTR, 2);
              structmemnext[STRMEM_PTRDEPTH] = (char)lastPtrDepth;
            }
            else if (id == IDENT_ARRAY && lastNdim > 1) {
              // Multi-dim array member: repurpose STRMEM_PTRDEPTH to hold NDIM
              // and STRMEM_CLASSPTR to hold the dimdata pointer.
              // (Pointer depth is always 0 for plain arrays and is not needed here.)
              putint((int)storeDimDat(typeSubIdx, lastNdim, lastStrides),
                     structmemnext + STRMEM_CLASSPTR, 2);
              structmemnext[STRMEM_PTRDEPTH] = (char)lastNdim;
            }
            else {
              structmemnext[STRMEM_PTRDEPTH] = (char)lastPtrDepth;
            }
          }
          /* Copy member name (common to both paths); max 12 chars + null */
          i = 0;
          while (an(ssname[i]) && i < 12) {
            structmemnext[STRMEM_NAME + i] = ssname[i++];
          }
          structmemnext[STRMEM_NAME + i] = 0;
          structmemnext += STRMEM_MAX;
        }

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
  /* Close any open bit-field unit at end of struct */
  if (kind == KIND_STRUCT && cur_unit_offset != -1) {
    totalsize = cur_unit_offset + BPW;
  }
  putint(totalsize, structdatnext + STRDAT_SIZE, 2);
  putint(structmemnext, structdatnext + STRDAT_MEND, 2);
  structdatnext += STRDAT_MAX;
  return structdatnext - STRDAT_MAX;
}

// Find a tag definition by name and kind (KIND_STRUCT or KIND_UNION).
// return pointer to tag data if found, else -1
int findStructByName(int kind, char *sname) {
  char *current;
  for (current = STRDAT_START; current < structdatnext; current += STRDAT_MAX) {
    if (current[STRDAT_KIND] == kind
        && strcmp(current + STRDAT_NAME, sname) == 0)
      return current;
  }
  return -1;
}

// Determine if the next token is a tag name matching the given kind.
// Consumes the tag name from input if found.
// kind: KIND_STRUCT or KIND_UNION
// @return pointer to tag data if found, else (char *)-1
char *getStructPtr(int kind) {
  char *current;
  // strlen is called only when STRDAT_KIND matches; no len local needed.
  for (current = STRDAT_START; current < structdatnext; current += STRDAT_MAX) {
    if (current[STRDAT_KIND] == kind)
      if (amatch(current + STRDAT_NAME, strlen(current + STRDAT_NAME)))
        return current;
  }
  return (char *)-1;
}

int getStructSize(char *basestruct) {
  return getint(basestruct + STRDAT_SIZE, 2);
}

char *getStructMember(char *basestruct, char *sname) {
  char *current;
  for (current = getptr(basestruct + STRDAT_MBEG, 2);
       current < structmemnext;
       current += STRMEM_MAX) {
    if (strcmp(current + STRMEM_NAME, sname) == 0)
      return current;
  }
  return (char *)0;
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
