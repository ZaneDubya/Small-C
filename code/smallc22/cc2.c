// ============================================================================
// Small-C Compiler -- Part 2 -- Front End and Miscellaneous.
// Copyright 1982, 1983, 1985, 1988 J. E. Hendrix.
// All rights reserved.
// ----------------------------------------------------------------------------
// Notice of Public Domain Status
// The source code for the Small-C Compiler and runtime libraries (CP/M & DOS),
// Small-Mac Assembler (CP/M), Small-Assembler (DOS), Small-Tools programs and
// Small-Windows library to which I hold copyrights are hereby available for
// royalty free use in private or commerical endeavors. The only obligation
// being that the users retain the original copyright notices and credit all
// prior authors (Ron Cain, James Hendrix, etc.) in derivative versions.
// James E. Hendrix Jr.
// ============================================================================

#include <stdio.h>
#include "cc.h"

extern char
*symtab, *macn, *macq, *pline, *mline, optimize,
alarm, *glbptr, *line, *lptr, *cptr, *cptr2, *cptr3,
*locptr, msname[NAMESIZE], pause;

extern int
*wq, ccode, ch, csp, eof, errflag, iflevel,
input, input2, listfp, macptr, nch,
nxtlab, op[16], opindex, opsize, output, pptr,
skiplevel, *wqptr;

/********************** input functions **********************/

preprocess() {
    int k;
    char c;
    if (ccode) {
        line = mline;
        ifline();
        if (eof) return;
    }
    else {
        inline();
        return;
    }
    pptr = -1;
    while (ch != NEWLINE && ch) {
        if (white()) {
            keepch(' ');
            while (white()) gch();
        }
        else if (ch == '"') {
            keepch(ch);
            gch();
            while (ch != '"' || (*(lptr - 1) == 92 && *(lptr - 2) != 92)) {
                if (ch == NULL) {
                    error("no quote");
                    break;
                }
                keepch(gch());
            }
            gch();
            keepch('"');
        }
        else if (ch == 39) {
            keepch(39);
            gch();
            while (ch != 39 || (*(lptr - 1) == 92 && *(lptr - 2) != 92)) {
                if (ch == NULL) {
                    error("no apostrophe");
                    break;
                }
                keepch(gch());
            }
            gch();
            keepch(39);
        }
        else if (ch == '/' && nch == '*') {
            bump(2);
            while ((ch == '*' && nch == '/') == 0) {
                if (ch) bump(1);
                else {
                    ifline();
                    if (eof) break;
                }
            }
            bump(2);
        }
        /* ignore C99-style comments */
        else if (ch == '/' && nch == '/') {
            bump(2);
            while ((ch == LF || ch == CR) == 0) {
                if (ch) bump(1);
                else {
                    ifline();
                    if (eof) break;
                }
            }
            bump(1);
            if (ch == LF) {
                bump(1);
            }
        }
        else if (an(ch)) {
            k = 0;
            while (an(ch) && k < NAMEMAX) {
                msname[k++] = ch;
                gch();
            }
            msname[k] = NULL;
            if (FindSymbol(msname, macn, NAMESIZE + 2, MACNEND, MACNBR, 0)) {
                k = getint(cptr + NAMESIZE, 2);
                while (c = macq[k++]) {
                  keepch(c);
                }
                while (an(ch)) {
                  gch();
                }
            }
            else {
                k = 0;
                while (c = msname[k++]) {
                  keepch(c);
                }
                while (an(ch)) {
                  keepch(ch);
                  gch();
                }
            }
        }
        else keepch(gch());
    }
    if (pptr >= LINEMAX) error("line too long");
    keepch(NULL);
    line = pline;
    bump(0);
}

keepch(char c) {
    if (pptr < LINEMAX) {
      pline[++pptr] = c;
    }
}

ifline() {
    while (1) {
        inline();
        if (eof) return;
        if (IsMatch("#ifdef")) {
            ++iflevel;
            if (skiplevel) 
                continue;
            symname(msname);
            if (FindSymbol(msname, macn, NAMESIZE + 2, MACNEND, MACNBR, 0) == 0)
                skiplevel = iflevel;
            continue;
        }
        if (IsMatch("#ifndef")) {
            ++iflevel;
            if (skiplevel) continue;
            symname(msname);
            if (FindSymbol(msname, macn, NAMESIZE + 2, MACNEND, MACNBR, 0))
                skiplevel = iflevel;
            continue;
        }
        if (IsMatch("#else")) {
            if (iflevel) {
                if (skiplevel == iflevel) skiplevel = 0;
                else if (skiplevel == 0)  skiplevel = iflevel;
            }
            else noiferr();
            continue;
        }
        if (IsMatch("#endif")) {
            if (iflevel) {
                if (skiplevel == iflevel) skiplevel = 0;
                --iflevel;
            }
            else noiferr();
            continue;
        }
        if (skiplevel) continue;
        if (ch == 0) continue;
        break;
    }
}

