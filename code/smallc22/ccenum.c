// ============================================================================
// Small-C Compiler -- Support for Enumerations.
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
//
// Enum constants are stored in the global symbol table as:
//   IDENT == IDENT_VARIABLE
//   TYPE  == TYPE_INT
//   CLASS == ENUMCONST
//   SIZE  == BPW
//   OFFSET == the constant's integer value
//
// No code or data is emitted for enum constants; primary() in cc3.c
// recognises ENUMCONST entries and folds them as compile-time integers.

#include "stdio.h"
#include "cc.h"
#include "ccenum.h"

extern char ssname[], *glbptr;
extern int  ch, eof;

char *enumdata, *enumdatnext;

// Allocate and zero the enum tag table.
void initEnums() {
    enumdata = calloc(ENUMDATSZ, 1);
    enumdatnext = EDAT_START;
}

// Find a defined enum tag by name.
// Returns pointer to its EDAT entry, or -1 if not found.
int findEnumTag(char *name) {
    char *cur;
    cur = EDAT_START;
    while (cur < enumdatnext) {
        if (strcmp(cur + EDAT_NAME, name) == 0)
            return cur;
        cur += EDAT_MAX;
    }
    return -1;
}

// Register a new enum tag name.
// Errors on duplicate tag or table overflow.
void addEnumTag(char *name) {
    int i;
    if (findEnumTag(name) != -1) {
        error("enum tag already defined");
        return;
    }
    if (enumdatnext >= EDAT_END) {
        error("enum tag table overflow");
        return;
    }
    i = 0;
    while (name[i] && i < NAMEMAX) {
        enumdatnext[EDAT_NAME + i] = name[i];
        i++;
    }
    enumdatnext[EDAT_NAME + i] = 0;
    enumdatnext += EDAT_MAX;
}

// Add an enum constant to the global symbol table.
// The integer value is stored in the OFFSET field.
// No code or data segment bytes are emitted.
void addEnumConst(char *name, int value) {
    if (findglb(name)) {
        error("enum constant already defined");
        return;
    }
    AddSymbol(name, IDENT_VARIABLE, TYPE_INT, BPW, value, &glbptr, ENUMCONST);
}

// Parse an enum specifier after the 'enum' keyword has been consumed.
// Defines enumerator constants and optionally registers a tag name.
// Sets *typeSubPtr = 0 (enum variables carry no extra metadata).
// Returns TYPE_INT (enums are backed by int).
int doEnum(int *typeSubPtr) {
    char tagname[NAMESIZE];
    int  hasTag, nextVal, done;

    *typeSubPtr = 0;
    blanks();

    // Optional tag name
    hasTag = symname(tagname);
    if (hasTag && isreserved(tagname)) {
        error("reserved keyword used as enum tag");
        hasTag = 0;
    }

    if (IsMatch("{")) {
        // --- Definition with body ---
        if (hasTag)
            addEnumTag(tagname);

        nextVal = 0;
        done = 0;
        while (!done) {
            if (IsMatch("}")) {
                done = 1;
                break;
            }
            if (ch == 0 && eof) {
                error("no final }");
                return TYPE_INT;
            }
            if (symname(ssname) == 0) {
                error("expected enumerator name");
                skipToNextToken();
                break;
            }
            if (isreserved(ssname)) {
                error("reserved keyword used as enumerator");
            }
            if (IsMatch("=")) {
                IsConstExpr(&nextVal);
            }
            addEnumConst(ssname, nextVal);
            nextVal++;
            if (IsMatch(",") == 0) {
                Require("}");
                done = 1;
            }
        }
    }
    else {
        // --- Tag-only reference (no body) ---
        if (!hasTag) {
            error("enum tag name required without body");
        }
        else if (findEnumTag(tagname) == -1) {
            error("unknown enum tag");
        }
    }

    return TYPE_INT;
}
