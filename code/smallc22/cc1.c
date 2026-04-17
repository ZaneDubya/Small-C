// ============================================================================
// Small-C Compiler -- Part 1 --  Top End.
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
#include "notice.h"
#include "cc.h"

// forward declarations for this file:
int addLabel(int def);
char *applyDimMeta(char *sym, int type, int typeSubPtr, char *ddentry);
void calcArgStrides(int type, int typeSubPtr, int ndim, int dims[]);
void dispatchLocInit(int type, int typeSubPtr, int id, int sz);
int doExpression(int use);
void doArgsKR();
void doArgsTyped(int type, int typeSubPtr);
void doAsmBlock();
void doBreak();
void doCase();
void doCompound();
void doContinue();
void doDefault();
int doGlbDeclare(int class);
void doGoto();
void doDo();
void doFor();
void doIf();
int doLabel();
int doReturn();
void doSwitch();
int doStatement();
void doWhile();
void emitLocStatic(int type, int id, int sz);
int isConstExpr(int *val);
int isConstExpr32(int *val, int *val_hi);
void initGlbMDArr(int type, int typeSubPtr, int ndim, int dims[], int class);
void initGlbVar(int size, int ident, int dim, int class);
void initLocArray(int type);
void initLocPtrArray();
void initLocChrStr(int infer);
void initLocMDArr(int type, char *symptr);
void initLocScalar(int type);
void initLocStruct(int type);
void initLocUnion(int typeSubPtr);
int initPtrArray(int type, int dim, int class);
void initLocStrArr(int typeSubPtr);
void initStruct(int typeSubPtr, int class);
void initUnion(int typeSubPtr, int class);
int openFileOrErr(char *fn, char *mode);
void parseTopLvl();
void recordParamTypes(char *funcSym);

// miscellaneous storage
int
    nogo,     // disable goto statements?
    noloc,    // disable block locals?
    opindex,  // index to matched operator
    opsize,   // size of operator in characters
    swactive, // inside a switch?
    swdefault,// default label #, else 0
    swislong, // switch expression is 32-bit?
    *swnext,   // address of next entry
    *swend,    // address of last entry
    *stage,    // staging buffer address
    *argbuf,   // argument collect buffer for two-pass R-to-L push
    *wq,       // while queue
    argcs,     // local copy of argc
    *argvs,    // local copy of argv
    *wqptr,    // ptr to next entry
    litptr,   // ptr to next entry
    ch,       // current character of input line
    nch,      // next character of input line
    declared, // # of local bytes to declare, -1 when declared
    nxtlab,   // next avail label #
    litlab,   // label # assigned to literal pool
    csp,      // compiler relative stk ptr
    argstk,   // function arg sp
    argtop,   // highest formal argument offset
    rettype,  // return type of current function (TYPE_INT default)
    rettypeSubPtr, // struct def ptr if rettype == TYPE_STRUCT
    rettypeIsPtr,  // 1 when the current function returns a pointer
    ncmp,     // # open compound statements
    errflag,  // true after 1st error in statement
    eof,      // true on final input eof
    output,   // fd for output file
    files,    // true if file list specified on cmd line
    filearg,  // cur file arg index
    input = EOF, // fd for input file
    usexpr = YES, // true if value of expression is used
    ccode = YES, // true while parsing C code
    *snext,    // next addr in stage
    *stail,    // last addr of data in stage
    *slast,    // last addr in stage
    listfp,   // file pointer to list device
    lastst,   // last parsed statement type
    oldseg,   // current segment (0, DATASEG, CODESEG)
    errcount,    // number of errors emitted this compilation
    lineno,   // current line number in input file
    lastNdim,               // array dimensions from last parseLocDecl (0 = scalar/1-D)
    lastStrides[MAX_DIMS],  // per-dim strides from last parseLocDecl (valid when lastNdim > 1)
    typeIsConst,            // set by dotype(): 1 if const qualifier was seen; cleared by dotype()
    typeIsPtr,              // set by dotype(): 1 if a typedef alias resolved to a pointer type;
                            //   consumed (read + cleared) by parseLocDecl and parseSizeofType
    protoVoidList,          // set by doArgsTyped when f(void) form is parsed
    protoVariadic,          // set by doArgsTyped/doArgsKR when ... is parsed
    argbufcur,              // global argbuf write cursor; saved/restored per callfunc level
    lastPtrDepth;           // pointer depth from last parseLocDecl/parseGlbDecl (0=non-ptr, 1=T*, 2=T**, ...)

char
    optimize, // optimize output of staging buffer?
    alarm,    // audible alarm on errors?
    monitor,  // monitor function headers?
    pause,    // pause for operator on errors?
    *symtab,   // symbol table
    *litq,     // literal pool - staging buffer for constants, per function
    *line,     // current line pointer
    *lptr,     // ptr to current character in "line"
    *glbptr,   // global symbol table
    *locptr,   // next local symbol table entry
    *cptr,     // work ptrs to any char buffer
    *cptr2,
    *cptr3,
    *dimdata,  // multi-dim array metadata buffer
    *dimdatptr, // next free entry in dimdata
    *paramTypes, // function parameter-type records (allocated in main)
    ssname[NAMESIZE],   // symbol name (static locals mangled to _L<N> form)
    curfn[NAMESIZE],    // name of function currently being compiled
    infn[FNSIZE];       // current input filename

int op[16] = {   // p-codes of signed binary operators
    OR12,                        // level5
    XOR12,                       // level6
    AND12,                       // level7
    EQ12,   NE12,                // level8
    LE12,   GE12,  LT12,  GT12,  // level9
    ASR12,  ASL12,               // level10
    ADD12,  SUB12,               // level11
    MUL12, DIV12, MOD12          // level12
};

int op2[16] = {  // p-codes of unsigned binary operators
    OR12,                        // level5
    XOR12,                       // level6
    AND12,                       // level7
    EQ12,   NE12,                // level8
    LE12u,  GE12u, LT12u, GT12u, // level9
    LSR12,  ASL12,               // level10
    ADD12,  SUB12,               // level11
    MUL12u, DIV12u, MOD12u       // level12
};

int opd[16] = {  // p-codes of signed 32-bit binary operators
    ORd12,                          // level5
    XORd12,                         // level6
    ANDd12,                         // level7
    EQd12,   NEd12,                 // level8
    LEd12,   GEd12,  LTd12,  GTd12, // level9
    ASRd12,  ASLd12,                // level10
    ADDd12,  SUBd12,                // level11
    MULd12,  DIVd12, MODd12         // level12
};

int opd2[16] = { // p-codes of unsigned 32-bit binary operators
    ORd12,                          // level5
    XORd12,                         // level6
    ANDd12,                         // level7
    EQd12,   NEd12,                 // level8
    LEd12u,  GEd12u, LTd12u, GTd12u, // level9
    ASRd12u, ASLd12,                // level10
    ADDd12,  SUBd12,                // level11
    MULd12u, DIVd12u, MODd12u       // level12
};

extern char hasInlineAsm;    // set by doAsmBlock() where there is inline assembly; disables optimizing frame teardown
extern char hasStaticLocal;  // set by emitLocStatic() when a local static is declared; disables FPO mid-function flush
extern int macptr;           // next write index in macq (preprocessor macro string buffer usage)
extern int paramTypesPtr;
extern char *structdata, *structdatnext;

#ifdef ENABLE_WARNINGS
    int warncount; // number of warnings emitted this compilation
    char nowarn;   // suppress warnings?
#endif

#ifdef ENABLE_DIAGNOSTICS
int litptrMax,      // high-water mark of litptr
    glbcount,       // number of global symbol table slots used
    locptrMax,      // high-water mark of local symbol table bytes used
    macnMax;        // high-water mark of macro name slots used (new unique defines)
char *structBase;   // snapshot of structdata base, for usage stats
#endif


// === Bootstrap and I/O ======================================================

// execution begins here
void main(int argc, int *argv) {
    unsigned int availMem;
    fputs(VERSION, stderr);
    fputs(CRIGHT1, stderr);
    argcs = argc;
    argvs = argv;
    swnext = calloc(SWTABSZ, 1);
    swend = swnext + (SWTABSZ - SWSIZ) / BPW;
    stage = calloc(STAGESIZE, 2 * BPW);
    argbuf = calloc(ARGBUFSZ, sizeof(int));
    argbufcur = 0;
    wqptr = wq = calloc(WQTABSZ, BPW);
    litq = calloc(LITABSZ, 1);
    initPreprocessor();
    slast = stage + (STAGESIZE * 2);
    symtab = calloc(NUMLOCS*SYMAVG + NUMGLBS*SYMMAX, 1);
    locptr = STARTLOC;
    glbptr = STARTGLB;
    dimdatptr = dimdata = calloc(DIMDATSZ, 1);
    paramTypes = calloc(FNPARAMSZ, 1);
    initStructs();
    initEnums();
    availMem = avail(0);
#ifdef ENABLE_DIAGNOSTICS
    structBase = structdata;
    fprintf(stderr, "  Memory available: %d b\n", availMem);
#endif
    if (availMem < 4000) {
        fputs("out of memory\n", stderr);
        exit(1);
    }
    rettype = TYPE_INT;
    rettypeSubPtr = 0;
#ifdef ENABLE_WARNINGS
    warncount = 0;
#endif
    getRunArgs();   // get run options from command line arguments
    openfile();     // and initial input file
    preprocess();   // fetch first line
    header();       // intro code
    parseTopLvl();        // process ALL input
    trailer();      // follow-up code
    fclose(output); // explicitly close output
#ifdef ENABLE_WARNINGS
    if (warncount > 0) {
        fprintf(stderr, "  %d warning(s). Press any key to continue...\n", warncount);
        fgetc(stderr);
    }
#endif
}

// get run options
void getRunArgs() {
    char argbuf[LINESIZE];
    int i;
    i = listfp = nxtlab = 0;
    output = stdout;
    optimize = YES;
    alarm = monitor = pause = NO;
#ifdef ENABLE_WARNINGS
    nowarn = NO;
#endif
    while (getarg(++i, argbuf, LINESIZE, argcs, argvs) != EOF) {
        if (argbuf[0] != '-' && argbuf[0] != '/')
            continue;
        if (toupper(argbuf[1]) == 'L' && isdigit(argbuf[2]) && argbuf[3] <= ' ') {
            listfp = argbuf[2] - '0';
            continue;
        }
        if (toupper(argbuf[1]) == 'N' && toupper(argbuf[2]) == 'O' && 
            argbuf[3] <= ' ') {
            optimize = NO;
            continue;
        }
        if (argbuf[2] <= ' ') {
            if (toupper(argbuf[1]) == 'A') {
                alarm = YES;
                continue;
            }
            if (toupper(argbuf[1]) == 'M') {
                monitor = YES; 
                continue;
            }
            if (toupper(argbuf[1]) == 'P') {
                pause = YES;
                continue;
            }
            if (toupper(argbuf[1]) == 'W') {
#ifdef ENABLE_WARNINGS
                nowarn = YES;
#endif
                continue;
            }
        }
        fputs("usage: cc [file]... [-m] [-a] [-p] [-w] [-l#] [-no]\n", stderr);
        abort(ERRCODE);
    }
}

// input and output file opens
void openfile() {
    char outfn[15];
    char fname[LINESIZE];
    int i, j, ext;
    input = EOF;
    while (getarg(++filearg, fname, LINESIZE, argcs, argvs) != EOF) {
        if (fname[0] == '-' || fname[0] == '/') continue;
        ext = NO;
        i = -1;
        j = 0;
        while (fname[++i]) {
            if (fname[i] == '.') {
                ext = YES;
                break;
            }
            if (j < 10) outfn[j++] = fname[i];
        }
        if (!ext) strcpy(fname + i, ".C");
        input = openFileOrErr(fname, "r");
        strcpy(infn, fname);
        lineno = 0;
        if (!files && iscons(stdout)) {
            strcpy(outfn + j, ".ASM");
            output = openFileOrErr(outfn, "w");
        }
        files = YES;
        kill();
        return;
    }
    if (files++)
        eof = YES;
    else
        input = stdin;
    kill();
}

// open a file with error checking
int openFileOrErr(char *fn, char *mode) {
    int fd;
    if (fd = fopen(fn, mode))
        return fd;
    fputs("open error on ", stderr);
    lout(fn, stderr);
    abort(ERRCODE);
}

// === Top-Level Parse Loop ===================================================

