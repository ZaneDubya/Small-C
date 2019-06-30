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

/*
** miscellaneous storage
*/
int
    nogo,     /* disable goto statements? */
    noloc,    /* disable block locals? */
    opindex,  /* index to matched operator */
    opsize,   /* size of operator in characters */
    swactive, /* inside a switch? */
    swdefault,/* default label #, else 0 */
    *swnext,   /* address of next entry */
    *swend,    /* address of last entry */
    *stage,    /* staging buffer address */
    *wq,       /* while queue */
    argcs,     /* local copy of argc */
    *argvs,    /* local copy of argv */
    *wqptr,    /* ptr to next entry */
    litptr,   /* ptr to next entry */
    macptr,   /* macro buffer index */
    pptr,     /* ptr to parsing buffer */
    ch,       /* current character of input line */
    nch,      /* next character of input line */
    declared, /* # of local bytes to declare, -1 when declared */
    iflevel,  /* #if... nest level */
    skiplevel,/* level at which #if... skipping started */
    nxtlab,   /* next avail label # */
    litlab,   /* label # assigned to literal pool */
    csp,      /* compiler relative stk ptr */
    argstk,   /* function arg sp */
    argtop,   /* highest formal argument offset */
    ncmp,     /* # open compound statements */
    errflag,  /* true after 1st error in statement */
    eof,      /* true on final input eof */
    output,   /* fd for output file */
    files,    /* true if file list specified on cmd line */
    filearg,  /* cur file arg index */
    input = EOF, /* fd for input file */
    input2 = EOF, /* fd for "#include" file */
    usexpr = YES, /* true if value of expression is used */
    ccode = YES, /* true while parsing C code */
    *snext,    /* next addr in stage */
    *stail,    /* last addr of data in stage */
    *slast,    /* last addr in stage */
    listfp,   /* file pointer to list device */
    lastst,   /* last parsed statement type */
    oldseg;   /* current segment (0, DATASEG, CODESEG) */

char
    optimize, /* optimize output of staging buffer? */
    alarm,    /* audible alarm on errors? */
    monitor,  /* monitor function headers? */
    pause,    /* pause for operator on errors? */
    *symtab,   /* symbol table */
    *litq,     /* literal pool */
    *macn,     /* macro name buffer */
    *macq,     /* macro string buffer */
    *pline,    /* parsing buffer */
    *mline,    /* macro buffer */
    *line,     /* ptr to pline or mline */
    *lptr,     /* ptr to current character in "line" */
    *glbptr,   /* global symbol table */
    *locptr,   /* next local symbol table entry */
    *cptr,     /* work ptrs to any char buffer */
    *cptr2,
    *cptr3,
    msname[NAMESIZE],   /* macro symbol name */
    ssname[NAMESIZE];   /* global symbol name */

int op[16] = {   /* p-codes of signed binary operators */
    OR12,                        /* level5 */
    XOR12,                       /* level6 */
    AND12,                       /* level7 */
    EQ12,   NE12,                /* level8 */
    LE12,   GE12,  LT12,  GT12,  /* level9 */
    ASR12,  ASL12,               /* level10 */
    ADD12,  SUB12,               /* level11 */
    MUL12, DIV12, MOD12          /* level12 */
};

int op2[16] = {  /* p-codes of unsigned binary operators */
    OR12,                        /* level5 */
    XOR12,                       /* level6 */
    AND12,                       /* level7 */
    EQ12,   NE12,                /* level8 */
    LE12u,  GE12u, LT12u, GT12u, /* level9 */
    ASR12,  ASL12,               /* level10 */
    ADD12,  SUB12,               /* level11 */
    MUL12u, DIV12u, MOD12u       /* level12 */
};

