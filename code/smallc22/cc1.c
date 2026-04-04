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

#include "stdio.h"
#include "notice.h"
#include "cc.h"
#include "ccstruct.h"

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
    *wq,       // while queue
    argcs,     // local copy of argc
    *argvs,    // local copy of argv
    *wqptr,    // ptr to next entry
    litptr,   // ptr to next entry
    macptr,   // macro buffer index
    pptr,     // ptr to parsing buffer
    ch,       // current character of input line
    nch,      // next character of input line
    declared, // # of local bytes to declare, -1 when declared
    iflevel,  // #if... nest level
    skiplevel,// level at which #if... skipping started
    nxtlab,   // next avail label #
    litlab,   // label # assigned to literal pool
    csp,      // compiler relative stk ptr
    argstk,   // function arg sp
    argtop,   // highest formal argument offset
    rettype,  // return type of current function (TYPE_INT default)
    rettypeSubPtr, // struct def ptr if rettype == TYPE_STRUCT
    ncmp,     // # open compound statements
    errflag,  // true after 1st error in statement
    eof,      // true on final input eof
    output,   // fd for output file
    files,    // true if file list specified on cmd line
    filearg,  // cur file arg index
    input = EOF, // fd for input file
    input2 = EOF, // fd for "#include" file
    usexpr = YES, // true if value of expression is used
    ccode = YES, // true while parsing C code
    *snext,    // next addr in stage
    *stail,    // last addr of data in stage
    *slast,    // last addr in stage
    listfp,   // file pointer to list device
    lastst,   // last parsed statement type
    oldseg,   // current segment (0, DATASEG, CODESEG)
    lineno,   // current line number in input file
    inclno,   // current line number in include file
    warncount; // number of warnings emitted this compilation

char
    optimize, // optimize output of staging buffer?
    alarm,    // audible alarm on errors?
    monitor,  // monitor function headers?
    pause,    // pause for operator on errors?
    nowarn,   // suppress warnings?
    *symtab,   // symbol table
    *litq,     // literal pool
    *macn,     // macro name buffer
    *macq,     // macro string buffer
    *pline,    // parsing buffer
    *mline,    // macro buffer
    *line,     // ptr to pline or mline
    *lptr,     // ptr to current character in "line"
    *glbptr,   // global symbol table
    *locptr,   // next local symbol table entry
    *cptr,     // work ptrs to any char buffer
    *cptr2,
    *cptr3,
    *dimdata,  // multi-dim array metadata buffer
    *dimdatptr, // next free entry in dimdata
    msname[NAMESIZE],   // macro symbol name
    ssname[NAMESIZE],   // symbol name (static locals mangled to _L<N> form)
    curfn[NAMESIZE],    // name of function currently being compiled
    infn[30],    // current input filename
    inclfn[30];  // current include filename

int
    lastNdim,            // ndim from last parseLocalDeclare call
    lastStrides[MAX_DIMS], // strides from last parseLocalDeclare call
    typeConstFlag;       // set by dotype(): 1 if const qualifier was seen

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
    ASR12,  ASL12,               // level10
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

// execution begins here
main(int argc, int *argv) {
    unsigned int availMem;
    fputs(VERSION, stderr);
    fputs(CRIGHT1, stderr);
    argcs = argc;
    argvs = argv;
    swnext = calloc(SWTABSZ, 1);
    swend = swnext + (SWTABSZ - SWSIZ) / BPW;
    stage = calloc(STAGESIZE, 2 * BPW);
    wqptr = wq = calloc(WQTABSZ, BPW);
    litq = calloc(LITABSZ, 1);
    macn = calloc(MACNSIZE, 1);
    macq = calloc(MACQSIZE, 1);
    pline = calloc(LINESIZE, 1);
    mline = calloc(LINESIZE, 1);
    slast = stage + (STAGESIZE * 2);
    symtab = calloc(NUMLOCS*SYMAVG + NUMGLBS*SYMMAX, 1);
    locptr = STARTLOC;
    glbptr = STARTGLB;
    dimdatptr = dimdata = calloc(DIMDATSZ, 1);
    initStructs();
    initEnums();
    availMem = avail(0);
    fprintf(stderr, "  Memory available: %d b\n", availMem);
    if (availMem < 2000) {
        fputs("out of memory\n", stderr);
        exit(1);
    }
    rettype = TYPE_INT;
    rettypeSubPtr = 0;
    warncount = 0;
    getRunArgs();   // get run options from command line arguments
    openfile();     // and initial input file
    preprocess();   // fetch first line
    header();       // intro code
    parse();        // process ALL input
    trailer();      // follow-up code
    fclose(output); // explicitly close output
    if (warncount > 0) {
        fprintf(stderr, "  %d warning(s). Press any key to continue...\n", warncount);
        fgetc(stderr);
    }
}

// ****************************************************************************
// High level Parsing
// ****************************************************************************

// Process all input text. At this level, only global declarations, defines,
// includes, structs, and function definitions are legal.
parse() {
    while (eof == 0) {
        if (amatch("extern", 6))
            dodeclare(EXTERNAL);
        else if (amatch("static", 6)) {
            dostatic();
        }
        else if (amatch("struct", 6)) {
            dostructblock();
        }
        else if (dodeclare(GLOBAL)) {
            // do nothing - for now, we only support global variable declarations of the form "extern int x;"
        }
        else if (IsMatch("#asm")) {
            doasm();
        }
        else if (IsMatch("#include")) {
            doinclude();
        }
        else if (IsMatch("#define")) {
            dodefine();
        }
        else {
            dofunction(GLOBAL);
        }
        blanks();                 // force eof if pending
    }
}

// do a static variable or function
dostatic() {
    if (dodeclare(STATIC)) {
        ;
    }
    else {
        dofunction(STATIC);
    }
}

// test for global declarations
dodeclare(int class) {
    int type, typeSubPtr, declclass;
    char *p;
    type = dotype(&typeSubPtr);
    // Incorporate const qualifier into the storage class byte.
    // Functions cannot be const; only variable declarations use this flag.
    declclass = typeConstFlag ? (class | CONST_FLAG) : class;
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
                dofunction(class);   // functions are never const
                rettype = TYPE_INT;
                rettypeSubPtr = 0;
                return 1;
            }
        }
        declglb(type, declclass, typeSubPtr);
    }
    else if (class == EXTERNAL) {
        declglb(TYPE_INT, class, typeSubPtr);
    }
    else {
        return 0;
    }
    ReqSemicolon();
    return 1;
}

// get type of declaration.
// Handles C90 type qualifiers: const, volatile, register, auto, signed,
// unsigned, short.  On 8086 short == int (both 16-bit).
// Sets global typeConstFlag = 1 when the const qualifier is present.
// "volatile" and "register" are accepted and silently ignored.
// "register" is treated as automatic storage (the default on 8086 where
// there are no spare GPRs available for user variables).
// @return type value of declaration, otherwise 0 if not a valid declaration.
dotype(int *typeSubPtr) {
    int isConst, isUnsigned, isShort, gotSignSize;
    isConst = isUnsigned = isShort = gotSignSize = 0;
    typeConstFlag = 0;
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
    typeConstFlag = isConst;
    // short [int] -- on 8086 short == int (16-bit word)
    if (isShort) {
        amatch("int", 3);
        return isUnsigned ? TYPE_UINT : TYPE_INT;
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
        if ((*typeSubPtr = getStructPtr()) == -1)
            error("unknown struct name");
        return TYPE_STRUCT;
    }
    if (amatch("enum", 4)) {
        return doEnum(typeSubPtr);
    }
    // If only const/volatile were seen with no base type, clear the flag
    // so callers do not misinterpret a non-declaration as const.
    typeConstFlag = 0;
    return 0;
}