// Process all input text. At this level, only global declarations, defines,
// includes, structs, and function definitions are legal.
void parseTopLvl() {
    while (eof == 0) {
        if (amatch("extern", 6))
            doGlbDeclare(EXTERNAL);
        else if (amatch("static", 6)) {
            if (doGlbDeclare(STATIC)) {
                // parsed and emitted a static variable
            }
            else {
                dofunction(STATIC);
            }
        }
        else if (amatch("struct", 6)) {
            doStructBlock(KIND_STRUCT);
        }
        else if (amatch("union", 5)) {
            doStructBlock(KIND_UNION);
        }
        else if (doGlbDeclare(GLOBAL)) {
            ;
        }
        else if (isMatch("#asm")) {
            doAsmBlock();
        }
        else {
            dofunction(GLOBAL);
        }
        blanks();                 // force eof if pending
    }
}

// === Type-Specifier Parsing =================================================

// get type of declaration.
// Handles type qualifiers: const, volatile, register, auto, signed,
// unsigned, short. On 8086 short == int (both 16-bit).
// Sets global typeIsConst = 1 when the const qualifier is present.
// "volatile" and "register" are accepted and silently ignored.
// "register" is treated as automatic storage (the default on 8086 where
// there are no spare GPRs available for user variables).
// return type value of declaration, otherwise 0 if not a valid declaration.
int dotype(int *typeSubPtr) {
    int isConst, isUnsigned, isShort, gotSignSize;
    isConst = isUnsigned = isShort = gotSignSize = 0;
    typeIsConst = 0;
    typeIsPtr = 0;
    // Consume qualifier/modifier keywords in any order.  Only unsigned and
    // short actually affect the returned type; the rest are accepted as
    // no-ops (const sets a flag for callers, the others are discarded).
    for (;;) {
        if (amatch("const", 5))        { isConst    = 1; }
        else if (amatch("volatile", 8)){ /* no-op on 8086  */       }
        else if (amatch("register", 8)){ /* treat as auto on 8086 */ }
        else if (amatch("auto", 4))    { /* default storage class  */ }
        else if (amatch("signed", 6))  { gotSignSize = 1; }
        else if (amatch("unsigned", 8)){ isUnsigned  = 1; gotSignSize = 1; }
        else if (amatch("short", 5))   { isShort     = 1; gotSignSize = 1; }
        else break;
    }
    typeIsConst = isConst;
    // short [int] -- on 8086 short == int (16-bit word)
    if (isShort) {
        amatch("int", 3);
        return isUnsigned ? TYPE_UINT : TYPE_INT;
    }
    if (amatch("void", 4)) {
        return TYPE_VOID;
    }
    if (amatch("char", 4)) {
        return isUnsigned ? TYPE_UCHR : TYPE_CHR;
    }
    if (amatch("long", 4)) {
        if (amatch("long", 4)) {
            error("long long not supported");
        }
        amatch("int", 3);
        return isUnsigned ? TYPE_ULONG : TYPE_LONG;
    }
    // "int" explicit, or implied by unsigned/signed alone
    if (amatch("int", 3) || gotSignSize) {
        return isUnsigned ? TYPE_UINT : TYPE_INT;
    }
    if (amatch("struct", 6)) {
        if ((*typeSubPtr = getStructPtr(KIND_STRUCT)) == -1)
            error("unknown struct name");
        return TYPE_STRUCT;
    }
    if (amatch("union", 5)) {
        if ((*typeSubPtr = getStructPtr(KIND_UNION)) == -1)
            error("unknown union name");
        return TYPE_STRUCT;
    }
    if (amatch("enum", 4)) {
        return doEnum(typeSubPtr);
    }
    {
        char *savedLptr, *p;
        int savedCh, savedIsConst;
        savedLptr = lptr;
        savedCh = ch;
        savedIsConst = typeIsConst;
        if (symname(ssname) && (p = findglb(ssname)) && ((p[CLASS] & 0x7F) == TYPEDEF)) {
            *typeSubPtr = getint(p + CLASSPTR, 2);
            typeIsPtr = (p[IDENT] == IDENT_POINTER);
            return p[TYPE];
        }
        lptr = savedLptr;
        ch = savedCh;
        typeIsConst = savedIsConst;
    }
    // If only const/volatile were seen with no base type, clear the flag
    // so callers do not misinterpret a non-declaration as const.
    typeIsConst = 0;
    return 0;
}

// parse a typedef declaration: typedef <type> [*] <name> ;
// Called from doGlbDeclare() after "typedef" has been consumed.
// Stores a TYPEDEF entry in the global symbol table.
// IDENT == IDENT_POINTER if the alias is a pointer type, else IDENT_VARIABLE.
// TYPE  == aliased base type (TYPE_INT, TYPE_CHR, TYPE_STRUCT, etc.)
// CLASSPTR == struct/union def pointer when TYPE == TYPE_STRUCT, else 0.
int doTypedef() {
    int tdType, tdSubPtr, tdIsPtr;
    char *sym;
    tdType = dotype(&tdSubPtr);
    tdIsPtr = typeIsPtr;
    if (isMatch("*")) tdIsPtr = 1;
    if (tdType == 0) {
        error("bad typedef type");
        return 1;
    }
    if (symname(ssname) == 0) {
        illname();
        return 1;
    }
    if (isreserved(ssname)) {
        error("reserved keyword used as typedef name");
        return 1;
    }
    if (findglb(ssname)) {
        multidef(ssname);
        return 1;
    }
    sym = addSymbol(ssname,
        tdIsPtr ? IDENT_POINTER : IDENT_VARIABLE,
        tdType, 0, 0, &glbptr, TYPEDEF);
    if (sym && tdType == TYPE_STRUCT)
        putint(tdSubPtr, sym + CLASSPTR, 2);
    reqSemicolon();
    return 1;
}

// parseSizeofType() -- parse the type/object operand of sizeof().
// Returns the byte size of the named type or object.
// Does NOT consume parentheses; the caller in level13() handles them.
// Reuses dotype() for all keyword-type parsing so that the two paths
// stay consistent (same qualifier handling, same typedef resolution).
int parseSizeofType() {
    int typeSubPtr, type, sz;
    char sname[NAMESIZE];
    char *ptr;
    typeSubPtr = 0;
    type = dotype(&typeSubPtr);
    // A pointer typedef (e.g. typedef int *P; sizeof(P)) is always BPW.
    if (typeIsPtr) return BPW;
    if (type == TYPE_VOID) {
        // sizeof(void) == 0; sizeof(void *) == BPW
        return isMatch("*") ? BPW : 0;
    }
    if (type == TYPE_STRUCT) {
        // TYPE_STRUCT is shared with unions.  typeSubPtr is the tag pointer
        // returned by getStructPtr(); -1 means the tag was unknown (error already
        // emitted by dotype).
        sz = (typeSubPtr != -1) ? getStructSize(typeSubPtr) : 0;
    }
    else if (type != 0) {
        // All scalar types: size is encoded in the high bits of the type
        // constant (type >> 2 == byte size).
        sz = type >> 2;
    }
    else {
        // dotype() found no keyword type and backtracks its token on failure,
        // so the name is still in the input.  Try sizeof(variable_name).
        // Also handles local typedef names that dotype() misses (it only
        // searches findglb for typedef).
        sz = 0;
        if (symname(sname) &&
            ((ptr = findloc(sname)) || (ptr = findglb(sname))) &&
            ptr[IDENT] != IDENT_FUNCTION &&
            ptr[IDENT] != IDENT_PTR_FUNCTION &&
            ptr[IDENT] != IDENT_LABEL) {
            if ((ptr[CLASS] & 0x7F) == TYPEDEF) {
                if (ptr[IDENT] == IDENT_POINTER)
                    sz = BPW;
                else if (ptr[TYPE] == TYPE_STRUCT)
                    sz = getStructSize(getint(ptr + CLASSPTR, 2));
                else
                    sz = ptr[TYPE] >> 2;
            }
            else {
                sz = getint(ptr + SIZE, 2);
            }
        }
        else {
            error("must be object or type");
        }
        return sz;
    }
    // A trailing '*' turns any base type into a pointer type (all pointers
    // are BPW on the 8086).
    if (sz && isMatch("*")) sz = BPW;
    return sz;
}

// === Global Declaration Parsing =============================================

// test for global declarations
int doGlbDeclare(int class) {
    int type, typeSubPtr, declclass;
    char *p;
    if (amatch("typedef", 7)) {
        return doTypedef();
    }
    type = dotype(&typeSubPtr);
    // Incorporate const qualifier into the storage class byte.
    // Functions cannot be const; only variable declarations use this flag.
    declclass = typeIsConst ? (class | CONST_FLAG) : class;
    if (type != 0) {
        // Lookahead: if next tokens are "name(" this is a function
        // definition, not a variable declaration. Divert to dofunction().
        blanks();
        p = lptr;
        if (alpha(*p)) {
            while (an(*p)) p++;
            while (*p == ' ' || *p == '\t') p++;
            if (*p == '(') {
                rettype = type;
                rettypeSubPtr = (type == TYPE_STRUCT) ? typeSubPtr : 0;
                rettypeIsPtr = typeIsPtr;    // pointer typedef -> pointer-returning fn
                dofunction(class);   // functions are never const
                rettypeIsPtr = 0;
                rettype = TYPE_INT;
                rettypeSubPtr = 0;
                return 1;
            }
        }
        else if (*p == '*') {
            char *q;
            q = p + 1;
            // Skip any additional '*' tokens (multi-level pointer return type)
            while (*q == ' ' || *q == '\t') q++;
            while (*q == '*') { q++; while (*q == ' ' || *q == '\t') q++; }
            if (alpha(*q)) {
                while (an(*q)) q++;
                while (*q == ' ' || *q == '\t') q++;
                if (*q == '(') {
                    rettype = type;
                    rettypeSubPtr = (type == TYPE_STRUCT) ? typeSubPtr : 0;
                    rettypeIsPtr = 1;
                    dofunction(class);
                    rettypeIsPtr = 0;
                    rettype = TYPE_INT;
                    rettypeSubPtr = 0;
                    return 1;
                }
            }
        }
        parseGlbDecl(type, declclass, typeSubPtr);
    }
    else if (class == EXTERNAL) {
        parseGlbDecl(TYPE_INT, class, typeSubPtr);
    }
    else {
        return 0;
    }
    reqSemicolon();
    return 1;
}

// Parse size expression inside one pair of array brackets, consuming the
// closing ']'.  The opening '[' has already been consumed by the caller.
// Returns the dimension size as a non-negative integer.
// An empty bracket pair '[]' returns 0 (unspecified/inferred).
// A non-constant expression defaults to 1 and continues without an error.
// A negative constant is rejected and its absolute value is used instead.
int parseArraySz() {
    int val;
    if (isMatch("]")) 
        return 0; // null size
    if (isConstExpr(&val) == 0) 
        val = 1;
    if (val < 0) {
        error("negative size illegal");
        val = -val;
    }
    require("]");               // force single dimension
    return val;                 // and return size
}

// Parse one or more array dim specifications starting after the first '['
// (already consumed by the caller).  dims[0] receives the first size; each
// subsequent '[N]' appends to dims[].  Returns total number of dimensions.
int parseArrayDims(int dims[]) {
    int ndim;
    dims[0] = parseArraySz();
    ndim = 1;
    while (isMatch("[")) {
        if (ndim >= MAX_DIMS) {
            error("too many dimensions");
            break;
        }
        dims[ndim++] = parseArraySz();
    }
    return ndim;
}