/*
** execution begins here
*/
main(int argc, int *argv) {
    fputs(VERSION, stderr);
    fputs(CRIGHT1, stderr);
    argcs = argc;
    argvs = argv;
    swnext = calloc(SWTABSZ, 1);
    swend = swnext + (SWTABSZ - SWSIZ);
    stage = calloc(STAGESIZE, 2 * BPW);
    wqptr =
        wq = calloc(WQTABSZ, BPW);
    litq = calloc(LITABSZ, 1);
    macn = calloc(MACNSIZE, 1);
    macq = calloc(MACQSIZE, 1);
    pline = calloc(LINESIZE, 1);
    mline = calloc(LINESIZE, 1);
    slast = stage + (STAGESIZE * 2 * BPW);
    symtab = calloc((NUMLOCS*SYMAVG + NUMGLBS*SYMMAX), 1);
    locptr = STARTLOC;
    glbptr = STARTGLB;
    initStructs();
    getRunArgs();   /* get run options from command line arguments */
    openfile();     /* and initial input file */
    preprocess();   /* fetch first line */
    header();       /* intro code */
    setcodes();     /* initialize code pointer array */
    parse();        /* process ALL input */
    trailer();      /* follow-up code */
    fclose(output); /* explicitly close output */
    printallstructs();
}

// ****************************************************************************
// High level Parsing
// ****************************************************************************

/*
** Process all input text. At this level, only global declarations, defines,
** includes, structs, and function definitions are legal.
*/
parse() {
    while (eof == 0) {
        if (amatch("extern", 6))
            dodeclare(EXTERNAL);
        else if (amatch("static", 6)) // global static = "file scope"
            dostatic();
        else if (amatch("struct", 6))
            dostruct();
        else if (dodeclare(GLOBAL))
            ;
        else if (IsMatch("#asm"))
            doasm();
        else if (IsMatch("#include"))
            doinclude();
        else if (IsMatch("#define"))
            dodefine();
        else
            dofunction(GLOBAL);
        blanks();                 /* force eof if pending */
    }
}

/*
** do a static variable or function
*/
dostatic() {
    if (dodeclare(STATIC)) {
        ;
    }
    else {
        dofunction(STATIC);
    }
}