// declare a global variable
declglb(int type, int class, int typeSubPtr) {
    int id, dim;
    int dims[MAX_DIMS], ndim, strides[MAX_DIMS];
    int totalSize, elemSz, k;
    char *ddentry;
    while (1) {
        if (endst()) {
            return;  // do line
        }
        if (IsMatch("*")) { 
            id = IDENT_POINTER; 
            dim = 0;
        }
        else { 
            id = IDENT_VARIABLE; 
            dim = 1; 
        }
        ndim = 0;
        if (symname(ssname) == 0)
            illname();
        else if (isreserved(ssname))
            error("reserved keyword used as name");
        if (findglb(ssname)) 
            multidef(ssname);
        if (id == IDENT_POINTER && IsMatch("[")) {
            id = IDENT_PTR_ARRAY;
            dims[0] = getArraySz();
            ndim = 1;
            dim = dims[0];
        }
        if (id == IDENT_VARIABLE) {
            if (IsMatch("(")) { 
                id = IDENT_FUNCTION; 
                Require(")"); 
            }
            else if (IsMatch("[")) { 
                id = IDENT_ARRAY;
                dims[0] = getArraySz();
                ndim = 1;
                while (IsMatch("[")) {
                    if (ndim >= MAX_DIMS) {
                        error("too many dimensions");
                        break;
                    }
                    dims[ndim++] = getArraySz();
                }
                dim = dims[0];
            }
        }
        if (class == EXTERNAL) 
            external(ssname, type >> 2, id);
        else if (id != IDENT_FUNCTION) {
            if (id == IDENT_PTR_ARRAY)
                dim = InitPtrArray(type, dim, class);
            else if (type == TYPE_STRUCT && id == IDENT_VARIABLE)
                InitStruct(typeSubPtr, class);
            else if (ndim > 1)
                initGlMDArr(type, typeSubPtr, ndim, dims, class);
            else {
                InitSize(type >> 2, id, dim, class);
            }
        }
        if (id == IDENT_PTR_ARRAY) {
            AddSymbol(ssname, id, type, dim * BPW, 0, &glbptr, class);
        }
        else if (id == IDENT_POINTER) {
            AddSymbol(ssname, id, type, BPW, 0, &glbptr, class);
        }
        else if (ndim > 1) {
            // multi-dimensional array
            if (type == TYPE_STRUCT)
                elemSz = getStructSize(typeSubPtr);
            else
                elemSz = type >> 2;
            if (elemSz < 1) elemSz = 1;
            // compute strides bottom-up
            strides[ndim - 2] = dims[ndim - 1] * elemSz;
            k = ndim - 3;
            while (k >= 0) {
                strides[k] = dims[k + 1] * strides[k + 1];
                --k;
            }
            totalSize = dims[0] * strides[0];
            ddentry = storeDimDat(typeSubPtr, ndim, strides);
            AddSymbol(ssname, id, type, totalSize, 0, &glbptr, class);
            cptr[NDIM] = ndim;
            putint(ddentry, cptr + CLASSPTR, 2);
        }
        else if (type == TYPE_STRUCT) {
            int structsize;
            structsize = getStructSize(typeSubPtr);
            AddSymbol(ssname, id, type, dim * structsize, 0, &glbptr, class);
            putint(typeSubPtr, cptr + CLASSPTR, 2);
        }
        else {
            AddSymbol(ssname, id, type, dim * (type >> 2), 0, &glbptr, class);
        }
        if (IsMatch(",") == 0) 
            return;
    }
}

// initialize global objects
InitSize(int size, int ident, int dim, int class) {
    int savedim;
    litptr = 0;
    if (dim == 0) {
        dim = -1;         // *... or ...[]
    }
    savedim = dim;
    decGlobal(ident, class == GLOBAL); // don't do public if class == STATIC
    if (IsMatch("=")) {
        if (IsMatch("{")) {
            while (dim) {
                EvalInitValue(size, ident, &dim);
                if (IsMatch(",") == 0) {
                    break;
                }
            }
            Require("}");
        }
        else {
            EvalInitValue(size, ident, &dim);
        }
    }
    if (savedim == -1 && dim == -1) {
        if (ident == IDENT_ARRAY) {
            error("need array size");
        }
        stowlit(0, size = BPW);
    }
    dumplits(size);
    dumpzero(size, dim);           // only if dim > 0
}

// evaluate one initializer
EvalInitValue(int size, int ident, int *dim) {
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
            if (IsCExpr32(&value, &value_hi)) {
                if (ident == IDENT_POINTER) {
                    error("cannot assign to pointer");
                }
                stowlit(value, BPW);
                stowlit(value_hi, BPW);
                *dim -= 1;
            }
        }
        else {
            if (IsConstExpr(&value)) {
                if (ident == IDENT_POINTER) {
                    error("cannot assign to pointer");
                }
                stowlit(value, size);
                *dim -= 1;
            }
        }
    }
}