inline() {           /* numerous revisions */
    int k, unit;
    poll(1);           /* allow operator interruption */
    if (input == EOF)
        openfile();
    if (eof)
        return;
    if ((unit = input2) == EOF)
        unit = input;
    if (fgets(line, LINEMAX, unit) == NULL) {
        fclose(unit);
        if (input2 != EOF)
            input2 = EOF;
        else
            input = EOF;
        *line = NULL;
    }
    else if (listfp) {
        if (listfp == output)
            fputc(';', output);
        fputs(line, listfp);
    }
    bump(0);
}

inbyte() {
    while (ch == 0) {
        if (eof) return 0;
        preprocess();
    }
    return gch();
}

/********************* scanning functions ********************/

/*
** test if next input string is legal symbol name
*/
symname(char *sname) {
    int k;
    blanks();
    if (alpha(ch) == 0) {
        return (*sname = 0);
    }
    k = 0;
    while (an(ch)) {
        sname[k] = gch();
        if (k < NAMEMAX) {
            ++k;
        }
    }
    sname[k] = 0;
    return 1;
}

Require(char *str)  {
    if (IsMatch(str) == 0) {
        error("missing token");
    }
}

ReqSemicolon() {
    if (IsMatch(";") == 0) {
        error("no semicolon");
    }
    else {
        errflag = 0;
    }
}

IsMatch(char *lit) {
    int k;
    blanks();
    if (k = streq(lptr, lit)) {
        bump(k);
        return 1;
    }
    return 0;
}

streq(char str1[], char str2[]) {
    int k;
    k = 0;
    while (str2[k]) {
        if (str1[k] != str2[k]) {
            return 0;
        }
        ++k;
    }
    return k;
}

amatch(char *lit, int len) {
    int k;
    blanks();
    if (k = astreq(lptr, lit, len)) {
        bump(k);
        return 1;
    }
    return 0;
}

astreq(char str1[], char str2[], int len) {
    int k;
    k = 0;
    while (k < len) {
        if (str1[k] != str2[k]) break;
        /*
        ** must detect end of symbol table names terminated by
        ** symbol length in binary
        */
        if (str2[k] < ' ') 
            break;
        if (str1[k] < ' ') 
            break;
        ++k;
    }
    if (an(str1[k]) || an(str2[k])) return 0;
    return k;
}

nextop(char *list) {
    char op[4];
    opindex = 0;
    blanks();
    while (1) {
        opsize = 0;
        while (*list > ' ') op[opsize++] = *list++;
        op[opsize] = 0;
        if (opsize = streq(lptr, op))
            if (*(lptr + opsize) != '=' &&
                *(lptr + opsize) != *(lptr + opsize - 1))
                return 1;
        if (*list) {
            ++list;
            ++opindex;
        }
        else return 0;
    }
}

blanks() {
    while (1) {
        while (ch) {
            if (white()) gch();
            else return;
        }
        if (line == mline) return;
        preprocess();
        if (eof) break;
    }
}

white() {
    avail(YES);  /* abort on stack/symbol table overflow */
    return (*lptr <= ' ' && *lptr);
}

gch() {
    int c;
    if (c = ch) bump(1);
    return c;
}

bump(n) int n; {
    if (n) lptr += n;
    else  lptr = line;
    if (ch = nch = *lptr) nch = *(lptr + 1);
}

kill() {
    *line = 0;
    bump(0);
}

skip() {
    if (an(inbyte()))
        while (an(ch)) gch();
    else while (an(ch) == 0) {
        if (ch == 0) break;
        gch();
    }
    blanks();
}

endst() {
    blanks();
    return (streq(lptr, ";") || ch == 0);
}

/*********** symbol table management functions ***********/