/*
** test for global declarations
*/
dodeclare(int class) {
    int type, typeSubPtr;
    type = dotype(&typeSubPtr);
    if (type != 0) {
        declglb(type, class, typeSubPtr);
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

/**
 * get type of declaration
 * @return type value of declaration, otherwise 0 if not a valid declaration.
 */
dotype(int *typeSubPtr) {
    if (amatch("char", 4)) {
        return TYPE_CHR;
    }
    else if (amatch("unsigned", 8)) {
        if (amatch("char", 4)) {
            return TYPE_UCHR;
        }
        else {
            amatch("int", 3);
            return TYPE_UINT;
        }
    }
    else if (amatch("int", 3)) {
        return TYPE_INT;
    }
    else if ((*typeSubPtr = getStructPtr()) != -1) {
        return TYPE_STRUCT;
    }
    return 0;
}

/*
** declare a global variable
*/
declglb(int type, int class, int typeSubPtr) {
    int id, dim;
    while (1) {
        if (endst()) 
            return;  /* do line */
        if (IsMatch("*")) { 
            id = IDENT_POINTER; 
            dim = 0;
        }
        else { 
            id = IDENT_VARIABLE; 
            dim = 1; 
        }
        if (symname(ssname) == 0) 
            illname();
        if (findglb(ssname)) 
            multidef(ssname);
        if (id == IDENT_VARIABLE) {
            if (IsMatch("(")) { 
                id = IDENT_FUNCTION; 
                Require(")"); 
            }
            else if (IsMatch("[")) { 
                id = IDENT_ARRAY; 
                dim = getArraySz(); 
            }
        }
        if (class == EXTERNAL) 
            external(ssname, type >> 2, id);
        else if (id != IDENT_FUNCTION) 
            InitSize(type >> 2, id, dim, class);
        if (id == IDENT_POINTER) {
            AddSymbol(ssname, id, type, BPW, 0, &glbptr, class);
        }
        else if (type == TYPE_STRUCT) {
            int structsize;
            structsize = getStructSize(typeSubPtr);
            AddSymbol(ssname, id, type, dim * structsize, 0, &glbptr, class);
            putint(typeSubPtr, cptr + CLASSPTR, 2);
        }
        else { // (type == TYPE_ARRAY)
            AddSymbol(ssname, id, type, dim * (type >> 2), 0, &glbptr, class);
        }
        if (IsMatch(",") == 0) 
            return;
    }
}

/*
** initialize global objects
*/
InitSize(int size, int ident, int dim, int class) {
    int savedim;
    litptr = 0;
    if (dim == 0) {
        dim = -1;         /* *... or ...[] */
    }
    savedim = dim;
    public(ident, class == GLOBAL); // don't do public if class == STATIC
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
        else EvalInitValue(size, ident, &dim);
    }
    if (savedim == -1 && dim == -1) {
        if (ident == IDENT_ARRAY) {
            error("need array size");
        }
        stowlit(0, size = BPW);
    }
    dumplits(size);
    dumpzero(size, dim);           /* only if dim > 0 */
}

/*
** evaluate one initializer
*/
EvalInitValue(int size, int ident, int *dim) {
    int value;
    if (string(&value)) {
        if (ident == IDENT_VARIABLE || size != 1) {
            error("must assign to char pointer or char array");
        }
        *dim -= (litptr - value);
        if (ident == IDENT_POINTER) {
            point();
        }
    }
    else if (IsConstExpr(&value)) {
        if (ident == IDENT_POINTER) {
            error("cannot assign to pointer");
        }
        stowlit(value, size);
        *dim -= 1;
    }
}

/*
** get required array size
*/
getArraySz() {
    int val;
    if (IsMatch("]")) 
        return 0; /* null size */
    if (IsConstExpr(&val) == 0) 
        val = 1;
    if (val < 0) {
        error("negative size illegal");
        val = -val;
    }
    Require("]");               /* force single dimension */
    return val;              /* and return size */
}

/*
** open an include file
*/
doinclude() {
    int i; char str[30];
    blanks();       /* skip over to name */
    if (*lptr == '"' || *lptr == '<')  {
        ++lptr;
    }
    i = 0;
    while (lptr[i] && lptr[i] != '"' && lptr[i] != '>' && lptr[i] != '\n') {
        str[i] = lptr[i];
        ++i;
    }
    str[i] = NULL;
    if ((input2 = fopen(str, "r")) == NULL) {
        input2 = EOF;
        error("open failure on include file");
    }
    kill();   /* make next read come from new file (if open) */
}

/*
** define a macro symbol
*/
dodefine() {
    int k;
    if (symname(msname) == 0) {
        illname();
        kill();
        return;
    }
    k = 0;
    if (FindSymbol(msname, macn, NAMESIZE + 2, MACNEND, MACNBR, 0) == 0) {
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

/*
** begin a function
**
** called from "parse" and tries to make a function
** out of the following text
*/
dofunction(int class) {
    int firstType, typeSubPtr;
    char *pGlobal;
    /*int typedargs; */           /* declared arguments have formal types */
    nogo = 0;                     /* enable goto statements */
    noloc = 0;                    /* enable block-local declarations */
    lastst = 0;                   /* no statement yet */
    litptr = 0;                   /* clear lit pool */
    litlab = getlabel();          /* label next lit pool */
    locptr = STARTLOC;            /* clear local variables */
    /* skip "void" & locate header */
    if (IsMatch("void")) {
        blanks();
    }
    if (monitor) {
        lout(line, stderr);
    }
    if (symname(ssname) == 0) {
        error("illegal function or declaration");
        errflag = 0;
        kill();                     /* invalidate line */
        return;
    }
    // If this name is already in the symbol table and is an autoext function,
    // define it instead as a global function
    if (pGlobal = findglb(ssname)) {
        if (pGlobal[CLASS] == AUTOEXT)
            pGlobal[CLASS] = class;
        else {
            // error: can't define something twice.
            multidef(ssname);
        }
    }
    else {
        AddSymbol(ssname, IDENT_FUNCTION, TYPE_INT, 0, 0, &glbptr, class);
    }
    public(IDENT_FUNCTION, class == GLOBAL); // don't do public if class == STATIC
    if (IsMatch("(") == 0)
        error("no open paren");
    if ((firstType = dotype(&typeSubPtr)) != 0) {
        doArgsTyped(firstType);
    }
    else {
        doArgsNonTyped();
    }
    gen(ENTER, 0);
    statement();
    if (lastst != STRETURN && lastst != STGOTO)
        gen(RETURN, 0);
    if (litptr) {
        toseg(DATASEG);
        gen(REFm, litlab);
        dumplits(1);               /* dump literals */
    }
}

/*
** doArgsTyped: interpret a function argument list with declared types.
** in type: the type of the first variable in the argument list.
*/
doArgsTyped(int type) {
    int id, sz, namelen, paren, typeSubPtr;
    char *ptr;
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
            if (findloc(ssname)) {
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
                else if (id == IDENT_VARIABLE && IsMatch("[]")) {
                    id = IDENT_POINTER;
                    sz = BPW;      /* size of pointer argument */
                }
                AddSymbol(ssname, id, type, sz, argstk, &locptr, AUTOMATIC);
                argstk += BPW;
            }
        }
        else {
            error("illegal argument name");
            break;
        }
        /* advance to next arg or closing paren */
        if (IsMatch(",")) {
            if ((type = dotype(&typeSubPtr)) == 0) {
                error("untyped argument declaration");
                break;
            }
        }
    }
    csp = 0;                       /* preset stack ptr */
    // incremenet argtop by one word (space for pushed BP), and reverse the
    // placement of the arguments (per the SmallC specification, see Chapter 8
    // and Fig 8-1.
    argtop = argstk + BPW;
    ptr = STARTLOC;
    while (argstk) {
        putint(argtop - getint(ptr + OFFSET, 2), ptr + OFFSET, 2);
        argstk -= BPW;            /* count down */
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

/*
** interpret a function argument list without declared types
*/
doArgsNonTyped() {
    // count args
    argstk = 0;
    while (IsMatch(")") == 0) {
        if (symname(ssname)) {
            if (findloc(ssname))
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
    csp = 0;                       /* preset stack ptr */
    argtop = argstk + BPW;         /* account for the pushed BP */
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
    return;
}

/*
** declare argument types
*/
doArgTypes(int type, int typeSubPtr) {
    int id, sz;
    char *ptr;
    int t;
    while (1) {
        if (argstk == 0)
            return;           /* no arguments */
        if (parseLocalDeclare(type, typeSubPtr, IDENT_POINTER, &id, &sz)) {
            if (ptr = findloc(ssname)) {
                ptr[IDENT] = id;
                ptr[TYPE] = type;
                putint(sz, ptr + SIZE, 2);
                putint(argtop - getint(ptr + OFFSET, 2), ptr + OFFSET, 2);
            }
            else {
                error("not an argument");
            }
        }
        argstk = argstk - BPW;            /* cnt down */
        if (endst())
            return;
        if (IsMatch(",") == 0)
            error("no comma or terminating semicolon");
    }
}

/**
 * parse next local or argument declaration
 * @param type value of type of variable, see cc.h
 * @param defArrTyp only comes into play if this is an array declare:
                           if IDENT_ARRAY, then must have array size;
 *                         if IDENT_POINTER, then must be a pointer.
 * @return 1 is name is valid, 0 + error if invalid.
 */
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
    if (hasOpenParen && IsMatch(")")) {
        // unmatch paren
    }
    if (IsMatch("(")) {
        if (!hasOpenParen || *id != IDENT_POINTER) {
            error("Is this a function pointer? Try (*...)()");
        }
        Require(")");
    }
    else if (*id == IDENT_VARIABLE && IsMatch("[")) {
        *id = defArrTyp;
        if ((*sz *= getArraySz()) == 0) {
            if (defArrTyp == IDENT_ARRAY) {
                error("need array size");
            }
            *sz = BPW;      /* size of pointer argument */
        }
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
    int type, typeSubPtr;
    if (ch == 0 && eof) return;
    else if ((type = dotype(&typeSubPtr)) != 0) {
        declareLocals(type, typeSubPtr);
        ReqSemicolon();
    }
    else {
        if (declared >= 0) {
            if (ncmp > 1) {
                nogo = declared;   /* disable goto */
            }
            gen(ADDSP, csp - declared);
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

/*
** declare local variables
*/
declareLocals(int type, int typeSubPtr) {
    int id, sz;
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
        declared += sz;
        // printf(" decloc %s sz=%u\n", ssname, sz);
        AddSymbol(ssname, id, type, sz, csp - declared, &locptr, AUTOMATIC);
        if (type == TYPE_STRUCT) {
            putint(typeSubPtr, cptr + CLASSPTR, 2);
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
    declared = 0;           /* may now declare local variables */
    ++ncmp;                 /* new level open */
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
            gen(ADDSP, savcsp);   /* delete local variable space */
        }
        cptr = savloc;          /* retain labels */
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
        locptr = savloc;        /* delete local symbols */
        declared = -1;          /* may not declare variables */
}

doif() {
    int flab1, flab2;
    GenTestAndJmp(flab1 = getlabel(), YES);  /* get expr, and branch false */
    statement();                    /* if true, do a statement */
    if (amatch("else", 4) == 0) {    /* if...else ? */
      /* simple "if"...print false label */
        gen(LABm, flab1);
        return;                       /* and exit */
    }
    flab2 = getlabel();
    if (lastst != STRETURN && lastst != STGOTO)
        gen(JMPm, flab2);
    gen(LABm, flab1);    /* print false label */
    statement();         /* and do "else" clause */
    gen(LABm, flab2);    /* print true label */
}

dowhile() {
    int wq[4];              /* allocate local queue */
    addwhile(wq);           /* add entry to queue for "break" */
    gen(LABm, wq[WQLOOP]);  /* loop label */
    GenTestAndJmp(wq[WQEXIT], YES);  /* see if true */
    statement();            /* if so, do a statement */
    gen(JMPm, wq[WQLOOP]);  /* loop to label */
    gen(LABm, wq[WQEXIT]);  /* exit label */
    delwhile();             /* delete queue entry */
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
        doexpr(NO);           /* expr 1 */
        ReqSemicolon();
    }
    gen(LABm, lab1);
    if (IsMatch(";") == 0) {
        GenTestAndJmp(wq[WQEXIT], NO); /* expr 2 */
        ReqSemicolon();
    }
    gen(JMPm, lab2);
    gen(LABm, wq[WQLOOP]);
    if (IsMatch(")") == 0) {
        doexpr(NO);           /* expr 3 */
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
    int wq[4], endlab, swact, swdef, *swnex, *swptr;
    swact = swactive;
    swdef = swdefault;
    swnex = swptr = swnext;
    addwhile(wq);
    *(wqptr + WQLOOP - WQSIZ) = 0;
    Require("(");
    doexpr(YES);                /* evaluate switch expression */
    Require(")");
    swdefault = 0;
    swactive = 1;
    gen(JMPm, endlab = getlabel());
    statement();                /* cases, etc. */
    gen(JMPm, wq[WQEXIT]);
    gen(LABm, endlab);
    gen(SWITCH, 0);             /* match cases */
    while (swptr < swnext) {
        gen(NEARm, *swptr++);
        gen(WORDn, *swptr++);    /* case value */
    }
    gen(WORDn, 0);
    if (swdefault)
        gen(JMPm, swdefault);
    gen(LABm, wq[WQEXIT]);
    delwhile();
    swnext = swnex;
    swdefault = swdef;
    swactive = swact;
}

docase() {
    if (swactive == 0) error("not in switch");
    if (swnext > swend) {
        error("too many cases");
        return;
    }
    gen(LABm, *swnext++ = getlabel());
    IsConstExpr(swnext++);
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
    if (endst() == 0)
        doexpr(YES);
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
    ccode = 0;           /* mark mode as "asm" */
    while (1) {
        inline();
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
    int const, val;
    int *before, *start;
    usexpr = use;        /* tell isfree() whether expr value is used */
    while (1) {
        setstage(&before, &start);
        ParseExpression(&const, &val);
        ClearStage(before, start);
        if (ch != ',')
            break;
        bump(1);
    }
    usexpr = YES;        /* return to normal value */
}

// ****************************************************************************
// miscellaneous functions
// ****************************************************************************

/*
** get run options
*/
getRunArgs() {
    int i;
    i = listfp = nxtlab = 0;
    output = stdout;
    optimize = YES;
    alarm = monitor = pause = NO;
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
        }
        fputs("usage: cc [file]... [-m] [-a] [-p] [-l#] [-no]\n", stderr);
        abort(ERRCODE);
    }
}

/*
** input and output file opens
*/
openfile() {        /* entire function revised */
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

/*
** open a file with error checking
*/
mustopen(char *fn, char *mode) {
    int fd;
    if (fd = fopen(fn, mode))
        return fd;
    fputs("open error on ", stderr);
    lout(fn, stderr);
    abort(ERRCODE);
}