// Parse a global variable declaration
void parseGlbDecl(int type, int class, int typeSubPtr) {
    int id, dim, ptrDepth, hasFnPtrParen, depth;
    int dims[MAX_DIMS], ndim;
    while (1) {
        if (endst()) {
            return;  // do line
        }
        ptrDepth = 0;
        // Detect function-pointer declarator: type (*name)(<params>).
        // The leading '(' must be consumed before the '*' check so that
        // void (*fn)() is not confused with a plain void variable.
        hasFnPtrParen = isMatch("(") ? 1 : 0;
        if (typeIsPtr || isMatch("*")) {
            typeIsPtr = 0;
            id = IDENT_POINTER; 
            dim = 0;
            ptrDepth = 1;
            while (isMatch("*")) ptrDepth++;
        }
        else { 
            id = IDENT_VARIABLE; 
            dim = 1; 
        }
        if (id == IDENT_VARIABLE && type == TYPE_VOID) {
            error("variable cannot have void type");
            return;
        }
        ndim = 0;
        if (symname(ssname) == 0)
            illname();
        else if (isreserved(ssname))
            error("reserved keyword used as name");
        if (findglb(ssname)) 
            multidef(ssname);
        if (hasFnPtrParen) {
            // Consume the closing ')' of (*name) and the parameter list.
            // Parameter types are accepted but discarded: all function pointer
            // variables are treated as plain pointers in Small-C.
            isMatch(")");
            if (id == IDENT_POINTER && isMatch("(")) {
                if (isMatch(")") == 0) {
                    depth = 1;
                    while (depth > 0) {
                        if (isMatch("("))      depth++;
                        else if (isMatch(")")) depth--;
                        else if (ch == 0)      break;
                        else                   gch();
                    }
                }
            }
            // Fall through: id == IDENT_POINTER, continue to init/addSymbol.
        }
        else {
            if (id == IDENT_POINTER && isMatch("[")) {
                id = IDENT_PTR_ARRAY;
                dims[0] = parseArraySz();
                ndim = 1;
                dim = dims[0];
            }
            if (id == IDENT_VARIABLE) {
                if (isMatch("(")) { 
                    id = IDENT_FUNCTION; 
                    require(")"); 
                }
                else if (isMatch("[")) { 
                    id = IDENT_ARRAY;
                    ndim = parseArrayDims(dims);
                    dim = dims[0];
                }
            }
        }
        if (class == EXTERNAL && id != IDENT_FUNCTION && id != IDENT_PTR_FUNCTION)
            external(ssname, type >> 2, id);
        else if (id != IDENT_FUNCTION) {
            if (id == IDENT_PTR_ARRAY)
                dim = initPtrArray(type, dim, class);
            else if (type == TYPE_STRUCT && id == IDENT_VARIABLE) {
                if (typeSubPtr != -1 && ((char *)typeSubPtr)[STRDAT_KIND] == KIND_UNION)
                    initUnion(typeSubPtr, class);
                else
                    initStruct(typeSubPtr, class);
            }
            else if (ndim > 1)
                initGlbMDArr(type, typeSubPtr, ndim, dims, class);
            else {
                initGlbVar(type >> 2, id, dim, class);
            }
        }
        if (id == IDENT_PTR_ARRAY) {
            addSymbol(ssname, id, type, dim * BPW, 0, &glbptr, class);
        }
        else if (id == IDENT_POINTER) {
            addSymbol(ssname, id, type, BPW, 0, &glbptr, class);
            cptr[PTRDEPTH] = ptrDepth;
            applyDimMeta(cptr, type, typeSubPtr, 0);
        }
        else if (ndim > 1) {
            calcArgStrides(type, typeSubPtr, ndim, dims);
            addSymbol(ssname, id, type, dims[0] * lastStrides[0], 0, &glbptr, class);
            applyDimMeta(cptr, type, typeSubPtr, 0);
        }
        else if (type == TYPE_STRUCT) {
            int structsize = getStructSize(typeSubPtr);
            addSymbol(ssname, id, type, dim * structsize, 0, &glbptr, class);
            applyDimMeta(cptr, type, typeSubPtr, 0);
        }
        else {
            int symclass = (id == IDENT_FUNCTION || id == IDENT_PTR_FUNCTION)
                ? PROTOEXT : class;
            addSymbol(ssname, id, type, dim * (type >> 2), 0, &glbptr, symclass);
        }
        if (isMatch(",") == 0) 
            return;
    }
}

// === Global Variable Initialization =========================================

// evaluate one initializer
void initGlbElem(int size, int ident, int *dim) {
    int value, value_hi;
    if (string(&value)) {
        if (ident == IDENT_VARIABLE || size != 1) {
            error("must assign to char pointer or char array");
        }
        *dim -= (litptr - value);
        if (ident == IDENT_POINTER) {
            point();
        }
    }
    else {
        if (size == BPD) {
            // 32-bit long initializer: must capture hi word too
            if (isConstExpr32(&value, &value_hi)) {
                if (ident == IDENT_POINTER) {
                    error("cannot assign to pointer");
                }
                stowlit(value, BPW);
                stowlit(value_hi, BPW);
                *dim -= 1;
            }
        }
        else {
            if (isConstExpr(&value)) {
                if (ident == IDENT_POINTER) {
                    error("cannot assign to pointer");
                }
                stowlit(value, size);
                *dim -= 1;
            }
        }
    }
}

// Initialize global object
void initGlbVar(int size, int ident, int dim, int class) {
    int savedim;
    litptr = 0;
    if (dim == 0) {
        dim = -1;         // *... or ...[]
    }
    savedim = dim;
    decGlobal(ident, class == GLOBAL); // don't do public if class == STATIC
    if (isMatch("=")) {
        if (isMatch("{")) {
            while (dim) {
                initGlbElem(size, ident, &dim);
                if (isMatch(",") == 0) {
                    break;
                }
            }
            require("}");
        }
        else {
            initGlbElem(size, ident, &dim);
        }
    }
    if (savedim == -1 && dim == -1) {
        if (ident == IDENT_ARRAY) {
            error("need array size");
        }
        stowlit(0, size = BPW);
    }
    dumpLitPool(size);
    dumpzero(size, dim);           // only if dim > 0
}

// Initialize a global pointer array (e.g. char *arr[] = {"str", 0})
// returns actual element count
int initPtrArray(int type, int dim, int class) {
    int labels[MAXARRAYDIM];
    int values[MAXARRAYDIM];
    int isstr[MAXARRAYDIM];
    int strstart[MAXARRAYDIM];
    int strend[MAXARRAYDIM];
    int count, i, k, j, savedim, value;
    litptr = 0;
    count = 0;
    savedim = dim;
    if (dim == 0) dim = -1;

    decGlobal(IDENT_PTR_ARRAY, class == GLOBAL);

    if (isMatch("=")) {
        if (isMatch("{")) {
            while (dim && count < MAXARRAYDIM) {
                if (string(&value)) {
                    isstr[count] = 1;
                    labels[count] = getlabel();
                    strstart[count] = value;
                    strend[count] = litptr;
                    ++count;
                    --dim;
                }
                else if (isConstExpr(&value)) {
                    isstr[count] = 0;
                    values[count] = value;
                    ++count;
                    --dim;
                }
                else break;
                if (isMatch(",") == 0) break;
            }
            require("}");
        }
    }

    if (savedim == 0 && count == 0) {
        error("need array size or initializer");
    }

    // emit array elements as DW entries
    i = 0;
    while (i < count) {
        if (isstr[i]) {
            gen(NEARm, labels[i]);
        }
        else {
            gen(WORD_, 0);
            outdec(values[i]);
            newline();
        }
        ++i;
    }

    // zero-fill remaining declared elements
    if (dim > 0) {
        dumpzero(BPW, dim);
    }

    // emit string data with labels
    i = 0;
    while (i < count) {
        if (isstr[i]) {
            gen(LABm, labels[i]);
            k = strstart[i];
            while (k < strend[i]) {
                gen(BYTE_, 0);
                j = 10;
                while (j--) {
                    outdec(litq[k] & 255);
                    ++k;
                    if (j == 0 || k >= strend[i]) {
                        newline();
                        break;
                    }
                    fputc(',', output);
                }
            }
        }
        ++i;
    }

    if (savedim == 0) return count;
    return savedim;
}

// Initialize all members of a global struct from { expr, expr, ... }
void initStructBody(int typeSubPtr) {
    char *membercur, *memberend;
    int memberSize, memberTotal, dim;
    membercur = getint(typeSubPtr + STRDAT_MBEG, 2);
    memberend = getint(typeSubPtr + STRDAT_MEND, 2);
    while (membercur < memberend) {
        memberSize = membercur[STRMEM_TYPE] >> 2;
        if (memberSize < 1) memberSize = 1;
        memberTotal = getint(membercur + STRMEM_SIZE, 2);
        dim = 1;
        initGlbElem(memberSize, IDENT_VARIABLE, &dim);
        // Zero-fill remaining bytes for this member (e.g. array member)
        while (litptr < getint(membercur + STRMEM_OFFSET, 2) + memberTotal) {
            stowlit(0, 1);
        }
        membercur += STRMEM_MAX;
        if (membercur < memberend) {
            if (isMatch(",") == 0) break;
        }
    }
}

// Initialize only the first member of a global union from { expr }
void initUnionBody(int typeSubPtr) {
    char *firstMem;
    int memberSize, memberTotal, dim;
    firstMem = getint(typeSubPtr + STRDAT_MBEG, 2);
    if (firstMem < getint(typeSubPtr + STRDAT_MEND, 2)) {
        memberSize = firstMem[STRMEM_TYPE] >> 2;
        if (memberSize < 1) memberSize = 1;
        memberTotal = getint(firstMem + STRMEM_SIZE, 2);
        dim = 1;
        initGlbElem(memberSize, IDENT_VARIABLE, &dim);
        // zero-fill remainder of first member (e.g. array member)
        while (litptr < memberTotal) {
            stowlit(0, 1);
        }
        if (isMatch(",")) {
            error("union initializer only allows one value");
        }
    }
}

// shared wrapper for global struct/union initialization.
// Emits the DATA label, parses the optional "= { ... }" initializer by
// calling body(typeSubPtr) for the type-specific inner logic, then flushes
// the literal pool and zero-fills any remaining bytes.
void initAggregate(int typeSubPtr, int class, int (*body)(int)) {
    int totalSize;
    litptr = 0;
    totalSize = getStructSize(typeSubPtr);
    decGlobal(IDENT_VARIABLE, class == GLOBAL);
    if (isMatch("=")) {
        if (isMatch("{")) {
            (*body)(typeSubPtr);
            require("}");
        }
        else {
            error("initializer requires { }");
        }
    }
    dumpLitPool(1);
    dumpzero(1, totalSize - litptr);
}

void initStruct(int typeSubPtr, int class) {
    initAggregate(typeSubPtr, class, initStructBody);
}

void initUnion(int typeSubPtr, int class) {
    initAggregate(typeSubPtr, class, initUnionBody);
}

// recursive helper for global multi-dim array init.
// depth: current dimension index (0 = outermost)
int initGlbMDSub(int elemSz, int ndim, int dims[], int depth) {
    int i, dim, innerTot, j;
    int savedLit;
    dim = dims[depth];
    if (depth == ndim - 1) {
        // innermost dimension: parse scalar values
        i = dim;
        while (i) {
            initGlbElem(elemSz, IDENT_ARRAY, &i);
            if (isMatch(",") == 0) break;
        }
        // zero-fill remaining inner elements
        while (i > 0) {
            stowlit(0, elemSz);
            --i;
        }
        return;
    }
    // non-innermost: compute inner total element count
    innerTot = 1;
    j = depth + 1;
    while (j < ndim) {
        innerTot *= dims[j];
        ++j;
    }
    blanks();
    if (streq(lptr, "{")) {
        // nested braces mode
        i = 0;
        while (i < dim) {
            require("{");
            savedLit = litptr;
            initGlbMDSub(elemSz, ndim, dims, depth + 1);
            require("}");
            // zero-fill this row if short
            while (litptr < savedLit + innerTot * elemSz) {
                stowlit(0, elemSz);
            }
            ++i;
            if (isMatch(",") == 0) break;
        }
    }
    else {
        // flat mode: just parse all elements sequentially
        i = dim * innerTot;
        while (i) {
            initGlbElem(elemSz, IDENT_ARRAY, &i);
            if (isMatch(",") == 0) break;
        }
        // zero-fill remaining elements in flat mode
        while (i > 0) {
            stowlit(0, elemSz);
            --i;
        }
        return;
    }
    // zero-fill remaining rows (nested brace mode)
    while (i < dim) {
        j = 0;
        while (j < innerTot) {
            stowlit(0, elemSz);
            ++j;
        }
        ++i;
    }
}

// Initialize a global multi-dimensional array.
// handles both nested braces {{1,2},{3,4}} and flat {1,2,3,4}.
// type: e.g. TYPE_INT, TYPE_CHR
// typeSubPtr: struct def ptr if TYPE_STRUCT, else 0
// ndim: number of dimensions
// dims[]: dimension sizes
// class: GLOBAL or STATIC
void initGlbMDArr(int type, int typeSubPtr, int ndim, int dims[], int class) {
    int elemSz, totalElems, i;
    elemSz = type >> 2;
    if (type == TYPE_STRUCT)
        elemSz = getStructSize(typeSubPtr);
    if (elemSz < 1) elemSz = 1;
    totalElems = 1;
    i = 0;
    while (i < ndim) {
        totalElems *= dims[i];
        ++i;
    }
    litptr = 0;
    decGlobal(IDENT_ARRAY, class == GLOBAL);
    if (isMatch("=")) {
        if (isMatch("{")) {
            initGlbMDSub(elemSz, ndim, dims, 0);
            require("}");
        }
        else {
            error("array initializer requires { }");
        }
    }
    if (type == TYPE_STRUCT) {
        dumpLitPool(1);
        dumpzero(1, totalElems * elemSz - litptr);
    } else {
        dumpLitPool(elemSz);
        dumpzero(elemSz, totalElems - litptr / elemSz);
    }
}