AddSymbol(char *sname, char id, char type, int size, int offset, int *lgpp, int class) {
    if (lgpp == &glbptr) {
        if (cptr2 = findglb(sname)) {
            return cptr2;
        }
        if (cptr == 0) {
            error("global symbol table overflow");
            return 0;
        }
    }
    else {
        if (locptr > (ENDLOC - SYMMAX)) {
            error("local symbol table overflow");
            abort(ERRCODE);
        }
        cptr = *lgpp;
    }
    cptr[IDENT] = id;
    cptr[TYPE] = type;
    cptr[CLASS] = class;
    putint(size, cptr + SIZE, 2);
    putint(offset, cptr + OFFSET, 2);
    cptr3 = cptr2 = cptr + NAME;
    while (an(*sname)) {
      *cptr2++ = *sname++;
    }
    if (lgpp == &locptr) {
        *cptr2 = cptr2 - cptr3;         /* set length */
        *lgpp = ++cptr2;
    }
    return cptr;
}

/*
** search for symbol match.
** on return, cptr points to slot found or empty slot
** sname: string we are trying to match
** buf: table of strings to match against
** len: max length to check for each symbol
** end: end of table of strings
** max: max count of strings in table
** off: ???
*/
FindSymbol(char *sname, char *buf, int len, char *end, int max, int off) {
    unsigned int ihash, imax;
    imax = (max - 1);
    ihash = hash(sname) % imax;
    cptr = cptr2 = buf + ihash * len;
    while (*cptr != NULL) {
        if (astreq(sname, cptr + off, NAMEMAX)) 
            return 1;
        if ((cptr = cptr + len) >= end) 
            cptr = buf;
        if (cptr == cptr2) 
            return (cptr = 0);
    }
    return 0;
}

hash(char *sname) {
    unsigned int i, c;
    i = 0;
    while (c = *sname++)
        i = (i << 1) + c;
    return i;
}

findglb(char *sname) {
    if (FindSymbol(sname, STARTGLB, SYMMAX, ENDGLB, NUMGLBS, NAME))
        return cptr;
    return 0;
}

findloc(char *sname)  {
    cptr = locptr - 1;  /* FindSymbol backward for block locals */
    while (cptr > STARTLOC) {
        cptr = cptr - *cptr;
        if (astreq(sname, cptr, NAMEMAX)) return (cptr - NAME);
        cptr = cptr - NAME - 1;
    }
    return 0;
}

nextsym(char *entry) {
    entry = entry + NAME;
    while (*entry++ >= ' ');    /* find length byte */
    return entry;
}

/******** while queue management functions *********/

addwhile(int ptr[]) {
    int k;
    ptr[WQSP] = csp;         /* and stk ptr */
    ptr[WQLOOP] = getlabel();  /* and looping label */
    ptr[WQEXIT] = getlabel();  /* and exit label */
    if (wqptr == WQMAX) {
        error("control statement nesting limit");
        abort(ERRCODE);
    }
    k = 0;
    while (k < WQSIZ) *wqptr++ = ptr[k++];
}

readwhile(int *ptr) {
    if (ptr <= wq) {
        error("out of context");
        return 0;
    }
    else return (ptr - WQSIZ);
}

delwhile() {
    if (wqptr > wq) wqptr -= WQSIZ;
}

/****************** utility functions ********************/

/*
** test if c is alphabetic
*/
alpha(char c) {
    return (isalpha(c) || c == '_');
}

/*
** test if given character is alphanumeric
*/
an(char c) {
    return (alpha(c) || isdigit(c));
}

/*
** return next avail internal label number
*/
getlabel() {
    return(++nxtlab);
}

/*
** get integer of length len from address addr
** (byte sequence set by "putint")
*/
getint(char *addr, int len) {
    int i;
    i = *(addr + --len);  /* high order byte sign extended */
    while (len--) i = (i << 8) | *(addr + len) & 255;
    return i;
}

/*
** put integer i of length len into address addr
** (low byte first)
*/
putint(int i, char *addr, int len) {
    while (len--) {
        *addr++ = i;
        i = i >> 8;
    }
}

lout(char *line, int fd) {
    fputs(line, fd);
    fputc(NEWLINE, fd);
}

/******************* error functions *********************/

illname() {
    error("illegal symbol");
    skip();
}

multidef(char *sname) {
    error("already defined");
}

needlval() {
    error("must be lvalue");
}

noiferr() {
    error("no matching #if...");
    errflag = 0;
}

error(char msg[]) {
    if (errflag) {
        return;
    }
    else {
        errflag = 1;
    }
    lout(line, stderr);
    errout(msg, stderr);
    if (alarm) fputc(7, stderr);
    if (pause) while (fgetc(stderr) != NEWLINE);
    if (listfp > 0) errout(msg, listfp);
}

errout(char msg[], int fp) {
    int k;
    k = line + 2;
    while (k++ <= lptr) {
        fputc(' ', fp);
    }
    lout("/\\", fp);
    fputs("Error: ", fp); 
    lout(msg, fp);
}

