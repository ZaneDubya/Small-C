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

// Parse an enum specifier after the 'enum' keyword has been consumed.
// Defines enumerator constants and optionally registers a tag name.
// Sets *typeSubPtr = 0 (enum variables carry no extra metadata).
// Returns TYPE_INT (enums are backed by int).
int doEnum(int *typeSubPtr) {
    char tagname[NAMESIZE];
    int  hasTag, nextVal, i;

    *typeSubPtr = 0;
    blanks();

    // Optional tag name
    hasTag = symname(tagname);
    if (hasTag && isreserved(tagname)) {
        error("reserved keyword used as enum tag");
        hasTag = 0;
    }

    if (isMatch("{")) {
        // --- Register tag name (was addEnumTag, inlined) ---
        if (hasTag) {
            if (findEnumTag(tagname) != -1) {
                error("enum tag already defined");
            }
            else if (enumdatnext >= EDAT_END) {
                error("enum tag table overflow");
            }
            else {
                i = 0;
                while (tagname[i] && i < NAMEMAX) {
                    enumdatnext[EDAT_NAME + i] = tagname[i];
                    i++;
                }
                enumdatnext[EDAT_NAME + i] = 0;
                enumdatnext += EDAT_MAX;
            }
        }

        // --- Parse enumerator list ---
        nextVal = 0;
        for (;;) {
            if (isMatch("}"))
                break;
            if (ch == 0 && eof) {
                error("no final }");
                return TYPE_INT;
            }
            if (symname(ssname) == 0) {
                error("expected enumerator name");
                skipToNextToken();
                break;
            }
            if (isreserved(ssname))
                error("reserved keyword used as enumerator");
            if (isMatch("="))
                isConstExpr(&nextVal);
            // addEnumConst inlined:
            if (findglb(ssname))
                error("enum constant already defined");
            else
                addSymbol(ssname, IDENT_VARIABLE, TYPE_INT, BPW, nextVal, &glbptr, ENUMCONST);
            nextVal++;
            if (isMatch(",") == 0) {
                require("}");
                break;
            }
        }
    }
    else {
        // --- Tag-only reference (no body) ---
        if (!hasTag)
            error("enum tag name required without body");
        else if (findEnumTag(tagname) == -1)
            error("unknown enum tag");
    }

    return TYPE_INT;
}