// === Function Definition Parsing ============================================

// begin a function
// called from "parse" and tries to make a function
// out of the following text
int dofunction(int class) {
    int firstType, typeSubPtr;
    char *pGlobal, *funcSym;
    // declared arguments have formal types
    nogo = 0;                     // enable goto statements
    noloc = 0;                    // enable block-local declarations
    lastst = 0;                   // no statement yet
    litptr = 0;                   // clear lit pool
    litlab = getlabel();          // label next lit pool
    locptr = STARTLOC;            // clear local variables
    protoVoidList = 0;
    protoVariadic = 0;
    if (rettypeIsPtr && !typeIsPtr) {
        // Consume all '*' tokens that form the function's return pointer depth.
        int retPtrDepth;
        retPtrDepth = 0;
        while (isMatch("*")) retPtrDepth++;
        if (retPtrDepth == 0) retPtrDepth = 1;  // rettypeIsPtr set but no '*' (shouldn't happen)
        // Store depth on funcSym after it is added below.
        lastPtrDepth = retPtrDepth;
    }
    else if (typeIsPtr) {
        lastPtrDepth = 1;  // typedef-based pointer return; depth from typedef (defaulting to 1)
    }
    else {
        lastPtrDepth = 0;
    }
    if (monitor) {
        lout(line, stderr);
    }
    if (symname(ssname) == 0) {
        error("illegal function or declaration");
        errflag = 0;
        kill();                     // invalidate line
        return;
    }
    if (isreserved(ssname)) {
        error("reserved keyword used as name");
        kill();
        return;
    }
    // capture current function name for static local name mangling
    cptr2 = curfn;
    cptr3 = ssname;
    while (*cptr3) *cptr2++ = *cptr3++;
    *cptr2 = 0;
    // If this is in the symbol table as autoext or extern, update the entry.
    // Any other pre-existing entry is an unpermitted duplicate definition.
    if (pGlobal = findglb(ssname)) {
        if (pGlobal[CLASS] == AUTOEXT || pGlobal[CLASS] == PROTOEXT || pGlobal[CLASS] == EXTERNAL) {
            pGlobal[IDENT] = rettypeIsPtr ? IDENT_PTR_FUNCTION : IDENT_FUNCTION;
            pGlobal[CLASS] = class;
            if (rettype == TYPE_STRUCT) {
                pGlobal[TYPE] = TYPE_STRUCT;
                putint(rettypeSubPtr, pGlobal + CLASSPTR, 2);
            }
            funcSym = pGlobal;
        }
        else {
            // error: can't define something twice.
            multidef(ssname);
            funcSym = pGlobal;
        }
    }
    else {
        addSymbol(ssname, rettypeIsPtr ? IDENT_PTR_FUNCTION : IDENT_FUNCTION, rettype, 0, 0, &glbptr, class);
        if (rettype == TYPE_STRUCT)
            putint(rettypeSubPtr, cptr + CLASSPTR, 2);
        funcSym = cptr;
    }
    // Store pointer return depth on the function symbol.
    funcSym[PTRDEPTH] = lastPtrDepth;
    if (isMatch("(") == 0)
        error("no open paren");
    if ((firstType = dotype(&typeSubPtr)) != 0) {
        doArgsTyped(firstType, typeSubPtr);
    }
    else {
        doArgsKR();
    }
    // Determine whether this is a prototype (';' follows) or a definition.
    blanks();
    if (*lptr == ';') {
        // Prototype: record param types, mark as PROTOEXT.
        // EXTRN will only be emitted by trailer() if the function is actually called.
        funcSym[CLASS] = PROTOEXT;
        recordParamTypes(funcSym);
        locptr = STARTLOC;
        reqSemicolon();
        return;
    }
    // Definition: record param types, emit PUBLIC label, generate body.
    recordParamTypes(funcSym);
    // Restore ssname from curfn: arg parsing overwrites ssname with param names.
    cptr2 = ssname; cptr3 = curfn; while (*cptr3) *cptr2++ = *cptr3++; *cptr2 = 0;
    // don't do public if class == STATIC
    decGlobal(rettypeIsPtr ? IDENT_PTR_FUNCTION : IDENT_FUNCTION, class == GLOBAL);
    hasStaticLocal = 0;  // reset per-function before generating body
    gen(ENTER, 0);
    doStatement();
    // Flush any code left in the staging buffer by doReturn() for struct-
    // returning functions.  doReturn() activates the staging buffer via
    // level1() but never calls clearStage, so snext is non-zero on exit.
    clearStage(0, snext);
    if (lastst != STRETURN && lastst != STGOTO)
        gen(RETURN, 0);
    if (litptr) {
        toseg(DATASEG);
        gen(REFm, litlab);
        dumpLitPool(1);               // dump literals
    }
}

// Match a variadic ellipsis '...' in an argument list.
// If requireNamed is true, emit an error if no named parameter precedes it.
// On match: sets protoVariadic, consumes ')', returns 1.
// Returns 0 if '...' is not present.
int matchEllipsis(int requireNamed) {
    if (isMatch("...") == 0)
        return 0;
    if (requireNamed && argstk == 0)
        error("'...' must follow named parameter");
    protoVariadic = 1;
    require(")");
    return 1;
}

// Record function parameter types into paramTypes[] and store the index
// in the function symbol's FNPARAMPTR (SIZE) field.  Called from dofunction()
// for both prototypes and definitions.  The local symbol table must still hold
// the argument entries when called.  locptr is NOT reset here; callers manage
// that.
void recordParamTypes(char *funcSym) {
    int nparams, nparams_byte, k;
    char typebuf[32], depthbuf[32], *p;
    // If param types are already recorded (e.g. a prior prototype), skip.
    if (getint(funcSym + FNPARAMPTR, 2) != 0) {
        protoVoidList = 0;
        protoVariadic = 0;
        return;
    }
    // Empty parens with no void/variadic means unspecified params; skip.
    // Exception: for prototype declarations (CLASS == PROTOEXT), still store a
    // minimal 0-param entry as a "had prototype" marker. Without it, the
    // PROTOEXT->AUTOEXT upgrade in callfunc() leaves FNPARAMPTR==0, which is
    // the sentinel for "never declared," causing a false implicit-declaration
    // warning on every call after the first.
    if (locptr == STARTLOC && !protoVoidList && !protoVariadic) {
        if ((funcSym[CLASS] & 0x7F) != PROTOEXT) {
            protoVoidList = 0;
            protoVariadic = 0;
            return;
        }
        // Fall through: store a 0-param entry so FNPARAMPTR != 0.
    }
    // Collect types from the local symbol table.
    // Entries are variable-length (name field is only as wide as the name);
    // use nextsym() to step to the next entry, matching findloc()/nextsym().
    nparams = 0;
    p = STARTLOC;
    while (p < locptr && nparams < 32) {
        typebuf[nparams]  = p[TYPE];
        depthbuf[nparams] = p[PTRDEPTH];
        ++nparams;
        p = nextsym(p);
    }
    nparams_byte = (protoVariadic ? 0x80 : 0) | (nparams & 0x7F);
    k = storeParamTypes(typebuf, depthbuf, nparams_byte);
    if (k != 0)
        putint(k, funcSym + FNPARAMPTR, 2);
    protoVoidList = 0;
    protoVariadic = 0;
}

// Adjust a struct-by-value parameter to a hidden-pointer parameter.
// In Small-C, struct arguments are passed by caller-allocated hidden pointer;
// the callee receives a pointer to the struct, not the value directly.
// Called by both doArgsTyped and doArgsKRTypes to eliminate duplicated logic.
void toStructPtr(int *id, int *sz, int type) {
    if (type == TYPE_STRUCT && *id == IDENT_VARIABLE) {
        *id = IDENT_POINTER;
        *sz = BPW;
    }
}

// doArgsTyped: interpret a function argument list with declared types.
// in type: the type of the first variable in the argument list.
void doArgsTyped(int type, int typeSubPtr) {
    int id, sz, argConst;
    // dofunction() called dotype() to get the first param's type; typeIsConst
    // reflects whether 'const' was seen then. Subsequent args re-capture it
    // after each comma-triggered dotype() call in the loop below.
    argConst = typeIsConst;
    // Handle f(void) -- explicit zero-parameter prototype or definition.
    // But only when 'void' is NOT followed by '*' or '(': void* is a valid
    // param type, and void (*fn)(...) is a function-pointer parameter.
    if (type == TYPE_VOID) {
        blanks();
        if (ch != '*' && ch != '(') {
            if (isMatch(")") == 0)
                error("void must be only parameter");
            argstk = 0;
            argtop = BPW;
            csp = 0;
            protoVoidList = 1;
            return;
        }
        // Fall through: 'void *' or 'void (*name)(...)' parameter.
    }
    // Walk the parameter list. Register each param in the local symbol table
    // using offset argstk + 2*BPW, yielding sequential cdecl BP-relative
    // offsets: first arg -> [BP+4], second -> [BP+6], etc.
    argstk = 0;
    while (isMatch(")") == 0) {
        // check for variadic ellipsis
        if (matchEllipsis(1)) break;
        // Parse the declarator: handles *, (*name)(), name[], name.
        // Populates ssname, id (IDENT_POINTER or IDENT_VARIABLE), sz,
        // and lastNdim/lastStrides for multi-dim array params.
        // defArrTyp=IDENT_POINTER causes array params to decay to pointers.
        if (parseLocDecl(type, typeSubPtr, IDENT_POINTER, &id, &sz) == 0)
            break;
        if (findloc(ssname)) {
            multidef(ssname);
        }
        else {
            // Hidden pointer for struct params: caller pushes address.
            toStructPtr(&id, &sz, type);
            // Preserve const qualifier on parameter (stored in CLASS byte).
            // Offset argstk + 2*BPW gives sequential BP-relative offsets:
            // first arg -> [BP+4], second -> [BP+6], etc. (cdecl R-to-L push).
            addSymbol(ssname, id, type, sz, argstk + 2*BPW, &locptr,
                argConst ? (AUTOMATIC | CONST_FLAG) : AUTOMATIC);
            cptr[PTRDEPTH] = lastPtrDepth;
            applyDimMeta(cptr, type, typeSubPtr, 0);
            if (id != IDENT_POINTER
                && (type == TYPE_LONG || type == TYPE_ULONG))
                argstk += BPD;
            else
                argstk += BPW;
        }
        // advance to next arg or closing paren
        if (isMatch(",")) {
            if (matchEllipsis(0)) break;
            if ((type = dotype(&typeSubPtr)) == 0) {
                error("untyped argument declaration");
                break;
            }
            argConst = typeIsConst;  // re-capture const qualifier for next arg
        }
    }
    csp = 0;                       // preset stack ptr
    // argtop = total arg bytes + one word for pushed BP.
    // With cdecl (R-to-L push), offsets are already sequential starting at [BP+4];
    // no reversal loop needed.
    argtop = argstk + BPW;
    return;
}

// Recalculate argument offsets for K&R-style declarations after
// all types are known. Needed because long params take 4 bytes
// while the initial pass assumed all params were BPW (2 bytes).
void fixArgOffs() {
    int total, running, psz, namelen;
    char *ptr;
    // Pass 1: compute total argument bytes
    total = 0;
    ptr = STARTLOC;
    while (ptr < locptr) {
        if (ptr[IDENT] != IDENT_POINTER
            && (ptr[TYPE] == TYPE_LONG || ptr[TYPE] == TYPE_ULONG))
            psz = BPD;
        else
            psz = BPW;
        total += psz;
        ptr += NAME;
        namelen = 0;
        while (*ptr != namelen) { ptr += 1; namelen++; }
        ptr++;
    }
    argtop = total + BPW;
    // Pass 2: assign final offsets
    running = 0;
    ptr = STARTLOC;
    while (ptr < locptr) {
        if (ptr[IDENT] != IDENT_POINTER
            && (ptr[TYPE] == TYPE_LONG || ptr[TYPE] == TYPE_ULONG))
            psz = BPD;
        else
            psz = BPW;
        running += psz;
        // Sequential cdecl offsets: first arg -> [BP+4], next -> [BP+4+psz], etc.
        putint(running - psz + 2*BPW, ptr + OFFSET, 2);
        ptr += NAME;
        namelen = 0;
        while (*ptr != namelen) { ptr += 1; namelen++; }
        ptr++;
    }
}