// initialize a global pointer array (e.g. char *arr[] = {"str", 0})
// returns actual element count
InitPtrArray(int type, int dim, int class) {
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

    if (IsMatch("=")) {
        if (IsMatch("{")) {
            while (dim && count < MAXARRAYDIM) {
                if (string(&value)) {
                    isstr[count] = 1;
                    labels[count] = getlabel();
                    strstart[count] = value;
                    strend[count] = litptr;
                    ++count;
                    --dim;
                }
                else if (IsConstExpr(&value)) {
                    isstr[count] = 0;
                    values[count] = value;
                    ++count;
                    --dim;
                }
                else break;
                if (IsMatch(",") == 0) break;
            }
            Require("}");
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

// initialize a global struct variable with optional { ... } brace syntax
InitStruct(int typeSubPtr, int class) {
    char *membercur, *memberend;
    int memberSize, structSize, dim, memberTotal;
    litptr = 0;
    structSize = getStructSize(typeSubPtr);
    decGlobal(IDENT_VARIABLE, class == GLOBAL);
    if (IsMatch("=")) {
        if (IsMatch("{")) {
            membercur = getint(typeSubPtr + STRDAT_MBEG, 2);
            memberend = getint(typeSubPtr + STRDAT_MEND, 2);
            while (membercur < memberend) {
                memberSize = membercur[STRMEM_TYPE] >> 2;
                if (memberSize < 1) memberSize = 1;
                memberTotal = getint(membercur + STRMEM_SIZE, 2);
                dim = 1;
                EvalInitValue(memberSize, IDENT_VARIABLE, &dim);
                // Zero-fill remaining bytes for this member (e.g. array member)
                while (litptr < getint(membercur + STRMEM_OFFSET, 2)
                                + memberTotal) {
                    stowlit(0, 1);
                }
                membercur += STRMEM_MAX;
                if (membercur < memberend) {
                    if (IsMatch(",") == 0) break;
                }
            }
            Require("}");
        }
        else {
            error("struct initializer requires { }");
        }
    }
    dumplits(1);
    dumpzero(1, structSize - litptr);
}

// initialize a global multi-dimensional array.
// handles both nested braces {{1,2},{3,4}} and flat {1,2,3,4}.
// type: e.g. TYPE_INT, TYPE_CHR
// typeSubPtr: struct def ptr if TYPE_STRUCT, else 0
// ndim: number of dimensions
// dims[]: dimension sizes
// class: GLOBAL or STATIC
initGlMDArr(int type, int typeSubPtr, int ndim, int dims[], int class) {
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
    if (IsMatch("=")) {
        if (IsMatch("{")) {
            initGlMDSub(elemSz, ndim, dims, 0);
            Require("}");
        }
        else {
            error("array initializer requires { }");
        }
    }
    if (type == TYPE_STRUCT) {
        dumplits(1);
        dumpzero(1, totalElems * elemSz - litptr);
    } else {
        dumplits(elemSz);
        dumpzero(elemSz, totalElems - litptr / elemSz);
    }
}

// recursive helper for global multi-dim array init.
// depth: current dimension index (0 = outermost)
initGlMDSub(int elemSz, int ndim, int dims[], int depth) {
    int i, dim, innerTot, j;
    int savedLit;
    dim = dims[depth];
    if (depth == ndim - 1) {
        // innermost dimension: parse scalar values
        i = dim;
        while (i) {
            EvalInitValue(elemSz, IDENT_ARRAY, &i);
            if (IsMatch(",") == 0) break;
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
            Require("{");
            savedLit = litptr;
            initGlMDSub(elemSz, ndim, dims, depth + 1);
            Require("}");
            // zero-fill this row if short
            while (litptr < savedLit + innerTot * elemSz) {
                stowlit(0, elemSz);
            }
            ++i;
            if (IsMatch(",") == 0) break;
        }
    }
    else {
        // flat mode: just parse all elements sequentially
        i = dim * innerTot;
        while (i) {
            EvalInitValue(elemSz, IDENT_ARRAY, &i);
            if (IsMatch(",") == 0) break;
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

// get required array size
getArraySz() {
    int val;
    if (IsMatch("]")) 
        return 0; // null size
    if (IsConstExpr(&val) == 0) 
        val = 1;
    if (val < 0) {
        error("negative size illegal");
        val = -val;
    }
    Require("]");               // force single dimension
    return val;              // and return size
}

// open an include file
doinclude() {
    int i; char str[30];
    blanks();       // skip over to name
    if (*lptr == '"' || *lptr == '<')  {
        ++lptr;
    }
    i = 0;
    while (lptr[i] && lptr[i] != '"' && lptr[i] != '>' && lptr[i] != '\n' && lptr[i] != CR) {
        str[i] = lptr[i];
        ++i;
    }
    str[i] = NULL;
    if ((input2 = fopen(str, "r")) == NULL) {
        input2 = EOF;
        error("open failure on include file");
    }
    else {
        strcpy(inclfn, str);
        inclno = 0;
    }
    kill();   // make next read come from new file (if open)
}

// define a macro symbol
dodefine() {
    int k;
    if (symname(msname) == 0) {
        illname();
        kill();
        return;
    }
    k = 0;
    if (FindSymbol(msname, macn, NAMESIZE + 2, MACNEND, MACNBR, 0, NAMEMAX) == 0) {
        if (cptr2 = cptr)
            while (*cptr2++ = msname[k++]);
        else {
            error("macro name table full");
            return;
        }
    }
    putint(macptr, cptr + NAMESIZE, 2);
    while (white()) 
        gch();
    while (putmac(gch())) {
        // place a macro, character by character
    }
    if (macptr >= MACMAX) {
        error("macro string queue full");
        abort(ERRCODE);
    }
}

putmac(char c) {
    macq[macptr] = c;
    if (macptr < MACMAX)
        ++macptr;
    return c;
}

// begin a function
//
// called from "parse" and tries to make a function
// out of the following text
dofunction(int class) {
    int firstType, typeSubPtr;
    char *pGlobal;
    // declared arguments have formal types
    nogo = 0;                     // enable goto statements
    noloc = 0;                    // enable block-local declarations
    lastst = 0;                   // no statement yet
    litptr = 0;                   // clear lit pool
    litlab = getlabel();          // label next lit pool
    locptr = STARTLOC;            // clear local variables
    // skip "void" & locate header
    if (IsMatch("void")) {
        blanks();
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
    // If this name is already in the symbol table and is an autoext function,
    // define it instead as a global function
    if (pGlobal = findglb(ssname)) {
        if (pGlobal[CLASS] == AUTOEXT) {
            pGlobal[CLASS] = class;
            if (rettype == TYPE_STRUCT) {
                pGlobal[TYPE] = TYPE_STRUCT;
                putint(rettypeSubPtr, pGlobal + CLASSPTR, 2);
            }
        }
        else {
            // error: can't define something twice.
            multidef(ssname);
        }
    }
    else {
        AddSymbol(ssname, IDENT_FUNCTION, rettype, 0, 0, &glbptr, class);
        if (rettype == TYPE_STRUCT)
            putint(rettypeSubPtr, cptr + CLASSPTR, 2);
    }
    decGlobal(IDENT_FUNCTION, class == GLOBAL); // don't do public if class == STATIC
    if (IsMatch("(") == 0)
        error("no open paren");
    if ((firstType = dotype(&typeSubPtr)) != 0) {
        doArgsTyped(firstType, typeSubPtr);
    }
    else {
        doArgsNonTyped();
    }
    gen(ENTER, 0);
    statement();
    // Flush any code left in the staging buffer by doreturn() for struct-
    // returning functions.  doreturn() activates the staging buffer via
    // level1() but never calls ClearStage, so snext is non-zero on exit.
    // If left unflushed, subsequent functions overflow the buffer and all
    // gen() calls become no-ops, causing dumplits() to emit bare numbers
    // with no leading "DB" directive.
    ClearStage(0, snext);
    if (lastst != STRETURN && lastst != STGOTO)
        gen(RETURN, 0);
    if (litptr) {
        toseg(DATASEG);
        gen(REFm, litlab);
        dumplits(1);               // dump literals
    }
}

// doArgsTyped: interpret a function argument list with declared types.
// in type: the type of the first variable in the argument list.
doArgsTyped(int type, int typeSubPtr) {
    int id, sz, namelen, paren, argConst;
    char *ptr, *ddentry;
    argConst = typeConstFlag;   // capture const flag for first argument
    // get a list of all arguments. Set the name, id (Variable or Pointer),
    // type (unsigned/signed int/char), size, and 'argstk' for each. argstk is
    // the 0-based index of the variable on the stack. We will next reverse 
    // these values and offset them to account for the pushed BP value, which
    // will also be on the stack.
    argstk = 0;
    while (IsMatch(")") == 0) {
        // match opening parent for function ptr: (*arg)()
        if (IsMatch("("))
            paren = 1;
        else
            paren = 0;
        // match * for pointer, else variable
        if (IsMatch("*")) {
            id = IDENT_POINTER;
            sz = BPW;
        }
        else {
            id = IDENT_VARIABLE;
            sz = type >> 2;
        }
        if (symname(ssname)) {
            if (isreserved(ssname)) {
                error("reserved keyword used as name");
            }
            else if (findloc(ssname)) {
                multidef(ssname);
            }
            else {
                if (paren && IsMatch(")")) {
                    // unmatch paren for function ptr.
                }
                if (IsMatch("(")) {
                    if (!paren || id != IDENT_POINTER) {
                        error("not a valid function ptr, try (*...)()");
                    }
                    Require(")");
                }
                else if (id == IDENT_VARIABLE && IsMatch("[")) {
                    int dims[MAX_DIMS], ndim, elemSz, k;
                    char *ddentry;
                    id = IDENT_POINTER;
                    sz = BPW;
                    dims[0] = getArraySz();
                    ndim = 1;
                    while (IsMatch("[")) {
                        if (ndim >= MAX_DIMS) {
                            error("too many dimensions");
                            break;
                        }
                        dims[ndim++] = getArraySz();
                    }
                    if (ndim > 1) {
                        if (type == TYPE_STRUCT)
                            elemSz = getStructSize(
                                typeSubPtr);
                        else
                            elemSz = type >> 2;
                        if (elemSz < 1) elemSz = 1;
                        lastStrides[ndim - 2] =
                            dims[ndim - 1] * elemSz;
                        k = ndim - 3;
                        while (k >= 0) {
                            lastStrides[k] = dims[k + 1]
                                * lastStrides[k + 1];
                            --k;
                        }
                        lastNdim = ndim;
                    }
                    else {
                        lastNdim = 0;
                    }
                }
                // Hidden pointer for struct params: caller pushes address
                if (type == TYPE_STRUCT && id == IDENT_VARIABLE) {
                    id = IDENT_POINTER;
                    sz = BPW;
                }
                // Preserve const qualifier on parameter (stored in CLASS byte).
                AddSymbol(ssname, id, type, sz, argstk, &locptr,
                    argConst ? (AUTOMATIC | CONST_FLAG) : AUTOMATIC);
                if (lastNdim > 1) {
                    cptr[NDIM] = lastNdim;
                    ddentry = storeDimDat(typeSubPtr,
                        lastNdim, lastStrides);
                    putint(ddentry, cptr + CLASSPTR, 2);
                }
                else if (type == TYPE_STRUCT) {
                    putint(typeSubPtr, cptr + CLASSPTR, 2);
                }
                if (id != IDENT_POINTER
                    && (type == TYPE_LONG || type == TYPE_ULONG))
                    argstk += BPD;
                else
                    argstk += BPW;
            }
        }
        else {
            error("illegal argument name");
            break;
        }
        // advance to next arg or closing paren
        if (IsMatch(",")) {
            if ((type = dotype(&typeSubPtr)) == 0) {
                error("untyped argument declaration");
                break;
            }
            argConst = typeConstFlag;  // capture const flag for this argument
        }
    }
    csp = 0;                       // preset stack ptr
    // incremenet argtop by one word (space for pushed BP), and reverse the
    // placement of the arguments (per the SmallC specification, see Chapter 8
    // and Fig 8-1.
    argtop = argstk + BPW;
    ptr = STARTLOC;
    while (argstk) {
        int psz;
        if (ptr[IDENT] != IDENT_POINTER
            && (ptr[TYPE] == TYPE_LONG || ptr[TYPE] == TYPE_ULONG))
            psz = BPD;
        else
            psz = BPW;
        putint(argtop - getint(ptr + OFFSET, 2) - psz + BPW,
               ptr + OFFSET, 2);
        argstk -= psz;            // count down
        ptr += NAME;
        namelen = 0;
        while (*ptr != namelen) {
            ptr += 1;
            namelen++;
        }
        ptr++;
    }
    return;
}

// Recalculate argument offsets for K&R-style declarations after
// all types are known. Needed because long params take 4 bytes
// while the initial pass assumed all params were BPW (2 bytes).
fixArgOffs() {
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
        putint(argtop + BPW - running, ptr + OFFSET, 2);
        ptr += NAME;
        namelen = 0;
        while (*ptr != namelen) { ptr += 1; namelen++; }
        ptr++;
    }
}

// interpret a function argument list without declared types
doArgsNonTyped() {
    // count args
    argstk = 0;
    while (IsMatch(")") == 0) {
        if (symname(ssname)) {
            if (isreserved(ssname))
                error("reserved keyword used as name");
            else if (findloc(ssname))
                multidef(ssname);
            else {
                AddSymbol(ssname, 0, 0, 0, argstk, &locptr, AUTOMATIC);
                argstk += BPW;
            }
        }
        else {
            error("illegal argument name");
            skip();
        }
        blanks();
        if (streq(lptr, ")") == 0 && IsMatch(",") == 0)
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
            doArgTypes(type, typeSubPtr);
            ReqSemicolon();
        }
        else {
            error("wrong number of arguments");
            break;
        }
    }
    fixArgOffs();
    return;
}

// declare argument types
doArgTypes(int type, int typeSubPtr) {
    int id, sz;
    char *ptr, *ddentry;
    while (1) {
        if (argstk == 0)
            return;           // no arguments
        if (parseLocalDeclare(type, typeSubPtr, IDENT_POINTER, &id, &sz)) {
            if (ptr = findloc(ssname)) {
                // Hidden pointer for struct params
                if (type == TYPE_STRUCT && id == IDENT_VARIABLE) {
                    id = IDENT_POINTER;
                    sz = BPW;
                }
                ptr[IDENT] = id;
                ptr[TYPE] = type;
                putint(sz, ptr + SIZE, 2);
                putint(argtop - getint(ptr + OFFSET, 2), ptr + OFFSET, 2);
                if (lastNdim > 1) {
                    ptr[NDIM] = lastNdim;
                    ddentry = storeDimDat(typeSubPtr,
                        lastNdim, lastStrides);
                    putint(ddentry, ptr + CLASSPTR, 2);
                }
                else if (type == TYPE_STRUCT) {
                    putint(typeSubPtr, ptr + CLASSPTR, 2);
                }
            }
            else {
                error("not an argument");
            }
        }
        argstk = argstk - BPW;            // cnt down
        if (endst())
            return;
        if (IsMatch(",") == 0)
            error("no comma or terminating semicolon");
    }
}

// parse next local or argument declaration
// @param type value of type of variable, see cc.h
// @param defArrTyp only comes into play if this is an array declare:
//                        if IDENT_ARRAY, then must have array size;
//                        if IDENT_POINTER, then must be a pointer.
// @return 1 is name is valid, 0 + error if invalid.
parseLocalDeclare(int type, int typeSubPtr, int defArrTyp, int *id, int *sz) {
    int nameIsValid, hasOpenParen;
    hasOpenParen = IsMatch("(") ? 1 : 0;
    if (IsMatch("*")) {
        *id = IDENT_POINTER;
        *sz = BPW;
    }
    else {
        *id = IDENT_VARIABLE;
        if (type == TYPE_STRUCT) {
            *sz = getStructSize(typeSubPtr);
        }
        else {
            *sz = type >> 2;
        }
    }
    if ((nameIsValid = symname(ssname)) == 0) {
        illname();
    }
    else if (isreserved(ssname)) {
        error("reserved keyword used as name");
        nameIsValid = 0;
    }
    if (hasOpenParen && IsMatch(")")) {
        // unmatch paren
    }
    if (IsMatch("(")) {
        if (!hasOpenParen || *id != IDENT_POINTER) {
            error("Is this a function pointer? Try (*...)()");
        }
        Require(")");
    }
    else if (*id == IDENT_POINTER && IsMatch("[")) {
        // pointer array: *name[]
        getArraySz();     // consume [] size if present
        if (defArrTyp == IDENT_POINTER) {
            // function parameter: *name[] is just a pointer
            *sz = BPW;
        }
        else {
            error("local pointer arrays not supported");
        }
        lastNdim = 0;
    }
    else if (*id == IDENT_VARIABLE && IsMatch("[")) {
        int dims[MAX_DIMS], ndim, elemSz, k;
        *id = defArrTyp;
        dims[0] = getArraySz();
        ndim = 1;
        while (IsMatch("[")) {
            if (ndim >= MAX_DIMS) {
                error("too many dimensions");
                break;
            }
            dims[ndim++] = getArraySz();
        }
        if (ndim > 1) {
            // multi-dimensional: compute strides and total size
            if (type == TYPE_STRUCT)
                elemSz = getStructSize(typeSubPtr);
            else
                elemSz = type >> 2;
            if (elemSz < 1) elemSz = 1;
            lastStrides[ndim - 2] = dims[ndim - 1] * elemSz;
            k = ndim - 3;
            while (k >= 0) {
                lastStrides[k] = dims[k + 1]
                    * lastStrides[k + 1];
                --k;
            }
            *sz = dims[0] * lastStrides[0];
            lastNdim = ndim;
        }
        else {
            // single dimension (original path)
            lastNdim = 0;
            if ((*sz *= dims[0]) == 0) {
                if (defArrTyp == IDENT_ARRAY
                    && (type == TYPE_CHR || type == TYPE_UCHR)) {
                    // char arr[] — size inferred from string init
                }
                else {
                    if (defArrTyp == IDENT_ARRAY) {
                        error("need array size");
                    }
                    *sz = BPW;      // size of pointer argument
                }
            }
        }
    }
    else {
        lastNdim = 0;
    }
    return nameIsValid;
}

// ****************************************************************************
// 2nd level parsing
// ****************************************************************************

// Each call to statement() parses a single statement. Sequences of statements
// only occur within a compound statement; the loop that recognizes statement
// sequences is only found in compound().
// First, statement() looks for a data declaration, introduced by char, int,
// unsigned, unsigned char, or unsigned int. Finding one of these, it declares
// a local, passing the data type (CHR, INT, UCHR, or UINT). It then enforces
// the presence of a semicolon.
// If that fails, it looks for one of the tokens that introduces an executable
// statement. If successful, the corresponding parsing function is called, and
// the global lastst is set to a value indicating which statement was parsed.
statement() {
    int type, typeSubPtr, isStatic;
    isStatic = 0;
    if (ch == 0 && eof) return;
    if (amatch("static", 6)) isStatic = 1;
    if ((type = dotype(&typeSubPtr)) != 0) {
        // Pass CONST_FLAG in the class so AddSymbol stores it in CLASS byte.
        declareLocals(type, typeSubPtr, isStatic, typeConstFlag);
        ReqSemicolon();
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
        if (IsMatch("{")) {
            compound();
        }
        else if (amatch("if", 2)) {
            doif();
            lastst = STIF;
        }
        else if (amatch("while", 5)) {
            dowhile();
            lastst = STWHILE;
        }
        else if (amatch("do", 2)) {
            dodo();
            lastst = STDO;
        }
        else if (amatch("for", 3)) {
            dofor();
            lastst = STFOR;
        }
        else if (amatch("switch", 6)) {
            doswitch();
            lastst = STSWITCH;
        }
        else if (amatch("case", 4)) {
            docase();
            lastst = STCASE;
        }
        else if (amatch("default", 7)) {
            dodefault();
            lastst = STDEF;
        }
        else if (amatch("goto", 4)) {
            dogoto();
            lastst = STGOTO;
        }
        else if (dolabel()) {
            lastst = STLABEL;
        }
        else if (amatch("return", 6)) {
            doreturn();
            ReqSemicolon();
            lastst = STRETURN;
        }
        else if (amatch("break", 5)) {
            dobreak();
            ReqSemicolon();
            lastst = STBREAK;
        }
        else if (amatch("continue", 8)) {
            docont();
            ReqSemicolon();
            lastst = STCONT;
        }
        else if (IsMatch(";"))   {
            errflag = 0;
        }
        else if (IsMatch("#asm")) {
            doasm();
            lastst = STASM;
        }
        else {
            doexpr(NO);
            ReqSemicolon();
            lastst = STEXPR;
        }
    }
    return lastst;
}

// allocate pending local variable space on the stack
allocPendLoc() {
    if (declared > 0) {
        if (ncmp > 1) {
            nogo = declared;
        }
        gen(ADDSP, csp - declared);
        declared = 0;
    }
}

// Helper: emit the store sequence after gen(POP2,0).
// BX has the destination address, AX has the value to store.
genStore(int elemSz, int isPtr) {
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
genStoreZero(int elemSz) {
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
genStoreByte(int off, int val) {
    gen(POINT1s, off);
    gen(PUSH1, 0);
    gen(GETw1n, val);
    gen(POP2, 0);
    gen(PUTbp1, 0);
}

// initialize a local scalar variable (int, char, pointer)
// cptr must point to the symbol table entry for the variable.
initLocScalar(int type) {
    int isConst, val;
    int *before, *start;
    int offset, elemSize, isPtr;
    char *savedcptr;
    savedcptr = cptr;
    offset = getint(savedcptr + OFFSET, 2);
    isPtr = (savedcptr[IDENT] == IDENT_POINTER);
    elemSize = type >> 2;
    if (elemSize < 1) elemSize = 1;
    // point to variable address, save it, evaluate expression, store
    gen(POINT1s, offset);
    gen(PUSH1, 0);
    setstage(&before, &start);
    ParseExpression(&isConst, &val);
    ClearStage(before, start);
    gen(POP2, 0);
    genStore(elemSize, isPtr);
}

// initialize a local array variable with { expr, expr, ... }
// cptr must point to the symbol table entry for the array.
initLocArray(int type) {
    int isConst, val;
    int *before, *start;
    int offset, elemSize, dim, count;
    char *savedcptr;
    savedcptr = cptr;
    offset = getint(savedcptr + OFFSET, 2);
    elemSize = type >> 2;
    if (elemSize < 1) elemSize = 1;
    dim = getint(savedcptr + SIZE, 2) / elemSize;
    count = 0;
    while (count < dim) {
        gen(POINT1s, offset + count * elemSize);
        gen(PUSH1, 0);
        setstage(&before, &start);
        ParseExpression(&isConst, &val);
        ClearStage(before, start);
        gen(POP2, 0);
        genStore(elemSize, 0);
        count++;
        if (IsMatch(",") == 0)
            break;
    }
    Require("}");
    // zero-fill remaining elements
    while (count < dim) {
        gen(POINT1s, offset + count * elemSize);
        gen(PUSH1, 0);
        gen(GETw1n, 0);
        gen(POP2, 0);
        genStoreZero(elemSize);
        count++;
    }
}

// initialize a local multi-dimensional array with { ... } syntax.
// supports both nested braces and flat row-major form.
// symptr must point to the symbol table entry for the array.
initLcMDArr(int type, char *symptr) {
    int offset, elemSz, totalElems, ndim;
    int dims[MAX_DIMS], strides[MAX_DIMS];
    int i, k;
    char *ddentry;
    offset = getint(symptr + OFFSET, 2);
    ndim = symptr[NDIM];
    if (type == TYPE_STRUCT)
        elemSz = getStructSize(getClsPtr(symptr));
    else
        elemSz = type >> 2;
    if (elemSz < 1) elemSz = 1;
    // reconstruct dims from strides and total size
    ddentry = getint(symptr + CLASSPTR, 2);
    totalElems = getint(symptr + SIZE, 2) / elemSz;
    // read strides from dimdata
    i = 0;
    while (i < ndim - 1) {
        strides[i] = getint(ddentry + 2 + i * 2, 2);
        ++i;
    }
    // reconstruct dimension sizes from strides
    // strides[i] = dims[i+1] * strides[i+1] (or dims[ndim-1]*elemSz)
    dims[ndim - 1] = strides[ndim - 2] / elemSz;
    i = ndim - 2;
    while (i > 0) {
        dims[i] = strides[i - 1] / strides[i];
        --i;
    }
    dims[0] = totalElems;
    i = 1;
    while (i < ndim) {
        dims[0] /= dims[i];
        ++i;
    }
    initLcMDSub(type, elemSz, ndim, dims, 0, offset, totalElems);
    Require("}");
}

// recursive helper for local multi-dim array init.
// emits code to store each element at the correct stack offset.
initLcMDSub(int type, int elemSz, int ndim, int dims[],
    int depth, int baseOff, int totalSlots) {
    int i, dim, innerTot, j, isConst, val;
    int *before, *start;
    int count;
    int strOff, strLen;
    dim = dims[depth];
    if (depth == ndim - 1) {
        // innermost: check for string literal (char arrays)
        if (elemSz == 1 && (type == TYPE_CHR
            || type == TYPE_UCHR)) {
            count = 0;
            while (count < dim) {
                if (string(&strOff)) {
                    // copy string chars to stack
                    i = 0;
                    strLen = litptr - strOff - 1;
                    while (i < strLen && count < dim) {
                        genStoreByte(baseOff + count,
                            litq[strOff + i] & 255);
                        ++count;
                        ++i;
                    }
                    litptr = strOff;
                }
                else {
                    gen(POINT1s,
                        baseOff + count);
                    gen(PUSH1, 0);
                    setstage(&before, &start);
                    ParseExpression(&isConst, &val);
                    ClearStage(before, start);
                    gen(POP2, 0);
                    gen(PUTbp1, 0);
                    ++count;
                }
                if (IsMatch(",") == 0) break;
            }
            // zero-fill rest
            while (count < dim) {
                genStoreByte(baseOff + count, 0);
                ++count;
            }
            return;
        }
        // innermost: parse and emit each element
        count = 0;
        while (count < dim) {
            gen(POINT1s, baseOff + count * elemSz);
            gen(PUSH1, 0);
            setstage(&before, &start);
            ParseExpression(&isConst, &val);
            ClearStage(before, start);
            gen(POP2, 0);
            genStore(elemSz, 0);
            ++count;
            if (IsMatch(",") == 0) break;
        }
        // zero-fill rest of this row
        while (count < dim) {
            gen(POINT1s, baseOff + count * elemSz);
            gen(PUSH1, 0);
            gen(GETw1n, 0);
            gen(POP2, 0);
            genStoreZero(elemSz);
            ++count;
        }
        return;
    }
    // compute inner total elements per this-dim slot
    innerTot = 1;
    j = depth + 1;
    while (j < ndim) {
        innerTot *= dims[j];
        ++j;
    }
    blanks();
    if (streq(lptr, "{")) {
        // nested braces
        i = 0;
        while (i < dim) {
            Require("{");
            initLcMDSub(type, elemSz, ndim, dims,
                depth + 1,
                baseOff + i * innerTot * elemSz,
                innerTot);
            Require("}");
            ++i;
            if (IsMatch(",") == 0) break;
        }
    }
    else {
        // flat mode: sequential elements
        count = 0;
        while (count < dim * innerTot) {
            gen(POINT1s,
                baseOff + count * elemSz);
            gen(PUSH1, 0);
            setstage(&before, &start);
            ParseExpression(&isConst, &val);
            ClearStage(before, start);
            gen(POP2, 0);
            if (elemSz == BPW) gen(PUTwp1, 0);
            else gen(PUTbp1, 0);
            ++count;
            if (IsMatch(",") == 0) break;
        }
        // zero-fill remaining flat elements
        while (count < dim * innerTot) {
            gen(POINT1s,
                baseOff + count * elemSz);
            gen(PUSH1, 0);
            gen(GETw1n, 0);
            gen(POP2, 0);
            if (elemSz == BPW) gen(PUTwp1, 0);
            else gen(PUTbp1, 0);
            ++count;
        }
        return;
    }
    // zero-fill remaining rows (nested brace mode)
    while (i < dim) {
        j = 0;
        while (j < innerTot) {
            gen(POINT1s,
                baseOff + (i * innerTot + j) * elemSz);
            gen(PUSH1, 0);
            gen(GETw1n, 0);
            gen(POP2, 0);
            if (elemSz == BPW) gen(PUTwp1, 0);
            else gen(PUTbp1, 0);
            ++j;
        }
        ++i;
    }
}

// initialize a local char array from a string literal.
// if infer is true, the array had [] with no size, so we fix up the
// symbol table entry from the string length before allocating.
// if infer is false, the array has an explicit size and
// allocPendLoc() has already been called by the caller.
initLocChrStr(int infer) {
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

// walk struct members and initialize each from expressions in current {...}.
// typeSubPtr points to the struct definition.
// baseOffset is the stack offset of the struct (or nested member) start.
initLocStrMem(int typeSubPtr, int baseOffset) {
    char *membercur, *memberend;
    int memberSize, memberTotal, memberOffset;
    int nestedPtr;
    int isConst, val;
    int *before, *start;
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
            Require("{");
            initLocStrMem(nestedPtr,
                baseOffset + memberOffset);
            Require("}");
        }
        else if (membercur[STRMEM_IDENT] == IDENT_ARRAY) {
            elemCount = memberTotal / memberSize;
            Require("{");
            elemIdx = 0;
            while (elemIdx < elemCount) {
                gen(POINT1s,
                    baseOffset + memberOffset + elemIdx * memberSize);
                gen(PUSH1, 0);
                setstage(&before, &start);
                ParseExpression(&isConst, &val);
                ClearStage(before, start);
                gen(POP2, 0);
                genStore(memberSize, 0);
                elemIdx++;
                if (IsMatch(",") == 0) break;
            }
            Require("}");
            while (elemIdx < elemCount) {
                gen(POINT1s,
                    baseOffset + memberOffset + elemIdx * memberSize);
                gen(PUSH1, 0);
                gen(GETw1n, 0);
                gen(POP2, 0);
                genStoreZero(memberSize);
                elemIdx++;
            }
        }
        else {
            gen(POINT1s, baseOffset + memberOffset);
            gen(PUSH1, 0);
            setstage(&before, &start);
            ParseExpression(&isConst, &val);
            ClearStage(before, start);
            gen(POP2, 0);
            if (memberSize == BPD
                && membercur[STRMEM_IDENT] != IDENT_POINTER) {
                gen(PUTwp1, 0);
                gen(ADD2n, BPW);
                gen(GETw1n, 0);
                gen(PUTwp1, 0);
            }
            else if (memberSize == BPW
                || membercur[STRMEM_IDENT] == IDENT_POINTER) {
                gen(PUTwp1, 0);
            }
            else {
                gen(PUTbp1, 0);
            }
        }
        membercur += STRMEM_MAX;
        if (membercur < memberend) {
            if (IsMatch(",") == 0) break;
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

// initialize a local struct variable: struct tag v = { ... };
// cptr must point to the symbol table entry for the variable.
initLocStruct(int typeSubPtr) {
    char *savedcptr;
    int baseOffset;
    savedcptr = cptr;
    baseOffset = getint(savedcptr + OFFSET, 2);
    if (IsMatch("{") == 0) {
        error("struct initializer requires { }");
        return;
    }
    initLocStrMem(typeSubPtr, baseOffset);
    Require("}");
}

// initialize a local struct array: struct tag a[N] = {{...},{...},...};
// cptr must point to the symbol table entry for the array.
initLocStrArr(int typeSubPtr) {
    char *savedcptr;
    int baseOffset, structSize, dim, idx, i;
    savedcptr = cptr;
    baseOffset = getint(savedcptr + OFFSET, 2);
    structSize = getStructSize(typeSubPtr);
    dim = getint(savedcptr + SIZE, 2) / structSize;
    if (IsMatch("{") == 0) {
        error("struct array initializer requires { }");
        return;
    }
    idx = 0;
    while (idx < dim) {
        Require("{");
        initLocStrMem(typeSubPtr,
            baseOffset + idx * structSize);
        Require("}");
        idx++;
        if (IsMatch(",") == 0) break;
    }
    Require("}");
    while (idx < dim) {
        i = 0;
        while (i < structSize) {
            genStoreByte(baseOffset + idx * structSize + i, 0);
            i++;
        }
        idx++;
    }
}

// Build mangled name for a static local into dst.
// Format: _L<N> where N is a unique label number (e.g. _L42).
// dst must be at least NAMESIZE bytes.
buildmangle(char *dst, int n) {
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

// Emit DATA segment storage for a static local variable.
// ssname must hold the mangled label name on entry.
// Consumes "= initializer" if present; otherwise zero-fills.
// Flushes the staging buffer before switching segments.
// Returns after switching back to CODE segment.
emitStatLoc(int type, int id, int sz) {
    int esize, dim, count, val;
    // compute element size and element count
    if (id == IDENT_POINTER) {
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
    ClearStage(0, snext);
    // emit label in DATA segment — no PUBLIC since CLASS == STATIC
    decGlobal(id, 0);
    if (IsMatch("=")) {
        if (id == IDENT_ARRAY) {
            if (IsMatch("{")) {
                count = 0;
                while (count < dim) {
                    int val_hi;
                    if (esize == BPD) {
                        if (IsCExpr32(&val, &val_hi) == 0) {
                            error("static local: constant initializer required");
                            break;
                        }
                        gen(WORD_, 0); outdec(val);     newline();
                        gen(WORD_, 0); outdec(val_hi);  newline();
                    }
                    else {
                        if (IsConstExpr(&val) == 0) {
                            error("static local: constant initializer required");
                            break;
                        }
                        if (esize == 1) {
                            gen(BYTE_, 0); outdec(val & 255); newline();
                        }
                        else {
                            gen(WORD_, 0); outdec(val); newline();
                        }
                    }
                    count++;
                    if (IsMatch(",") == 0) break;
                }
                Require("}");
            }
            else {
                error("static local: array initializer requires { }");
                count = 0;
            }
            dumpzero(esize, dim - count);
        }
        else {
            // scalar or pointer
            int val_hi;
            if (esize == BPD) {
                if (IsCExpr32(&val, &val_hi) == 0) {
                    error("static local: constant initializer required");
                    val = 0; val_hi = 0;
                }
                gen(WORD_, 0); outdec(val);     newline();
                gen(WORD_, 0); outdec(val_hi);  newline();
            }
            else {
                if (IsConstExpr(&val) == 0) {
                    error("static local: constant initializer required");
                    val = 0;
                }
                if (esize == 1) {
                    gen(BYTE_, 0); outdec(val & 255); newline();
                }
                else {
                    gen(WORD_, 0); outdec(val); newline();
                }
            }
        }
    }
    else {
        // no initializer: zero-fill (static storage always initialized to zero)
        dumpzero(esize, dim);
    }
    // switch back to code segment so function body resumes correctly
    toseg(CODESEG);
}

// declare local variables
// isStatic: 1 if the 'static' keyword preceded the type
// isConst:  1 if the 'const' qualifier was on the type (stored in CLASS byte)
declareLocals(int type, int typeSubPtr, int isStatic, int isConst) {
    int id, sz, N, lclass;
    char *ddentry, *glbEntry;
    char localName[NAMESIZE];
    char *p, *q;
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
        parseLocalDeclare(type, typeSubPtr, IDENT_ARRAY, &id, &sz);
        if (isStatic) {
            // static local: allocate in DATA segment, not on stack
            // save the short local name (user-visible, stored in local entry)
            p = localName; q = ssname;
            while (*q) *p++ = *q++;
            *p = 0;
            // build unique _L<N> label into ssname for DATA segment and global entry
            N = getlabel();
            buildmangle(ssname, N);
            // emit DATA segment label + initializer (or zero-fill)
            emitStatLoc(type, id, sz);
            // add to global table under _L<N> name
            // CONST_FLAG goes on the global entry: after primary() redirects
            // is[SYMTAB_ADR] to the global entry, the const check in level1()
            // reads the flag from there.
            glbEntry = AddSymbol(ssname, id, type, sz, 0, &glbptr,
                isConst ? (STATIC | CONST_FLAG) : STATIC);
            if (lastNdim > 1) {
                cptr[NDIM] = lastNdim;
                ddentry = storeDimDat(typeSubPtr, lastNdim, lastStrides);
                putint(ddentry, cptr + CLASSPTR, 2);
            }
            else if (type == TYPE_STRUCT) {
                putint(typeSubPtr, cptr + CLASSPTR, 2);
            }
            // restore short name; add local entry for scoping
            // OFFSET holds the global entry pointer for redirect in primary()
            // Local entry uses plain STATIC (no CONST_FLAG) so that the
            // ptr[CLASS]==STATIC redirect detection in primary() still works.
            p = ssname; q = localName;
            while (*q) *p++ = *q++;
            *p = 0;
            AddSymbol(ssname, id, type, sz, glbEntry, &locptr, STATIC);
            if (lastNdim > 1) {
                cptr[NDIM] = lastNdim;
                putint(ddentry, cptr + CLASSPTR, 2);
            }
            else if (type == TYPE_STRUCT) {
                putint(typeSubPtr, cptr + CLASSPTR, 2);
            }
            // no stack space needed for static locals
        }
        else {
            // pad to even alignment for long (4-byte) locals
            if ((type >> 2) >= BPD && (declared & 1))
                declared++;
            declared += sz;
            // lclass carries CONST_FLAG when the variable is const-qualified.
            AddSymbol(ssname, id, type, sz, csp - declared, &locptr, lclass);
            if (lastNdim > 1) {
                cptr[NDIM] = lastNdim;
                ddentry = storeDimDat(typeSubPtr, lastNdim,
                    lastStrides);
                putint(ddentry, cptr + CLASSPTR, 2);
            }
            else if (type == TYPE_STRUCT) {
                putint(typeSubPtr, cptr + CLASSPTR, 2);
            }
            if (IsMatch("=")) {
                if (sz == 0 && id == IDENT_ARRAY) {
                    // char arr[] = "..." — infer size from string
                    initLocChrStr(1);
                }
                else {
                    allocPendLoc();
                    if (type == TYPE_STRUCT && id == IDENT_VARIABLE) {
                        initLocStruct(typeSubPtr);
                    }
                    else if (id == IDENT_ARRAY && lastNdim > 1
                        && IsMatch("{")) {
                        initLcMDArr(type, cptr);
                    }
                    else if (type == TYPE_STRUCT && id == IDENT_ARRAY) {
                        initLocStrArr(typeSubPtr);
                    }
                    else if (id == IDENT_ARRAY && IsMatch("{")) {
                        initLocArray(type);
                    }
                    else if (id == IDENT_ARRAY
                        && (type == TYPE_CHR || type == TYPE_UCHR)) {
                        initLocChrStr(0);
                    }
                    else if (id == IDENT_ARRAY) {
                        error("array initializer requires { }");
                    }
                    else {
                        initLocScalar(type);
                    }
                }
            }
            else if (sz == 0 && id == IDENT_ARRAY) {
                error("need array size");
            }
        }
        if (IsMatch(",") == 0)
            return;
    }
}

compound() {
    int savcsp; 
    char *savloc;
    savcsp = csp;
    savloc = locptr;
    declared = 0;           // may now declare local variables
    ++ncmp;                 // new level open
    while (IsMatch("}") == 0)
        if (eof) {
            error("no final }");
            break;
        }
        else {
            // do one statement:
            statement();
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

doif() {
    int flab1, flab2;
    GenTestAndJmp(flab1 = getlabel(), YES);  // get expr, and branch false
    statement();                    // if true, do a statement
    if (amatch("else", 4) == 0) {    // if...else?
        // simple "if"...print false label
        gen(LABm, flab1);
        return;                       // and exit
    }
    flab2 = getlabel();
    if (lastst != STRETURN && lastst != STGOTO)
        gen(JMPm, flab2);
    gen(LABm, flab1);    // print false label
    statement();         // and do "else" clause
    gen(LABm, flab2);    // print true label
}

dowhile() {
    int wq[4];              // allocate local queue
    addwhile(wq);           // add entry to queue for "break"
    gen(LABm, wq[WQLOOP]);  // loop label
    GenTestAndJmp(wq[WQEXIT], YES);  // see if true
    statement();            // if so, do a statement
    gen(JMPm, wq[WQLOOP]);  // loop to label
    gen(LABm, wq[WQEXIT]);  // exit label
    delwhile();             // delete queue entry
}

dodo() {
    int wq[4];
    addwhile(wq);
    gen(LABm, wq[WQLOOP]);
    statement();
    Require("while");
    GenTestAndJmp(wq[WQEXIT], YES);
    gen(JMPm, wq[WQLOOP]);
    gen(LABm, wq[WQEXIT]);
    delwhile();
    ReqSemicolon();
}

dofor() {
    int wq[4], lab1, lab2;
    addwhile(wq);
    lab1 = getlabel();
    lab2 = getlabel();
    Require("(");
    if (IsMatch(";") == 0) {
        doexpr(NO);           // expr 1
        ReqSemicolon();
    }
    gen(LABm, lab1);
    if (IsMatch(";") == 0) {
        GenTestAndJmp(wq[WQEXIT], NO); // expr 2
        ReqSemicolon();
    }
    gen(JMPm, lab2);
    gen(LABm, wq[WQLOOP]);
    if (IsMatch(")") == 0) {
        doexpr(NO);           // expr 3
        Require(")");
    }
    gen(JMPm, lab1);
    gen(LABm, lab2);
    statement();
    gen(JMPm, wq[WQLOOP]);
    gen(LABm, wq[WQEXIT]);
    delwhile();
}

doswitch() {
    int wq[4], endlab, swact, swdef, swilg, *swnex, *swptr;
    swact = swactive;
    swdef = swdefault;
    swilg = swislong;
    swnex = swptr = swnext;
    addwhile(wq);
    *(wqptr + WQLOOP - WQSIZ) = 0;
    Require("(");
    {
        int is[ISSIZE], savedloc;
        int *before, *start;
        savedloc = locptr;
        setstage(&before, &start);
        if (level1(is)) fetch(is);
        ClearStage(before, start);
        locptr = savedloc;
        swislong = isLongVal(is);
    }
    Require(")");
    swdefault = 0;
    swactive = 1;
    gen(JMPm, endlab = getlabel());
    statement();                // cases, etc.
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

docase() {
    if (swactive == 0) error("not in switch");
    if (swnext > swend - (swislong ? 1 : 0)) {
        error("too many cases");
        return;
    }
    gen(LABm, *swnext++ = getlabel());
    if (swislong) {
        int val_hi, val_lo;
        IsCExpr32(&val_lo, &val_hi);
        // Sign-extend 16-bit case constants to 32-bit
        if (val_hi == 0 && (val_lo & 0x8000))
            val_hi = -1;
        *swnext++ = val_lo;
        *swnext++ = val_hi;
    }
    else {
        IsConstExpr(swnext++);
    }
    Require(":");
}

dodefault() {
    if (swactive) {
        if (swdefault)
            error("multiple defaults");
    }
    else
        error("not in switch");
    Require(":");
    gen(LABm, swdefault = getlabel());
}

dogoto() {
    if (nogo > 0)
        error("not allowed with block-locals");
    else noloc = 1;
    if (symname(ssname))
        gen(JMPm, addlabel(NO));
    else
        error("bad label");
    ReqSemicolon();
}

dolabel() {
    char *savelptr;
    blanks();
    savelptr = lptr;
    if (symname(ssname)) {
        if (gch() == ':') {
            gen(LABm, addlabel(YES));
            return 1;
        }
        else bump(savelptr - lptr);
    }
    return 0;
}

addlabel(int def) {
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
        cptr = AddSymbol(ssname, IDENT_LABEL, def, 0, getlabel(), &locptr, TYPE_LABEL);
    }
    return (getint(cptr + OFFSET, 2));
}

doreturn() {
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
            // Emit structcp(dst, src, n) call.
            // AX = src. Push src, load dst, swap so stack is [dst][src].
            gen(PUSH1, 0);
            gen(POINT1s, argtop + BPW);
            gen(GETw1p, 0);
            gen(SWAP1s, 0);
            gen(PUSH1, 0);
            gen(GETw1n, getStructSize(rettypeSubPtr));
            gen(PUSH1, 0);
            cpyfn = findglb("structcp");
            if (cpyfn == 0)
                cpyfn = AddSymbol("structcp", IDENT_FUNCTION, TYPE_INT,
                    0, 0, &glbptr, AUTOEXT);
            gen(ARGCNTn, 3);
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
                if (rettype == TYPE_ULONG || IsUnsigned(is))
                    gen(WIDENu, 0);
                else
                    gen(WIDENs, 0);
            }
            ClearStage(before, start);
        }
    }
    savcsp = csp;
    gen(RETURN, 0);
    csp = savcsp;
}

dobreak() {
    int *ptr;
    if ((ptr = readwhile(wqptr)) == 0)
        return;
    gen(ADDSP, ptr[WQSP]);
    gen(JMPm, ptr[WQEXIT]);
}

docont() {
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

doasm() {
    ccode = 0;           // mark mode as "asm"
    while (1) {
        doInline();
        if (IsMatch("#endasm"))
            break;
        if (eof)
            break;
        fputs(line, output);
    }
    kill();
    ccode = 1;
}

doexpr(int use) {
    int isConst, val;
    int *before, *start;
    usexpr = use;        // tell isfree() whether expr value is used
    while (1) {
        setstage(&before, &start);
        ParseExpression(&isConst, &val);
        ClearStage(before, start);
        if (ch != ',')
            break;
        bump(1);
    }
    usexpr = YES;        // return to normal value
}

// ****************************************************************************
// miscellaneous functions
// ****************************************************************************

// get run options
getRunArgs() {
    int i;
    i = listfp = nxtlab = 0;
    output = stdout;
    optimize = YES;
    alarm = monitor = pause = nowarn = NO;
    line = mline;
    while (getarg(++i, line, LINESIZE, argcs, argvs) != EOF) {
        if (line[0] != '-' && line[0] != '/')
            continue;
        if (toupper(line[1]) == 'L' && isdigit(line[2]) && line[3] <= ' ') {
            listfp = line[2] - '0';
            continue;
        }
        if (toupper(line[1]) == 'N' && toupper(line[2]) == 'O' && 
            line[3] <= ' ') {
            optimize = NO;
            continue;
        }
        if (line[2] <= ' ') {
            if (toupper(line[1]) == 'A') {
                alarm = YES;
                continue;
            }
            if (toupper(line[1]) == 'M') {
                monitor = YES; 
                continue;
            }
            if (toupper(line[1]) == 'P') {
                pause = YES;
                continue;
            }
            if (toupper(line[1]) == 'W') {
                nowarn = YES;
                continue;
            }
        }
        fputs("usage: cc [file]... [-m] [-a] [-p] [-w] [-l#] [-no]\n", stderr);
        abort(ERRCODE);
    }
}

// input and output file opens
openfile() {        // entire function revised
    char outfn[15];
    int i, j, ext;
    input = EOF;
    while (getarg(++filearg, pline, LINESIZE, argcs, argvs) != EOF) {
        if (pline[0] == '-' || pline[0] == '/') continue;
        ext = NO;
        i = -1;
        j = 0;
        while (pline[++i]) {
            if (pline[i] == '.') {
                ext = YES;
                break;
            }
            if (j < 10) outfn[j++] = pline[i];
        }
        if (!ext) strcpy(pline + i, ".C");
        input = mustopen(pline, "r");
        strcpy(infn, pline);
        lineno = 0;
        if (!files && iscons(stdout)) {
            strcpy(outfn + j, ".ASM");
            output = mustopen(outfn, "w");
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
mustopen(char *fn, char *mode) {
    int fd;
    if (fd = fopen(fn, mode))
        return fd;
    fputs("open error on ", stderr);
    lout(fn, stderr);
    abort(ERRCODE);
}

