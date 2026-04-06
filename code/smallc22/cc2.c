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

#include "stdio.h"
#include "cc.h"

// reserved keywords -- may not be used as identifiers
char *keywords[] = {
    "auto",     "break",    "case",     "char",     "const",    "continue",
    "default",  "do",       "double",   "else",     "enum",     "extern",
    "float",    "for",      "goto",     "if",       "int",      "long",
    "register", "return",   "short",    "signed",   "sizeof",   "static",
    "struct",   "switch",   "typedef",  "union",    "unsigned", "void",
    "volatile", "while",    0
};

extern char
    *symtab, *macn, *macq, *pline, *mline, optimize,
    alarm, nowarn, *glbptr, *line, *lptr, *cptr, *cptr2, *cptr3,
    *locptr, *dimdata, *dimdatptr, msname[NAMESIZE], pause,
    infn[30], inclfn[30], *paramTypes;

extern int
    *wq, ccode, ch, csp, eof, errflag, iflevel,
    input, input2, listfp, macptr, nch,
    nxtlab, op[16], opindex, opsize, output, pptr,
    skiplevel, *wqptr, lineno, inclno, warncount;

// forward declarations for this file:
int astreq(char str1[], char str2[], int len);
void errout(char msg[], int fp, char *path, int ln);
int hash(char *sname);
void ifline();
void keepch(char c);
void noiferr();

// === input functions ========================================================