// K&R second-pass type patcher: called once per type-declaration line by
// doArgsKR after all argument names have been collected (Phase 1).
// Patches IDENT, TYPE, and SIZE in the symbol table entries that Phase 1
// created.  Does NOT write final BP-relative offsets -- that is deferred to
// fixArgOffs() (Phase 3), which accounts for long args taking 4 bytes.
//
// argstk here is a BPW slot counter (one tick per name), NOT a byte offset.
// The offset field written by Phase 1 (sequential 0, BPW, 2*BPW, ...) is
// left untouched; fixArgOffs() always overwrites every slot's OFFSET.
void doArgsKRTypes(int type, int typeSubPtr) {
    int id, sz;
    char *ptr;
    while (argstk > 0) {
        if (parseLocDecl(type, typeSubPtr, IDENT_POINTER, &id, &sz)) {
            if (ptr = findloc(ssname)) {
                // Hidden pointer for struct params.
                toStructPtr(&id, &sz, type);
                ptr[IDENT] = id;
                ptr[TYPE] = type;
                ptr[PTRDEPTH] = lastPtrDepth;
                putint(sz, ptr + SIZE, 2);
                applyDimMeta(ptr, type, typeSubPtr, 0);
            }
            else {
                error("not an argument");
            }
        }
        argstk = argstk - BPW;            // decrement slot counter
        if (endst())
            return;
        if (isMatch(",") == 0)
            error("no comma or terminating semicolon");
    }
}

// interpret a function argument list without declared types (K&R) 
//   Phase 1 (above): each name is registered in the local symbol table
//     with a sequential BPW-slot offset and zero type/size.
//   Phase 2 (below): each type-declaration line is parsed; doArgsKRTypes
//     patches IDENT, TYPE, and SIZE for every name on that line.
//     argstk is decremented by BPW per name as a slot counter, not a
//     byte counter -- long args are still one slot at this stage.
//   Phase 3: fixArgOffs() recomputes true BP-relative byte offsets
//     accounting for long args occupying 4 bytes instead of 2.
void doArgsKR() {
    // count args
    argstk = 0;
    while (isMatch(")") == 0) {
        if (matchEllipsis(1)) break;
        if (symname(ssname)) {
            if (isreserved(ssname))
                error("reserved keyword used as name");
            else if (findloc(ssname))
                multidef(ssname);
            else {
                addSymbol(ssname, 0, 0, 0, argstk, &locptr, AUTOMATIC);
                argstk += BPW;
            }
        }
        else {
            error("illegal argument name");
            skipToNextToken();
        }
        blanks();
        if (streq(lptr, ")") == 0 && isMatch(",") == 0)
            error("no comma");
        if (endst())
            break;
    }
    csp = 0;                       // preset stack ptr
    argtop = argstk + BPW;         // account for the pushed BP
    while (argstk) {
        int type, typeSubPtr;
        type = dotype(&typeSubPtr);
        if (type != 0) {
            doArgsKRTypes(type, typeSubPtr);
            reqSemicolon();
        }
        else {
            error("wrong number of arguments");
            break;
        }
    }
    fixArgOffs();
    return;
}

// calcArgStrides: fill lastStrides[] and lastNdim for a declarator with
// 2+ array dimensions. When ndim <= 1, sets lastNdim = 0 (caller handles the
// single-dimension case). Used by doArgsTyped and parseLocDecl.
void calcArgStrides(int type, int typeSubPtr, int ndim, int dims[]) {
    int elemSz, k;
    if (ndim > 1) {
        if (type == TYPE_STRUCT)
            elemSz = getStructSize(typeSubPtr);
        else
            elemSz = type >> 2;
        if (elemSz < 1) elemSz = 1;
        lastStrides[ndim - 2] = dims[ndim - 1] * elemSz;
        k = ndim - 3;
        while (k >= 0) {
            lastStrides[k] = dims[k + 1] * lastStrides[k + 1];
            --k;
        }
        lastNdim = ndim;
    }
    else {
        lastNdim = 0;
    }
}

// === Local Declaration Parsing ==============================================

// Parse a single declarator after the base type has been consumed by dotype().
// Used for local variables, function arguments, and struct/union members.
//
// Inputs:
//   type       -- base type constant (TYPE_INT, TYPE_CHR, TYPE_STRUCT, ...)
//   typeSubPtr -- struct/union descriptor index when type == TYPE_STRUCT; else 0
//   defArrTyp  -- parsing context for array declarations:
//                   IDENT_ARRAY   -> local or struct member (explicit size required)
//                   IDENT_POINTER -> function argument (*name[] decays to pointer)
//   id, sz     -- out-parameters for declarator identity and byte size
//
// Outputs (via out-parameters and globals):
//   *id        <- IDENT_POINTER, IDENT_VARIABLE, or the effective array identity
//   *sz        <- byte size of the declared object (0 only for unsized char[] locals)
//   ssname     <- (global) validated identifier name
//   typeIsPtr  -- global set by dotype() when a typedef resolves to a pointer type;
//                 consumed (read then cleared) here so subsequent calls start clean
//   lastNdim   <- (global) number of array dimensions for multi-dim arrays, else 0
//   lastStrides[] <- (global) per-dimension strides for ndim > 1
//
// Returns 1 if the identifier name is valid, 0 (+ error) if invalid or unsupported.
// On a 0 return the caller should skip further processing of this declarator.
int parseLocDecl(int type, int typeSubPtr, int defArrTyp, int *id, int *sz) {
    int nameIsValid, hasFnPtrParen;

    // Default: not a multi-dim declarator. The array suffix path below
    // overrides this via calcArgStrides when ndim > 1.
    lastNdim = 0;

    // A leading '(' signals a function-pointer declarator: (*fp)(args).
    // Consume it now so the identifier name can still be reached and we
    // can emit a useful error below rather than crashing the parser.
    hasFnPtrParen = isMatch("(") ? 1 : 0;

    // Determine declarator identity and element size.
    // typeIsPtr is a global set by dotype() when a typedef alias resolves to
    // a pointer type (e.g. typedef int *P; P x;).  Treat it exactly like an
    // explicit '*' token and clear it so the next dotype() call starts clean.
    if (typeIsPtr || isMatch("*")) {
        typeIsPtr = 0;          // consumed; clear for subsequent callers
        *id = IDENT_POINTER;
        *sz = BPW;
        lastPtrDepth = 1;
        while (isMatch("*")) lastPtrDepth++;
    }
    else {
        lastPtrDepth = 0;
        *id = IDENT_VARIABLE;
        if (type == TYPE_VOID) {
            error("parameter or variable cannot have void type");
            *sz = BPW;          // error-recovery: use a pointer-sized slot
        }
        else if (type == TYPE_STRUCT) {
            *sz = getStructSize(typeSubPtr);
        }
        else {
            *sz = type >> 2;    // size in bytes is encoded in the high bits of the type constant
        }
    }

    // Parse and validate the identifier name.
    if ((nameIsValid = symname(ssname)) == 0) {
        illname();
    }
    else if (isreserved(ssname)) {
        error("reserved keyword used as name");
        nameIsValid = 0;
    }

    // Function-pointer declarator: (*fnName)(<params>).
    // The parameter list may be empty or typed (C90 prototype style).
    // Small-C treats all function pointer variables as plain pointers;
    // the parameter type annotations are consumed and discarded.
    // *id is already IDENT_POINTER (set above when '*' was consumed); the
    // symbol is registered as a plain pointer variable.
    if (hasFnPtrParen) {
        isMatch(")");           // consume the closing ')' of (*fnName)
        if (*id == IDENT_POINTER) {
            if (isMatch("(")) {
                // Consume the parameter list, tracking nested parentheses
                // to handle e.g. void (*fn)(int (*)(int)) correctly.
                if (isMatch(")") == 0) {
                    int depth;
                    depth = 1;
                    while (depth > 0) {
                        if (isMatch("("))      depth++;
                        else if (isMatch(")")) depth--;
                        else if (ch == 0)      break;
                        else                   gch();
                    }
                }
            }
            // (*fnName)(...) accepted; treated as a plain pointer variable.
        }
        // else: int (x) — parenthesized plain declarator name, valid C90;
        // paren was already consumed above, continue normally.
    }
    else if (isMatch("(")) {
        // 'type name(...)' without a preceding '(*' is not a variable declaration.
        error("function pointer declaration requires (*name)() form");
        while (ch && ch != ')')
            gch();
        isMatch(")");
        return 0;
    }

    // Parse optional array dimension suffix.
    if (*id == IDENT_POINTER && isMatch("[")) {
        // *name[N]: in argument position, decay to plain pointer.
        // In local variable position, allocate N pointer slots on the stack.
        int dim = parseArraySz();
        if (defArrTyp == IDENT_POINTER) {
            *sz = BPW;              // *name[] parameter adjusted to plain pointer
        }
        else {
            if (dim <= 0) {
                error("local pointer array requires explicit size");
                dim = 1;
            }
            *id = IDENT_PTR_ARRAY;
            *sz = dim * BPW;        // N pointer slots on the stack
        }
    }
    else if (*id == IDENT_VARIABLE && isMatch("[")) {
        int dims[MAX_DIMS], ndim;
        *id = defArrTyp;
        ndim = parseArrayDims(dims);
        if (ndim > 1) {
            // Multi-dimensional: compute per-dimension strides and total size.
            calcArgStrides(type, typeSubPtr, ndim, dims);
            *sz = dims[0] * lastStrides[0];
        }
        else {
            // Single dimension.
            if ((*sz *= dims[0]) == 0) {
                if (defArrTyp == IDENT_ARRAY
                    && (type == TYPE_CHR || type == TYPE_UCHR)) {
                    // 'char name[]' with no explicit size is legal.
                    // The actual element count is inferred from the string-literal
                    // Initializer; leave *sz == 0 and let initLocChrStr allocate
                    // the right stack space after seeing the initializer.
                }
                else {
                    if (defArrTyp == IDENT_ARRAY)
                        error("need array size");
                    *sz = BPW;  // unsized parameter array decays to pointer
                }
            }
        }
    }

    return nameIsValid;
}

// Copy a NUL-terminated symbol name into dst (up to NAMESIZE bytes).
// Used to save and restore ssname around static local name mangling.
void copyname(char *dst, char *src) {
    while (*src) *dst++ = *src++;
    *dst = 0;
}

// Makes a unique name for a static local and puts it into dst.
// Format: _L<N> where N is a unique label number (e.g. _L42).
// dst must be at least NAMESIZE bytes.
void makeNameLcSt(char *dst, int n) {
    char buf[8];
    int len;
    char *p, *q;
    p = dst;
    *p++ = '_';
    *p++ = 'L';
    if (n == 0) { *p++ = '0'; }
    else {
        q = buf;
        while (n > 0) { *q++ = (n % 10) + '0'; n /= 10; }
        len = q - buf;
        while (len > 0) *p++ = buf[--len];
    }
    *p = 0;
}