void preprocess() {
    int k;
    char c;
    if (ccode) {
        line = mline;
        ifline();
        if (eof) return;
    }
    else {
        doInline();
        return;
    }
    pptr = -1;
    while (ch != LF && ch != CR && ch) {
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
        // ignore C99-style comments
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
            if (FindSymbol(msname, macn, NAMESIZE + 2, MACNEND, MACNBR, 0, NAMEMAX)) {
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

void keepch(char c) {
    if (pptr < LINEMAX) {
      pline[++pptr] = c;
    }
}

void ifline() {
    while (1) {
        doInline();
        if (eof) return;
        if (IsMatch("#ifdef")) {
            ++iflevel;
            if (skiplevel) 
                continue;
            symname(msname);
            if (FindSymbol(msname, macn, NAMESIZE + 2, MACNEND, MACNBR, 0, NAMEMAX) == 0)
                skiplevel = iflevel;
            continue;
        }
        if (IsMatch("#ifndef")) {
            ++iflevel;
            if (skiplevel) continue;
            symname(msname);
            if (FindSymbol(msname, macn, NAMESIZE + 2, MACNEND, MACNBR, 0, NAMEMAX))
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

void doInline() {
    int k, unit;
    poll(1);           // allow operator interruption
    if (input == EOF)
        openfile();
    if (eof)
        return;
    if ((unit = input2) == EOF)
        unit = input;
    line[LINEMAX - 2] = '\n';  // sentinel: fgets overwrites only if buffer fills
    if (fgets(line, LINEMAX, unit) == NULL) {
        fclose(unit);
        if (input2 != EOF)
            input2 = EOF;
        else
            input = EOF;
        *line = NULL;
    }
    else {
        if (input2 != EOF) {
            ++inclno;
        }
        else {
            ++lineno;
        }
        // If the buffer filled without a newline the physical line was longer
        // than LINEMAX-1.  Drain the remainder so the next read starts on a
        // fresh line, then report the error exactly once.
        if (line[LINEMAX - 2] != '\n' && line[LINEMAX - 2] != '\0') {
            int c;
            while ((c = fgetc(unit)) != '\n' && c != EOF) ;
            error("line too long");
        }
        if (listfp) {
            if (listfp == output)
                fputc(';', output);
            fputs(line, listfp);
        }
    }
    bump(0);
}

int inbyte() {
    while (ch == 0) {
        if (eof) return 0;
        preprocess();
    }
    return gch();
}

// === scanning functions =====================================================

int isreserved(char *name) {
    int k;
    k = 0;
    while (keywords[k]) {
        if (astreq(name, keywords[k], strlen(keywords[k])))
            return 1;
        ++k;
    }
    return 0;
}

// test if next input string is legal symbol name
int symname(char *sname) {
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

void Require(char *str)  {
    if (IsMatch(str) == 0) {
        error("missing token");
    }
}

void ReqSemicolon() {
    if (IsMatch(";") == 0) {
        error("no semicolon");
    }
    else {
        errflag = 0;
    }
}

int IsMatch(char *lit) {
    int k;
    blanks();
    if (k = streq(lptr, lit)) {
        bump(k);
        return 1;
    }
    return 0;
}

int streq(char str1[], char str2[]) {
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

int amatch(char *lit, int len) {
    int k;
    blanks();
    if (k = astreq(lptr, lit, len)) {
        bump(k);
        return 1;
    }
    return 0;
}

int astreq(char str1[], char str2[], int len) {
    int k;
    k = 0;
    while (k < len) {
        if (str1[k] != str2[k]) break;
        // must detect end of symbol table names terminated by
        // symbol length in binary
        if (str2[k] < ' ') 
            break;
        if (str1[k] < ' ') 
            break;
        ++k;
    }
    if (an(str1[k]) || an(str2[k])) return 0;
    return k;
}

int nextop(char *list) {
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

void blanks() {
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

int white() {
    avail(YES);  // abort on stack/symbol table overflow
    return (*lptr <= ' ' && *lptr);
}

int gch() {
    int c;
    if (c = ch) bump(1);
    return c;
}

void bump(int n) {
    if (n) lptr += n;
    else  lptr = line;
    if (ch = nch = *lptr) nch = *(lptr + 1);
}

void kill() {
    *line = 0;
    bump(0);
}

void skipToNextToken() {
    if (an(inbyte()))
        while (an(ch)) gch();
    else while (an(ch) == 0) {
        if (ch == 0) break;
        gch();
    }
    blanks();
}

int endst() {
    blanks();
    return (streq(lptr, ";") || ch == 0);
}

// === multi-dimensional array metadata =======================================

// Store dimension metadata for a multi-dim array.
// Entry format: [structPtr(2)] [stride_0(2)] ... [stride_{ndim-2}(2)]
// Returns pointer to start of entry in dimdata[].
int storeDimDat(int sptr, int ndim, int strides[]) {
    char *entry;
    int i, sz;
    sz = ndim * 2;
    if (dimdatptr + sz > dimdata + DIMDATSZ) {
        error("dimdata overflow");
        return 0;
    }
    entry = dimdatptr;
    putint(sptr, dimdatptr, 2);
    dimdatptr += 2;
    i = 0;
    while (i < ndim - 1) {
        putint(strides[i], dimdatptr, 2);
        dimdatptr += 2;
        ++i;
    }
    return entry;
}

// Get struct def pointer from symbol, handling dimdata indirection.
// If ndim > 1, first 2 bytes of dimdata entry hold struct def ptr.
// Otherwise, CLASSPTR is the struct def ptr directly.
int getClsPtr(char *sym) {
    if (sym[NDIM] > 1)
        return getint(getint(sym + CLASSPTR, 2), 2);
    return getint(sym + CLASSPTR, 2);
}

// Get stride for dimension idx from a multi-dim symbol's dimdata.
// idx 0 = outermost stride (row size in bytes for 2D).
int getDimStride(char *sym, int idx) {
    char *entry;
    entry = getint(sym + CLASSPTR, 2);
    return getint(entry + 2 + idx * 2, 2);
}

// === symbol table management functions ======================================

int AddSymbol(char *sname, char id, char type, int size, int offset, int *lgpp, int class) {
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
    cptr[NDIM] = 0;
    putint(0, cptr + CLASSPTR, 2);
    putint(size, cptr + SIZE, 2);
    putint(offset, cptr + OFFSET, 2);
    cptr3 = cptr2 = cptr + NAME;
    while (an(*sname)) {
      *cptr2++ = *sname++;
    }
    if (lgpp == &locptr) {
        *cptr2 = cptr2 - cptr3;         // set length
        *lgpp = ++cptr2;
    }
    return cptr;
}

// search for symbol match.
// on return, cptr points to slot found or empty slot
// sname: string we are trying to match
// buf: table of strings to match against
// len: max length to check for each symbol
// end: end of table of strings
// max: max count of strings in table
// off: ???
int FindSymbol(char *sname, char *buf, int len, char *end, int max, int off, int namelen) {
    unsigned int ihash, imax;
    imax = (max - 1);
    ihash = hash(sname) % imax;
    cptr = cptr2 = buf + ihash * len;
    while (*cptr != NULL) {
        if (astreq(sname, cptr + off, namelen)) 
            return 1;
        if ((cptr = cptr + len) >= end) 
            cptr = buf;
        if (cptr == cptr2) 
            return (cptr = 0);
    }
    return 0;
}

int hash(char *sname) {
    unsigned int i, c;
    i = 0;
    while (c = *sname++)
        i = (i << 1) + c;
    return i;
}

int findglb(char *sname) {
    if (FindSymbol(sname, STARTGLB, SYMMAX, ENDGLB, NUMGLBS, NAME, SYMMAX - NAME))
        return cptr;
    return 0;
}

int findloc(char *sname)  {
    cptr = locptr - 1;  // FindSymbol backward for block locals
    while (cptr > STARTLOC) {
        cptr = cptr - *cptr;
        if (astreq(sname, cptr, NAMEMAX)) return (cptr - NAME);
        cptr = cptr - NAME - 1;
    }
    return 0;
}

int nextsym(char *entry) {
    entry = entry + NAME;
    while (*entry++ >= ' ');    // find length byte
    return entry;
}

// === while queue management functions =======================================

void addwhile(int ptr[]) {
    int k;
    ptr[WQSP] = csp;         // and stk ptr
    ptr[WQLOOP] = getlabel();  // and looping label
    ptr[WQEXIT] = getlabel();  // and exit label
    if (wqptr == WQMAX) {
        error("control statement nesting limit");
        abort(ERRCODE);
    }
    k = 0;
    while (k < WQSIZ) *wqptr++ = ptr[k++];
}

int readwhile(int *ptr) {
    if (ptr <= wq) {
        error("out of context");
        return 0;
    }
    else return (ptr - WQSIZ);
}

void delwhile() {
    if (wqptr > wq) wqptr -= WQSIZ;
}

// === utility functions ======================================================

// test if c is alphabetic
int alpha(char c) {
    return (isalpha(c) || c == '_');
}

// test if given character is alphanumeric
int an(char c) {
    return (alpha(c) || isdigit(c));
}

// return next avail internal label number
int getlabel() {
    return(++nxtlab);
}

// get integer of length len from address addr
// (byte sequence set by "putint")
int getint(char *addr, int len) {
    int i;
    i = *(addr + --len);  // high order byte sign extended
    while (len--) i = (i << 8) | *(addr + len) & 255;
    return i;
}

// put integer i of length len into address addr
// (low byte first)
void putint(int i, char *addr, int len) {
    while (len--) {
        *addr++ = i;
        i = i >> 8;
    }
}

void lout(char *line, int fd) {
    fputs(line, fd);
    fputc(NEWLINE, fd);
}

// === error functions ========================================================

void illname() {
    error("illegal symbol");
    skipToNextToken();
}

void multidef(char *sname) {
    error("already defined");
}

void needlval() {
    error("must be lvalue");
}

void noiferr() {
    error("no matching #if...");
    errflag = 0;
}

void error(char msg[]) {
    char *path;
    int   ln;
    if (errflag) {
        return;
    }
    else {
        errflag = 1;
    }
    if (input2 != EOF) { path = inclfn; ln = inclno; }
    else               { path = infn;   ln = lineno; }
    lout(line, stderr);
    errout(msg, stderr, path, ln);
    if (alarm) fputc(7, stderr);
    if (pause) while (fgetc(stderr) != NEWLINE);
    if (listfp > 0) errout(msg, listfp, path, ln);
}

void errout(char msg[], int fp, char *path, int ln) {
    int k;
    k = line + 2;
    while (k++ <= lptr) {
        fputc(' ', fp);
    }
    lout("/\\", fp);
    fputs("Error at ", fp);
    fprintf(fp, "%s(%d): ", path, ln);
    lout(msg, fp);
}

void warning(char msg[]) {
    if (nowarn) return;
    warncount++;
    if (input2 != EOF)
        fprintf(stderr, "%s:%d ", inclfn, inclno);
    else
        fprintf(stderr, "%s:%d ", infn, lineno);
    fputs("Warning: ", stderr);
    fputs(msg, stderr);
    fputc('\n', stderr);
    if (alarm) fputc(7, stderr);
}

void warningWithName(char msg[], char *name, char suffix[]) {
    if (nowarn) return;
    warncount++;
    if (input2 != EOF)
        fprintf(stderr, "%s:%d ", inclfn, inclno);
    else
        fprintf(stderr, "%s:%d ", infn, lineno);
    fputs("Warning: ", stderr);
    fputs(msg, stderr);
    fputs(name, stderr);
    fputs(suffix, stderr);
    fputc('\n', stderr);
    if (alarm) fputc(7, stderr);
}

// paramTypes[] -- function parameter-type records.
// Each entry is laid out as:
//   [0] nparams_byte: (variadic<<7) | fixed_param_count
//   [1..n] per param: (isPointer<<7) | (TYPE_xxx & 0x7F)
// Index 0 is the reserved "no prototype" sentinel; entries start at 1.
// paramTypes is defined in cc1.c and allocated in main() via calloc(256,2).
int paramTypesPtr = 1;   // next free index; 0 is reserved as sentinel

// Store function parameter types into paramTypes[].
// nparams_byte = (variadic<<7) | fixed_param_count
// typebuf = array of (isPtr<<7)|(TYPE_xxx&0x7F), one per fixed param.
// Returns the index of the stored entry (>= 1), or 0 on overflow.
int storeParamTypes(char *typebuf, int nparams_byte) {
    int idx, n, k;
    n = nparams_byte & 0x7F;
    if (paramTypesPtr + 1 + n > FNPARAMTS_SZ) {
        error("function param table full");
        return 0;
    }
    idx = paramTypesPtr;
    paramTypes[idx] = (char)nparams_byte;
    k = 0;
    while (k < n) {
        paramTypes[idx + 1 + k] = typebuf[k];
        ++k;
    }
    paramTypesPtr += 1 + n;
    return idx;
}