// Declare local variables for the current statement.
// type:      base type (TYPE_INT, TYPE_CHR, TYPE_STRUCT, ...) parsed by dotype()
// typeSubPtr: struct/enum descriptor pointer when type == TYPE_STRUCT
// isStatic:  1 if 'static' storage class preceded the type keyword
// isConst:   1 if 'const' qualifier was present (typeIsConst, cleared by dotype())
void declareLocals(int type, int typeSubPtr, int isStatic, int isConst) {
    int id, sz, N;
    // lclass only used in the auto (non-static) branch
    int lclass;
    // ddentry, glbEntry, localName only used in the static branch
    char *ddentry, *glbEntry;
    char localName[NAMESIZE];
    lclass = isConst ? (AUTOMATIC | CONST_FLAG) : AUTOMATIC;
    if (swactive) {
        error("not allowed in switch");
    }
    if (noloc) {
        error("not allowed with goto");
    }
    if (declared < 0) {
        error("must declare first in block");
    }
    while (1) {
        if (endst()) {
            return;
        }
        parseLocDecl(type, typeSubPtr, IDENT_ARRAY, &id, &sz);
        if (isStatic) {
            // static local: allocate in DATA segment, not on stack
            // save original name; ssname will be overwritten with the _L<N> label
            copyname(localName, ssname);
            N = getlabel();
            makeNameLcSt(ssname, N);  // ssname = "_L<N>" for DATA label + global entry
            emitLocStatic(type, id, sz);
            // after addSymbol(&glbptr), cptr = the new global DATA entry
            // CONST_FLAG goes on the global entry: primary() redirects is[SYMTAB_ADR]
            // there, so level1()'s const check reads the flag from the global entry.
            glbEntry = addSymbol(ssname, id, type, sz, 0, &glbptr,
                isConst ? (STATIC | CONST_FLAG) : STATIC);
            cptr[PTRDEPTH] = lastPtrDepth;
            ddentry = applyDimMeta(cptr, type, typeSubPtr, 0);   // allocate dimdata slot once
            // restore original name for the local scoping entry
            // OFFSET stores glbEntry so primary() can redirect STATIC-class accesses.
            // Plain STATIC class (no CONST_FLAG) keeps the redirect detection working.
            copyname(ssname, localName);
            // after addSymbol(&locptr), cptr = the new local scoping entry
            addSymbol(ssname, id, type, sz, glbEntry, &locptr, STATIC);
            applyDimMeta(cptr, type, typeSubPtr, ddentry);  // reuse slot; do NOT reallocate
        }
        else {
            // auto local: allocate on stack
            // 8086 ABI — align 4-byte (long) locals to an even offset
            if ((type >> 2) >= BPD && (declared & 1)) {
                declared++;
            }
            declared += sz;
            // lclass carries CONST_FLAG when the variable is const-qualified
            // after addSymbol(&locptr), cptr = the new auto local entry
            addSymbol(ssname, id, type, sz, csp - declared, &locptr, lclass);
            cptr[PTRDEPTH] = lastPtrDepth;
            // allocate dimdata slot if needed
            applyDimMeta(cptr, type, typeSubPtr, 0);
            if (isMatch("=")) {
                dispatchLocInit(type, typeSubPtr, id, sz);
            }
            else if (sz == 0 && id == IDENT_ARRAY) {
                error("need array size");
            }
        }
        if (isMatch(",") == 0)
            return;
    }
}

// allocate pending local variable space on the stack
void allocPendLoc() {
    if (declared > 0) {
        if (ncmp > 1) {
            nogo = declared;
        }
        gen(ADDSP, csp - declared);
        declared = 0;
    }
}

// Dispatch to the correct local-variable initializer after '=' is consumed.
// Handles the sz==0 array case (char arr[]="...") without calling allocPendLoc
// because the size is not yet known; initLocChrStr(1) infers and fixes it.
// All other cases call allocPendLoc() first to commit the stack frame slot.
// cptr must point to the symbol just added by addSymbol (needed by initLocMDArr).
// WARNING: the arms that call isMatch("{") are token-consuming side effects;
// their order encodes parsing priority and must not be changed.
void dispatchLocInit(int type, int typeSubPtr, int id, int sz) {
    if (sz == 0 && id == IDENT_ARRAY) {
        // char arr[] = "..." — size inferred from string literal
        initLocChrStr(1);
        return;
    }
    allocPendLoc();     // commit pending stack frame space before emitting stores
    if (type == TYPE_STRUCT && id == IDENT_VARIABLE) {
        if (typeSubPtr != -1 && ((char *)typeSubPtr)[STRDAT_KIND] == KIND_UNION)
            initLocUnion(typeSubPtr);
        else
            initLocStruct(typeSubPtr);
    }
    // multi-dim check must precede the 1D isMatch("{") to avoid consuming '{'
    else if (id == IDENT_ARRAY && lastNdim > 1 && isMatch("{")) {
        initLocMDArr(type, cptr);    // cptr = entry added by the preceding addSymbol
    }
    else if (type == TYPE_STRUCT && id == IDENT_ARRAY) {
        initLocStrArr(typeSubPtr);
    }
    else if (id == IDENT_ARRAY && isMatch("{")) {
        // '{' consumed above; initLocArray reads the element list
        initLocArray(type);
    }
    else if (id == IDENT_ARRAY && (type == TYPE_CHR || type == TYPE_UCHR)) {
        initLocChrStr(0);
    }
    else if (id == IDENT_ARRAY) {
        error("array initializer requires { }");
    }
    else if (id == IDENT_PTR_ARRAY) {
        if (isMatch("{"))
            initLocPtrArray();
        else
            error("pointer array initializer requires { }");
    }
    else {
        initLocScalar(type);
    }
}

// === Symbol Metadata Helpers ================================================

// Apply multi-dim or struct metadata to symbol table entry sym.
// sym is typically cptr (the entry just created by addSymbol); for K&R argument
// patching it may be any valid symbol table entry.
// Must be called while lastNdim/lastStrides reflect the current declarator.
// ddentry: 0 => allocate a new dimdata slot via storeDimDat (first-time use);
//          non-zero => reuse an already-allocated slot (e.g. static local alias).
// Returns the dimdata pointer (non-zero for multi-dim arrays, else 0).
char *applyDimMeta(char *sym, int type, int typeSubPtr, char *ddentry) {
    if (lastNdim > 1) {
        sym[NDIM] = lastNdim;
        if (ddentry == 0)
            ddentry = storeDimDat(typeSubPtr, lastNdim, lastStrides);
        putint(ddentry, sym + CLASSPTR, 2);
    }
    else if (type == TYPE_STRUCT) {
        putint(typeSubPtr, sym + CLASSPTR, 2);
    }
    return ddentry;
}

// === Local Initializer Code-Gen Helpers =====================================

// Helper: emit the store sequence after gen(POP2,0).
// BX has the destination address, AX has the value to store.
void genStore(int elemSz, int isPtr) {
    if (elemSz == BPD) {
        gen(PUTdp1, 0);
    }
    else if (isPtr || elemSz == BPW) {
        gen(PUTwp1, 0);
    }
    else {
        gen(PUTbp1, 0);
    }
}

// Helper: emit the zero-store sequence after gen(GETw1n,0) + gen(POP2,0).
// BX has the destination address, AX is already 0.
void genStoreZero(int elemSz) {
    if (elemSz == BPD) {
        gen(GETdxn, 0);
        gen(PUTdp1, 0);
    }
    else if (elemSz == BPW) {
        gen(PUTwp1, 0);
    }
    else {
        gen(PUTbp1, 0);
    }
}

// Helper: emit POINT1s(off), PUSH1, GETw1n(val), POP2, PUTbp1.
void genStoreByte(int off, int val) {
    gen(POINT1s, off);
    gen(PUSH1, 0);
    gen(GETw1n, val);
    gen(POP2, 0);
    gen(PUTbp1, 0);
}

// Helper: emit a zero-value store into the stack slot at off with element size elemSz.
// Sequence: POINT1s(off), PUSH1, GETw1n(0), POP2, genStoreZero(elemSz).
void genZeroElem(int off, int elemSz) {
    gen(POINT1s, off);
    gen(PUSH1, 0);
    gen(GETw1n, 0);
    gen(POP2, 0);
    genStoreZero(elemSz);
}

// Helper: parse one initializer expression and store it into the stack slot at off.
// Sequence: POINT1s(off), PUSH1, staged expr (widened to DX:AX if long dst), POP2, genStore.
void initLocElem(int off, int elemSz, int isPtr) {
    int isConst, val;
    int *before, *start;
    gen(POINT1s, off);
    gen(PUSH1, 0);
    setstage(&before, &start);
    parseExprWidened(&isConst, &val, elemSz == BPD);
    clearStage(before, start);
    gen(POP2, 0);
    genStore(elemSz, isPtr);
}

// === Local Scalar, 1D Array, and Char-String Initializers ===================

// Initialize a local scalar variable (int, char, pointer)
// cptr must point to the symbol table entry for the variable.
void initLocScalar(int type) {
    int offset, elemSize, isPtr;
    char *savedcptr;
    savedcptr = cptr;
    offset = getint(savedcptr + OFFSET, 2);
    isPtr = (savedcptr[IDENT] == IDENT_POINTER);
    elemSize = type >> 2;
    if (elemSize < 1) elemSize = 1;
    initLocElem(offset, elemSize, isPtr);
}

// Initialize a local array variable with { expr, expr, ... }
// cptr must point to the symbol table entry for the array.
void initLocArray(int type) {
    int offset, elemSize, dim, count;
    char *savedcptr;
    savedcptr = cptr;
    offset = getint(savedcptr + OFFSET, 2);
    elemSize = type >> 2;
    if (elemSize < 1) elemSize = 1;
    dim = getint(savedcptr + SIZE, 2) / elemSize;
    count = 0;
    while (count < dim) {
        initLocElem(offset + count * elemSize, elemSize, 0);
        count++;
        if (isMatch(",") == 0)
            break;
    }
    require("}");
    // zero-fill remaining elements
    while (count < dim) {
        genZeroElem(offset + count * elemSize, elemSize);
        count++;
    }
}

// Initialize a local pointer array variable with { expr, expr, ... }
// cptr must point to the symbol table entry for the array.
// '{' has already been consumed by the caller.
void initLocPtrArray() {
    int offset, dim, count;
    char *savedcptr;
    savedcptr = cptr;
    offset = getint(savedcptr + OFFSET, 2);
    dim = getint(savedcptr + SIZE, 2) / BPW;
    count = 0;
    while (count < dim) {
        initLocElem(offset + count * BPW, BPW, 1);
        count++;
        if (isMatch(",") == 0)
            break;
    }
    require("}");
    // zero-fill remaining slots
    while (count < dim) {
        genZeroElem(offset + count * BPW, BPW);
        count++;
    }
}

// Initialize a local char array from a string literal.
// if infer is true, the array had [] with no size, so we fix up the
// symbol table entry from the string length before allocating.
// if infer is false, the array has an explicit size and
// allocPendLoc() has already been called by the caller.
void initLocChrStr(int infer) {
    int strOffset, strLen;
    int offset, arrSize, i;
    char *savedcptr;
    savedcptr = cptr;
    if (string(&strOffset) == 0) {
        error("expected string literal");
        return;
    }
    strLen = litptr - strOffset;
    if (infer) {
        declared += strLen;
        putint(strLen, savedcptr + SIZE, 2);
        putint(csp - declared, savedcptr + OFFSET, 2);
        allocPendLoc();
    }
    offset = getint(savedcptr + OFFSET, 2);
    arrSize = getint(savedcptr + SIZE, 2);
    i = 0;
    while (i < strLen && i < arrSize) {
        genStoreByte(offset + i, litq[strOffset + i] & 255);
        i++;
    }
    while (i < arrSize) {
        genStoreByte(offset + i, 0);
        i++;
    }
    litptr = strOffset;
}

// === Local N-Dimensional Array Initializers =================================

// Reconstruct per-dimension element counts from stride metadata stored in
// the symbol's dimdata slot.  strides[k] holds the byte size of one slice
// at dimension k (i.e. dims[k+1]*...*dims[ndim-1]*elemSz).
// On return, dims[0..ndim-1] contain the element count for each dimension.
void deriveDims(char *symptr, int dims[], int elemSz, int ndim) {
    int strides[MAX_DIMS];
    int totalElems, i;
    char *ddentry;
    ddentry = getint(symptr + CLASSPTR, 2);
    totalElems = getint(symptr + SIZE, 2) / elemSz;
    // read stored strides (ndim-1 entries beginning at byte 2 of the slot)
    i = 0;
    while (i < ndim - 1) {
        strides[i] = getint(ddentry + 2 + i * 2, 2);
        ++i;
    }
    // innermost dimension: stride[ndim-2] / elemSz
    dims[ndim - 1] = strides[ndim - 2] / elemSz;
    // middle dimensions: ratio of adjacent strides
    i = ndim - 2;
    while (i > 0) {
        dims[i] = strides[i - 1] / strides[i];
        --i;
    }
    // outermost dimension: total elements divided by the product of all others
    dims[0] = totalElems;
    i = 1;
    while (i < ndim) {
        dims[0] /= dims[i];
        ++i;
    }
}

// Initialize the innermost row of a char/uchar multi-dim array.
// Accepts string literals (expanded byte-by-byte) and individual char
// expressions intermixed; zero-fills any slots not covered.
// baseOff: stack offset of the first byte in this row.
// dim:     number of bytes in this row.
void initLocMDCharRow(int baseOff, int dim) {
    int count, i, strOff, strLen;
    count = 0;
    while (count < dim) {
        if (string(&strOff)) {
            // expand string literal bytes (excluding NUL terminator) inline
            strLen = litptr - strOff - 1;
            i = 0;
            while (i < strLen && count < dim) {
                genStoreByte(baseOff + count, litq[strOff + i] & 255);
                ++count;
                ++i;
            }
            litptr = strOff;   // roll back literal pool; bytes emitted as immediates
        }
        else {
            initLocElem(baseOff + count, 1, 0);
            ++count;
        }
        if (isMatch(",") == 0) break;
    }
    // zero-fill any remaining bytes in this row
    while (count < dim) {
        genStoreByte(baseOff + count, 0);
        ++count;
    }
}

// Initialize the innermost row of a non-char multi-dim array.
// Parses element expressions and emits stores; zero-fills the remainder.
// baseOff: stack offset of the first element.
// elemSz:  element size in bytes.
// dim:     number of elements in this row.
void initLocMDElemRow(int elemSz, int baseOff, int dim) {
    int count;
    count = 0;
    while (count < dim) {
        initLocElem(baseOff + count * elemSz, elemSz, 0);
        ++count;
        if (isMatch(",") == 0) break;
    }
    // zero-fill remainder of this row
    while (count < dim) {
        genZeroElem(baseOff + count * elemSz, elemSz);
        ++count;
    }
}

// recursive helper for local multi-dim array init.
// Handles nested-brace {{r0},{r1}} and flat {e0,e1,...} initializer forms.
// type:    element type (e.g. TYPE_INT, TYPE_CHR)
// elemSz:  element size in bytes
// ndim:    total number of dimensions
// dims[]:  element count for each dimension
// depth:   current recursion depth (0 == outermost brace level)
// baseOff: stack offset of the first element reachable at this depth
void initLocMDSub(int type, int elemSz, int ndim, int dims[],
    int depth, int baseOff) {
    int i, dim, innerTot, j;
    dim = dims[depth];
    if (depth == ndim - 1) {
        // innermost dimension: dispatch by element type
        if (elemSz == 1 && (type == TYPE_CHR || type == TYPE_UCHR))
            initLocMDCharRow(baseOff, dim);
        else
            initLocMDElemRow(elemSz, baseOff, dim);
        return;
    }
    // compute total element count of one sub-array at this depth
    innerTot = 1;
    j = depth + 1;
    while (j < ndim) {
        innerTot *= dims[j];
        ++j;
    }
    blanks();
    if (streq(lptr, "{")) {
        // nested-brace mode: each sub-array is enclosed in { }
        i = 0;
        while (i < dim) {
            require("{");
            initLocMDSub(type, elemSz, ndim, dims,
                depth + 1,
                baseOff + i * innerTot * elemSz);
            require("}");
            ++i;
            if (isMatch(",") == 0) break;
        }
        // zero-fill sub-arrays not covered by the initializer
        while (i < dim) {
            j = 0;
            while (j < innerTot) {
                genZeroElem(baseOff + (i * innerTot + j) * elemSz, elemSz);
                ++j;
            }
            ++i;
        }
    }
    else {
        // flat mode: all elements in row-major order, no inner braces
        if (elemSz == 1 && (type == TYPE_CHR || type == TYPE_UCHR))
            initLocMDCharRow(baseOff, dim * innerTot);
        else
            initLocMDElemRow(elemSz, baseOff, dim * innerTot);
    }
}

// Initialize a local multi-dimensional array with { ... } syntax.
// Supports both nested-brace {{r0},{r1}} and flat {e0,e1,...} forms.
// symptr must point to the symbol table entry for the array.
void initLocMDArr(int type, char *symptr) {
    int offset, elemSz, ndim;
    int dims[MAX_DIMS];
    offset = getint(symptr + OFFSET, 2);
    ndim = symptr[NDIM];
    if (type == TYPE_STRUCT)
        elemSz = getStructSize(getClsPtr(symptr));
    else
        elemSz = type >> 2;
    if (elemSz < 1) elemSz = 1;
    deriveDims(symptr, dims, elemSz, ndim);
    initLocMDSub(type, elemSz, ndim, dims, 0, offset);
    require("}");
}

// === Local Struct and Union Initializers ====================================

// walk struct members and initialize each from expressions in current {...}.
// typeSubPtr points to the struct definition.
// baseOffset is the stack offset of the struct (or nested member) start.
void initLocStrMem(int typeSubPtr, int baseOffset) {
    char *membercur, *memberend;
    int memberSize, memberTotal, memberOffset;
    int nestedPtr;
    int elemCount, elemIdx, i;
    membercur = getint(typeSubPtr + STRDAT_MBEG, 2);
    memberend = getint(typeSubPtr + STRDAT_MEND, 2);
    while (membercur < memberend) {
        memberSize = membercur[STRMEM_TYPE] >> 2;
        if (memberSize < 1) memberSize = 1;
        memberTotal = getint(membercur + STRMEM_SIZE, 2);
        memberOffset = getint(membercur + STRMEM_OFFSET, 2);
        if (membercur[STRMEM_TYPE] == TYPE_STRUCT
            && membercur[STRMEM_IDENT] == IDENT_VARIABLE) {
            nestedPtr = getint(membercur + STRMEM_CLASSPTR, 2);
            require("{");
            initLocStrMem(nestedPtr, baseOffset + memberOffset);
            require("}");
        }
        else if (membercur[STRMEM_IDENT] == IDENT_ARRAY) {
            elemCount = memberTotal / memberSize;
            require("{");
            elemIdx = 0;
            while (elemIdx < elemCount) {
                initLocElem(baseOffset + memberOffset + elemIdx * memberSize,
                    memberSize, 0);
                elemIdx++;
                if (isMatch(",") == 0) break;
            }
            require("}");
            while (elemIdx < elemCount) {
                genZeroElem(baseOffset + memberOffset + elemIdx * memberSize,
                    memberSize);
                elemIdx++;
            }
        }
        else {
            initLocElem(baseOffset + memberOffset, memberSize,
                membercur[STRMEM_IDENT] == IDENT_POINTER);
        }
        membercur += STRMEM_MAX;
        if (membercur < memberend) {
            if (isMatch(",") == 0) break;
        }
    }
    // zero-fill remaining members
    while (membercur < memberend) {
        memberTotal = getint(membercur + STRMEM_SIZE, 2);
        memberOffset = getint(membercur + STRMEM_OFFSET, 2);
        i = 0;
        while (i < memberTotal) {
            genStoreByte(baseOffset + memberOffset + i, 0);
            i++;
        }
        membercur += STRMEM_MAX;
    }
}

// Initialize a local struct variable: struct tag v = { ... };
// cptr must point to the symbol table entry for the variable.
void initLocStruct(int typeSubPtr) {
    char *savedcptr;
    int baseOffset;
    savedcptr = cptr;
    baseOffset = getint(savedcptr + OFFSET, 2);
    if (isMatch("{") == 0) {
        error("struct initializer requires { }");
        return;
    }
    initLocStrMem(typeSubPtr, baseOffset);
    require("}");
}

// Initialize a local union variable: union tag v = { expr };
// Only the first member may be initialized; remaining union bytes are zeroed.
// cptr must point to the symbol table entry for the variable.
void initLocUnion(int typeSubPtr) {
    char *savedcptr, *firstMem;
    int baseOffset, unionSize, memberSize, memberTotal, i;
    savedcptr = cptr;
    baseOffset = getint(savedcptr + OFFSET, 2);
    unionSize = getStructSize(typeSubPtr);
    if (isMatch("{") == 0) {
        error("union initializer requires { }");
        return;
    }
    firstMem = getint(typeSubPtr + STRDAT_MBEG, 2);
    if (firstMem < getint(typeSubPtr + STRDAT_MEND, 2)) {
        memberSize = firstMem[STRMEM_TYPE] >> 2;
        if (memberSize < 1) memberSize = 1;
        memberTotal = getint(firstMem + STRMEM_SIZE, 2);
        // evaluate and store first member
        initLocElem(baseOffset, memberSize,
            firstMem[STRMEM_IDENT] == IDENT_POINTER);
        if (isMatch(",")) {
            error("union initializer only allows one value");
        }
        // zero-fill bytes after the first member up to union size
        i = memberTotal;
        while (i < unionSize) {
            genStoreByte(baseOffset + i, 0);
            i++;
        }
    }
    require("}");
}

// Initialize a local struct array: struct tag a[N] = {{...},{...},...};
// cptr must point to the symbol table entry for the array.
// cptr must point to the symbol table entry for the array.
void initLocStrArr(int typeSubPtr) {
    char *savedcptr;
    int baseOffset, structSize, dim, idx, i;
    savedcptr = cptr;
    baseOffset = getint(savedcptr + OFFSET, 2);
    structSize = getStructSize(typeSubPtr);
    dim = getint(savedcptr + SIZE, 2) / structSize;
    if (isMatch("{") == 0) {
        error("struct array initializer requires { }");
        return;
    }
    idx = 0;
    while (idx < dim) {
        require("{");
        initLocStrMem(typeSubPtr, baseOffset + idx * structSize);
        require("}");
        idx++;
        if (isMatch(",") == 0) break;
    }
    require("}");
    while (idx < dim) {
        i = 0;
        while (i < structSize) {
            genStoreByte(baseOffset + idx * structSize + i, 0);
            i++;
        }
        idx++;
    }
}

// === Static Local Variables =================================================

// Parse and emit one constant initializer value of the given element size.
// Handles 32-bit (BPD), 8-bit (1), and 16-bit cases.
// Returns 1 on success, 0 if the expression is not a constant (error emitted).
int emitConstVal(int esize) {
    int val, val_hi;
    if (esize == BPD) {
        if (isConstExpr32(&val, &val_hi) == 0) {
            error("static local: constant initializer required");
            return 0;
        }
        gen(WORD_, 0); outdec(val);     newline();
        gen(WORD_, 0); outdec(val_hi);  newline();
    }
    else {
        if (isConstExpr(&val) == 0) {
            error("static local: constant initializer required");
            return 0;
        }
        if (esize == 1) {
            gen(BYTE_, 0); outdec(val & 255); newline();
        }
        else {
            gen(WORD_, 0); outdec(val); newline();
        }
    }
    return 1;
}

// Emit DATA segment storage for a static local variable.
// ssname must hold the mangled label name on entry.
// Consumes "= initializer" if present; otherwise zero-fills.
// Flushes the staging buffer before switching segments.
// Returns after switching back to CODE segment.
void emitLocStatic(int type, int id, int sz) {
    int esize, dim, count;
    // compute element size and element count
    if (id == IDENT_POINTER || id == IDENT_PTR_ARRAY) {
        esize = BPW;
    }
    else {
        esize = type >> 2;
        if (esize < 1) esize = 1;
    }
    dim = sz / esize;
    if (dim < 1) dim = 1;
    // flush any pending stack allocations and staged code before switching segments
    allocPendLoc();
    clearStage(0, snext);
    hasStaticLocal = 1;  // prevent FPO from stripping ENTER during mid-function flushfunc()
    // emit label in DATA segment — no PUBLIC since CLASS == STATIC
    decGlobal(id, 0);
    if (isMatch("=")) {
        if (id == IDENT_ARRAY || id == IDENT_PTR_ARRAY) {
            if (isMatch("{")) {
                count = 0;
                while (count < dim) {
                    if (emitConstVal(esize) == 0) break;
                    count++;
                    if (isMatch(",") == 0) break;
                }
                require("}");
            }
            else {
                error("static local: array initializer requires { }");
                count = 0;
            }
            dumpzero(esize, dim - count);
        }
        else {
            // scalar or pointer
            emitConstVal(esize);
        }
    }
    else {
        // no initializer: zero-fill (static storage always initialized to zero)
        dumpzero(esize, dim);
    }
    // switch back to code segment so function body resumes correctly
    toseg(CODESEG);
}

// === Statement Dispatch and Control Flow ====================================

// Each call to doStatement() parses a single statement. Sequences of statements
// only occur within a compound statement; the loop that recognizes statement
// sequences is only found in doCompound().
// First, doStatement() looks for a data declaration, introduced by char, int,
// unsigned, unsigned char, or unsigned int. Finding one of these, it declares
// a local, passing the data type (CHR, INT, UCHR, or UINT). It then enforces
// the presence of a semicolon.
// If that fails, it looks for one of the tokens that introduces an executable
// statement. If successful, the corresponding parsing function is called, and
// the global lastst is set to a value indicating which statement was parsed.
int doStatement() {
    int type, typeSubPtr, isStatic;
    isStatic = 0;
    if (ch == 0 && eof) return;
    if (amatch("static", 6)) isStatic = 1;
    if ((type = dotype(&typeSubPtr)) != 0) {
        // Pass CONST_FLAG in the class so addSymbol stores it in CLASS byte.
        declareLocals(type, typeSubPtr, isStatic, typeIsConst);
        reqSemicolon();
    }
    else if (isStatic) {
        error("expected type after static");
    }
    else {
        if (declared >= 0) {
            if (ncmp > 1) {
                nogo = declared;
            }
            if (declared > 0) {
                gen(ADDSP, csp - declared);
            }
            declared = -1;
        }
        if (isMatch("{")) {
            doCompound();
        }
        else if (amatch("if", 2)) {
            doIf();
            lastst = STIF;
        }
        else if (amatch("while", 5)) {
            doWhile();
            lastst = STWHILE;
        }
        else if (amatch("do", 2)) {
            doDo();
            lastst = STDO;
        }
        else if (amatch("for", 3)) {
            doFor();
            lastst = STFOR;
        }
        else if (amatch("switch", 6)) {
            doSwitch();
            lastst = STSWITCH;
        }
        else if (amatch("case", 4)) {
            doCase();
            lastst = STCASE;
        }
        else if (amatch("default", 7)) {
            doDefault();
            lastst = STDEF;
        }
        else if (amatch("goto", 4)) {
            doGoto();
            lastst = STGOTO;
        }
        else if (doLabel()) {
            lastst = STLABEL;
        }
        else if (amatch("return", 6)) {
            doReturn();
            reqSemicolon();
            lastst = STRETURN;
        }
        else if (amatch("break", 5)) {
            doBreak();
            reqSemicolon();
            lastst = STBREAK;
        }
        else if (amatch("continue", 8)) {
            doContinue();
            reqSemicolon();
            lastst = STCONT;
        }
        else if (isMatch(";"))   {
            errflag = 0;
        }
        else if (isMatch("#asm")) {
            doAsmBlock();
            lastst = STASM;
        }
        else {
            doExpression(NO);
            reqSemicolon();
            lastst = STEXPR;
        }
    }
    return lastst;
}

void doCompound() {
    int savcsp; 
    char *savloc;
    savcsp = csp;
    savloc = locptr;
    declared = 0;           // may now declare local variables
    ++ncmp;                 // new level open
    while (isMatch("}") == 0)
        if (eof) {
            error("no final }");
            break;
        }
        else {
            // do one statement:
            doStatement();
        }
        // close current level
        if (--ncmp && lastst != STRETURN && lastst != STGOTO) {
            gen(ADDSP, savcsp);   // delete local variable space
        }
        cptr = savloc;          // retain labels
        while (cptr < locptr) {
            cptr2 = nextsym(cptr);
            if (cptr[IDENT] == IDENT_LABEL) {
                while (cptr < cptr2) {
                    *savloc++ = *cptr++;
                }
            }
            else {
                cptr = cptr2;
            }
        }
        locptr = savloc;        // delete local symbols
        declared = -1;          // may not declare variables
}

void doIf() {
    int flab1, flab2;
    genTestAndJmp(flab1 = getlabel(), YES);  // get expr, and branch false
    doStatement();                    // if true, do a statement
    if (amatch("else", 4) == 0) {    // if...else?
        // simple "if"...print false label
        gen(LABm, flab1);
        return;                       // and exit
    }
    flab2 = getlabel();
    if (lastst != STRETURN && lastst != STGOTO)
        gen(JMPm, flab2);
    gen(LABm, flab1);    // print false label
    doStatement();         // and do "else" clause
    gen(LABm, flab2);    // print true label
}

void doWhile() {
    int wq[4];              // allocate local queue
    addwhile(wq);           // add entry to queue for "break"
    gen(LABm, wq[WQLOOP]);  // loop label
    genTestAndJmp(wq[WQEXIT], YES);  // see if true
    doStatement();            // if so, do a statement
    gen(JMPm, wq[WQLOOP]);  // loop to label
    gen(LABm, wq[WQEXIT]);  // exit label
    delwhile();             // delete queue entry
}

void doDo() {
    int wq[4], top;
    addwhile(wq);
    gen(LABm, top = getlabel());
    doStatement();
    require("while");
    gen(LABm, wq[WQLOOP]);
    genTestAndJmp(wq[WQEXIT], YES);
    gen(JMPm, top);
    gen(LABm, wq[WQEXIT]);
    delwhile();
    reqSemicolon();
}

void doFor() {
    int wq[4], lab1, lab2;
    addwhile(wq);
    lab1 = getlabel();
    lab2 = getlabel();
    require("(");
    if (isMatch(";") == 0) {
        doExpression(NO);           // expr 1
        reqSemicolon();
    }
    gen(LABm, lab1);
    if (isMatch(";") == 0) {
        genTestAndJmp(wq[WQEXIT], NO); // expr 2
        reqSemicolon();
    }
    gen(JMPm, lab2);
    gen(LABm, wq[WQLOOP]);
    if (isMatch(")") == 0) {
        doExpression(NO);           // expr 3
        require(")");
    }
    gen(JMPm, lab1);
    gen(LABm, lab2);
    doStatement();
    gen(JMPm, wq[WQLOOP]);
    gen(LABm, wq[WQEXIT]);
    delwhile();
}

void doSwitch() {
    int wq[4], endlab, swact, swdef, swilg, *swnex, *swptr;
    swact = swactive;
    swdef = swdefault;
    swilg = swislong;
    swnex = swptr = swnext;
    addwhile(wq);
    *(wqptr + WQLOOP - WQSIZ) = 0;
    require("(");
    {
        int is[ISSIZE], savedloc;
        int *before, *start;
        savedloc = locptr;
        setstage(&before, &start);
        if (level1(is)) fetch(is);
        clearStage(before, start);
        locptr = savedloc;
        swislong = isLongVal(is);
    }
    require(")");
    swdefault = 0;
    swactive = 1;
    gen(JMPm, endlab = getlabel());
    doStatement();                // cases, etc.
    gen(JMPm, wq[WQEXIT]);
    gen(LABm, endlab);
    if (swislong) {
        gen(LSWITCHd, 0);
        while (swptr < swnext) {
            gen(NEARm, *swptr++);    // case label
            gen(WORDn, *swptr++);    // value lo
            gen(WORDn, *swptr++);    // value hi
        }
        gen(WORDn, 0);              // terminator
        gen(WORDn, 0);
        gen(WORDn, 0);
    }
    else {
        gen(SWITCH, 0);
        while (swptr < swnext) {
            gen(NEARm, *swptr++);
            gen(WORDn, *swptr++);    // case value
        }
        gen(WORDn, 0);
    }
    if (swdefault)
        gen(JMPm, swdefault);
    gen(LABm, wq[WQEXIT]);
    delwhile();
    swnext = swnex;
    swdefault = swdef;
    swactive = swact;
    swislong = swilg;
}

void doCase() {
    if (swactive == 0) error("not in switch");
    if (swnext > swend - (swislong ? 1 : 0)) {
        error("too many cases");
        return;
    }
    gen(LABm, *swnext++ = getlabel());
    if (swislong) {
        int val_hi, val_lo;
        isConstExpr32(&val_lo, &val_hi);
        // Sign-extend 16-bit case constants to 32-bit
        if (val_hi == 0 && (val_lo & 0x8000))
            val_hi = -1;
        *swnext++ = val_lo;
        *swnext++ = val_hi;
    }
    else {
        isConstExpr(swnext++);
    }
    require(":");
}

void doDefault() {
    if (swactive) {
        if (swdefault)
            error("multiple defaults");
    }
    else
        error("not in switch");
    require(":");
    gen(LABm, swdefault = getlabel());
}

void doGoto() {
    if (nogo > 0)
        error("not allowed with block-locals");
    else noloc = 1;
    if (symname(ssname))
        gen(JMPm, addLabel(NO));
    else
        error("bad label");
    reqSemicolon();
}

int doLabel() {
    char *savelptr;
    blanks();
    savelptr = lptr;
    if (symname(ssname)) {
        if (gch() == ':') {
            gen(LABm, addLabel(YES));
            return 1;
        }
        else bump(savelptr - lptr);
    }
    return 0;
}

int addLabel(int def) {
    if (cptr = findloc(ssname)) {
        if (cptr[IDENT] != IDENT_LABEL)
            error("not a label"); // same name used for variable
        else if (def) {
            if (cptr[TYPE])
                error("duplicate label");
            else
                cptr[TYPE] = YES;
        }
    }
    else {
        cptr = addSymbol(ssname, IDENT_LABEL, def, 0, getlabel(), &locptr, TYPE_LABEL);
    }
    return (getint(cptr + OFFSET, 2));
}

int doReturn() {
    int savcsp;
    if (endst() == 0) {
        if (rettype == TYPE_STRUCT) {
            // Return struct via hidden pointer (first caller-pushed arg).
            // Parse return expression as lvalue to get source address.
            int is[ISSIZE], savedlocptr, savcsp2;
            char *cpyfn, *retptr;
            savedlocptr = locptr;
            savcsp2 = csp;
            if (level1(is)) {
                if (is[TYP_OBJ] == TYPE_STRUCT) {
                    // AX = source address (local, member, dereferenced)
                }
                else if (is[TYP_OBJ] == 0 && (retptr = is[SYMTAB_ADR])
                         && retptr[TYPE] == TYPE_STRUCT) {
                    gen(POINT1m, retptr);
                }
                else {
                    error("must return a struct");
                }
            }
            // Emit structcp(dst, src, n) R-to-L: push n first (deepest [BP+8]),
            // then &src ([BP+6]), then dst (last [BP+4]).
            // AX = &src at this point.
            gen(MOVE21, 0);                             // BX = &src
            gen(GETw1n, getStructSize(rettypeSubPtr));  // AX = n
            gen(PUSH1, 0);                              // push n [BP+8]
            gen(SWAP12, 0);                             // AX = &src (from BX)
            gen(PUSH1, 0);                              // push &src [BP+6]
            gen(POINT1s, argtop + BPW);                 // AX = addr of hidden-ptr slot
            gen(GETw1p, 0);                             // AX = hidden_ptr (dst)
            gen(PUSH1, 0);                              // push dst [BP+4]
            cpyfn = findglb("structcp");
            if (cpyfn == 0)
                cpyfn = addSymbol("structcp", IDENT_FUNCTION, TYPE_INT, 0, 0, &glbptr, AUTOEXT);
            else if ((cpyfn[CLASS] & 0x7F) == PROTOEXT)
                cpyfn[CLASS] = AUTOEXT;  // ensure trailer() emits EXTRN
            gen(CALLm, cpyfn);
            gen(ADDSP, savcsp2);
            locptr = savedlocptr;
        }
        else {
            int is[ISSIZE], savedloc;
            int *before, *start;
            savedloc = locptr;
            setstage(&before, &start);
            if (level1(is)) fetch(is);
            locptr = savedloc;
            if ((rettype == TYPE_LONG || rettype == TYPE_ULONG)
                && !isLongVal(is)) {
                if (rettype == TYPE_ULONG || isUnsigned(is))
                    gen(WIDENu, 0);
                else
                    gen(WIDENs, 0);
            }
            clearStage(before, start);
        }
    }
    savcsp = csp;
    gen(RETURN, 0);
    csp = savcsp;
}

void doBreak() {
    int *ptr;
    if ((ptr = readwhile(wqptr)) == 0)
        return;
    gen(ADDSP, ptr[WQSP]);
    gen(JMPm, ptr[WQEXIT]);
}

void doContinue() {
    int *ptr;
    ptr = wqptr;
    while (1) {
        if ((ptr = readwhile(ptr)) == 0)
            return;
        if (ptr[WQLOOP])
            break;
    }
    gen(ADDSP, ptr[WQSP]);
    gen(JMPm, ptr[WQLOOP]);
}

void doAsmBlock() {
    ccode = 0;           // mark mode as "asm"
    hasInlineAsm = 1;    // suppress FP omission: inline asm may reference BP outside p-code view
    clearStage(0, snext);  // flush any pending staged p-codes
    flushfunc();           // flush funcbuf so inline asm lands in order
    while (1) {
        doInline();
        if (isMatch("#endasm"))
            break;
        if (eof)
            break;
        fputs(line, output);
    }
    kill();
    ccode = 1;
}

int doExpression(int use) {
    int isConst, val;
    int *before, *start;
    usexpr = use;        // tell isfree() whether expr value is used
    while (1) {
        setstage(&before, &start);
        parseExpr(&isConst, &val);
        clearStage(before, start);
        if (ch != ',')
            break;
        bump(1);
    }
    usexpr = YES;        // return to normal value
}

