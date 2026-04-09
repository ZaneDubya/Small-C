// ============================================================================
// Small-C Compiler -- Part 3 -- Expression Analyzer.
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
#include "ccstruct.h"
#include "ccenum.h"

// Starting with level1(), this code places information about the expression
// under analysis into the is[] and is2[] arrays. These are local arrays of
// seven integers each. The term 'is' in these array names is not an acronym,
// but the word. These arrays tell the analyzer what the entire expression or
// any part of it is. In other words, they carry the properties of whatever has
// been parsed at every point in the process.
// This first instance of the array is for the left leg of the descent through
// all of the levels. Another (is2[]) is declared--for use with the right
// operand--at each level where a binary operator is recognized. Before control
// rises to a higher level, the left array is adjusted to reflect the result of
// the operation, and the right, being local and having served its purpose, is
// discarded. At lower levels, the right array appears as a left array, and
// other right arrays are declared as other binary operators are encountered.


extern char *litq, *glbptr, *locptr, *lptr, *dimdata, *dimdatptr,
    ssname[NAMESIZE], *paramTypes;
extern int ch, csp, litlab, litptr, nch, op[16], op2[16], opd[16], opd2[16],
    opindex, opsize, *snext, *slast, *argbuf, argbufcur, argtop, rettype, rettypeSubPtr,
    lastNdim, lastStrides[MAX_DIMS];

// forward declarations for this file:
void CalcCmp32(int lhi, int llo, int oper, int rhi, int rlo, int *reshi, int *reslo);
void CalcConst32(int lhi, int llo, int oper, int rhi, int rlo, int *reshi, int *reslo);
void CalcUConst32(int lhi, int llo, int oper, int rhi, int rlo, int *reshi, int *reslo);
int CalcUConst(unsigned left, int oper, unsigned right);
int CalcConst(int left, int oper, int right);
int chrcon(int *lo, int *hi);
int constant(int is[]);
int down(char *opstr, int opoff, int (*level)(), int is[]);
int down1(int (*level)(), int is[]);
void down2(int oper, int oper2, int (*level)(), int is[], int is2[]);
void dropout(int k, int tcode, int exit1, int is[]);
void error(char msg[]);
void experr();
void GenJmpIfZero(int oper, int label, int is[]);
int level14(int *is);
int litchar();
void negate32(int *hi, int *lo);
int number(int *lo, int *hi);
int skim(char *opstr, int tcode, int dropval, int endval, int (*level)(), int is[]);
void step(int oper, int is[], int oper2);
void store(int is[]);
int structmember(char *ptr, int *is);
void widen_primary(int is[]);
int tableFind(int tab[], int key);

// ****************************************************************************
// lead-in functions
// ****************************************************************************

int IsConstExpr(int *val) {
    int isConst;
    int *before, *start;
    setstage(&before, &start);
    ParseExpression(&isConst, val);
    ClearStage(before, 0);     // scratch generated code
    if (isConst == 0) {
        error("must be constant expression");
    }
    return isConst;
}

void ParseExpression(int *con, int *val) {
    int is[ISSIZE], savedlocptr;
    savedlocptr = locptr;
    if (level1(is)) {
        fetch(is);
    }
    *con = is[TYP_CNST];
    *val = is[VAL_CNST];
    locptr = savedlocptr;
}

void ParseExpr32(int *con, int *val, int *val_hi) {
    int is[ISSIZE], savedlocptr;
    savedlocptr = locptr;
    if (level1(is)) fetch(is);
    *con = is[TYP_CNST];
    *val = is[VAL_CNST];
    *val_hi = is[VAL_CNST_HI];
    locptr = savedlocptr;
}

int IsCExpr32(int *val, int *val_hi) {
    int isConst;
    int *before, *start;
    setstage(&before, &start);
    ParseExpr32(&isConst, val, val_hi);
    ClearStage(before, 0);
    if (isConst == 0)
        error("must be constant expression");
    return isConst;
}

void GenTestAndJmp(int label, int reqParens) {
    int is[ISSIZE];
    int *before, *start;
    if (reqParens) {
        Require("(");
    }
    while (1) {
        setstage(&before, &start);
        if (level1(is)) {
            fetch(is);
        }
        if (IsMatch(",")) {
            ClearStage(before, start);
        }
        else {
            break;
        }
    }
    if (reqParens) {
        Require(")");
    }
    if (is[TYP_CNST]) {             // constant expression
        ClearStage(before, 0);
        if (is[VAL_CNST] || is[VAL_CNST_HI]) {
            return;
        }
        gen(JMPm, label);
        return;
    }
    if (is[STG_ADR]) {             // stage address of "oper 0" code
        switch (is[LAST_OP]) {       // operator code
            case EQ12:
            case LE12u: 
                GenJmpIfZero(EQ10f, label, is); 
                break;
            case NE12:
            case GT12u: 
                GenJmpIfZero(NE10f, label, is); 
                break;
            case GT12:  
                GenJmpIfZero(GT10f, label, is); 
                break;
            case GE12:  
                GenJmpIfZero(GE10f, label, is); 
                break;
            case GE12u: 
                ClearStage(is[STG_ADR], 0);      
                break;
            case LT12:  
                GenJmpIfZero(LT10f, label, is); 
                break;
            case LT12u: 
                GenJmpIfZero(JMPm, label, is); 
                break;
            case LE12:  
                GenJmpIfZero(LE10f, label, is); 
                break;
            default:    
                if (isLongVal(is))
                    gen(NEd10f, label);
                else
                    gen(NE10f, label);          
                break;
        }
    }
    else {
        if (isLongVal(is))
            gen(NEd10f, label);
        else
            gen(NE10f, label);
    }
    ClearStage(before, start);
}

// test primary register against zero and generate a jump instruction if false
void GenJmpIfZero(int oper, int label, int is[]) {
    ClearStage(is[STG_ADR], 0);       // purge conventional code
    gen(oper, label);
}

// ============================================================================
// Expression Analysis
// Functions level1() through level14() correspond to the precedence levels of
// the expression operators, such that level1() recognizes the operators at the
// lowest precedence level and level14() the highest.
// ============================================================================

// level1() recognizes and generates code for the assignment operators: 
// =, |=, ^=, &=, +=, -=, *=, /=, %=, >>=, and <<=.
// Since these operators group from right to left, the assignments must be made
// in the reverse order from which the expression is scanned. Therefore, after
// parsing the left subexpression, if one of these operators is recognized and
// if the parsed subexpression produced an lvalue, then level1() calls itself
// again. On parsing the second subexpression, the cycle repeats and continues
// to repeat until the series of assignments ends. At that point, store() is
// called to generate code for the right most assignment, then control returns
// to the previous instance of level1(). Assignment code is again generated and
// control again returns to the prior instance. This continues until the
// initial instance of level1() returns to one of the lead-in functions or to
// primary() or level14() (by way of down1() and down2() ).
int level1(int is[]) {
    int k, is2[ISSIZE], is3[2], oper, oper2;
    char *lhsPtr, *rhsPtr;
    int isStructLhs, savedcsp, structSize;
    char *cpyfn;
    k = down1(level2, is);
    if (is[TYP_CNST]) {
        gen(GETw1n, is[VAL_CNST]);
        if (is[TYP_CNST] >> 2 == BPD)
            gen(GETdxn, is[VAL_CNST_HI]);
    }
    // check if this is an assignment operator, requiring an lvalue.
    if (IsMatch("|=")) {
        oper = oper2 = OR12;
    }
    else if (IsMatch("^=")) {
        oper = oper2 = XOR12;
    }
    else if (IsMatch("&=")) { 
        oper = oper2 = AND12; 
    }
    else if (IsMatch("+=")) { 
        oper = oper2 = ADD12;
    }
    else if (IsMatch("-=")) {
        oper = oper2 = SUB12;
    }
    else if (IsMatch("*=")) { 
        oper = MUL12; oper2 = MUL12u;
    }
    else if (IsMatch("/=")) { 
        oper = DIV12; oper2 = DIV12u;
    }
    else if (IsMatch("%=")) { 
        oper = MOD12; oper2 = MOD12u;
    }
    else if (IsMatch(">>=")) {
        oper = ASR12; oper2 = LSR12;
    }
    else if (IsMatch("<<=")) {
        oper = oper2 = ASL12;
    }
    else if (IsMatch("=")) {
        oper = oper2 = 0;
    }
    else {
        return k;
    }
    if (k == 0) {
        error("lvalue cannot be assigned");
        return 0;
    }
    // Reject assignment to a const-qualified variable.
    lhsPtr = is[SYMTAB_ADR];
    if (lhsPtr && lhsPtr[CLASS] & CONST_FLAG) {
        error("cannot assign to const variable");
        return 0;
    }
    is3[SYMTAB_ADR] = is[SYMTAB_ADR];
    is3[TYP_OBJ] = is[TYP_OBJ];
    // Struct-to-struct deep copy
    if (oper == 0) {
        lhsPtr = is[SYMTAB_ADR];
        isStructLhs = 0;
        if (is[TYP_OBJ] == TYPE_STRUCT)
            isStructLhs = 1;
        else if (is[TYP_OBJ] == 0 && lhsPtr && lhsPtr[TYPE] == TYPE_STRUCT
                 && lhsPtr[IDENT] == IDENT_VARIABLE)
            isStructLhs = 1;
        if (isStructLhs) {
            savedcsp = csp;
            structSize = getStructSize(getClsPtr(lhsPtr));
            if (is[TYP_OBJ] == TYPE_STRUCT) {
                // LHS is a computed address in AX (member access, etc).
                // Must save before parsing RHS.
                gen(PUSH1, 0);
                k = level1(is2);
                if (k && is2[TYP_OBJ] == TYPE_STRUCT) {
                    // AX = src from struct-returning function
                } else if (k && is2[TYP_OBJ] == 0
                           && (rhsPtr = is2[SYMTAB_ADR])
                           && rhsPtr[TYPE] == TYPE_STRUCT) {
                    gen(POINT1m, rhsPtr);
                } else {
                    error("type mismatch in struct assignment");
                    gen(ADDSP, savedcsp);
                    return 0;
                }
                // AX = &src.  Build structcp(dst, src, n) args R-to-L:
                // push n first ([BP+8]), &src second ([BP+6]),
                // &dst last ([BP+4]).
                gen(MOVE21, 0);                  // BX = &src
                gen(GETw1n, structSize);         // AX = n
                gen(PUSH1, 0);                   // push n
                gen(SWAP12, 0);                  // AX = &src (from BX)
                gen(PUSH1, 0);                   // push &src
                gen(GETw1s, savedcsp - BPW);     // AX = &dst (from saved slot)
                gen(PUSH1, 0);                   // push &dst (last [BP+4])
            }
            else {
                // LHS from symbol table -- parse RHS first so that
                // callfunc's return buffer doesn't separate &dst from
                // the structcp arguments.
                k = level1(is2);
                if (k && is2[TYP_OBJ] == TYPE_STRUCT) {
                    // AX = src from struct-returning function
                } else if (k && is2[TYP_OBJ] == 0
                           && (rhsPtr = is2[SYMTAB_ADR])
                           && rhsPtr[TYPE] == TYPE_STRUCT) {
                    gen(POINT1m, rhsPtr);
                } else {
                    error("type mismatch in struct assignment");
                    gen(ADDSP, savedcsp);
                    return 0;
                }
                // AX = &src.  Build structcp(dst, src, n) args R-to-L:
                // push n first ([BP+8]), &src second ([BP+6]),
                // &dst last ([BP+4]).
                gen(MOVE21, 0);                  // BX = &src
                gen(GETw1n, structSize);         // AX = n
                gen(PUSH1, 0);                   // push n
                gen(SWAP12, 0);                  // AX = &src (from BX)
                gen(PUSH1, 0);                   // push &src
                gen(POINT1m, lhsPtr);            // AX = &dst (global address)
                gen(PUSH1, 0);                   // push &dst (last [BP+4])
            }
            cpyfn = findglb("structcp");
            if (cpyfn == 0) {
                cpyfn = AddSymbol("structcp", IDENT_FUNCTION, TYPE_INT,
                    0, 0, &glbptr, AUTOEXT);
            } else if ((cpyfn[CLASS] & 0x7F) == PROTOEXT) {
                cpyfn[CLASS] = AUTOEXT;  // ensure trailer() emits EXTRN
            }
            gen(CALLm, cpyfn);
            gen(ADDSP, savedcsp);
            return 0;
        }
    }
    if (is[TYP_OBJ]) {                          // indirect target
        if (oper) {                             // ?=
            gen(PUSH1, 0);                      // save address (always 16-bit)
            fetch(is);                          // fetch left side
            down2(oper, oper2, level1, is, is2);// parse right side
            gen(POP2, 0);                       // retrieve address
        }
        else {                                  //  =
            gen(PUSH1, 0);                      // save address (always 16-bit)
            if (level1(is2))
                fetch(is2);
            // widen RHS if LHS is long and RHS is not
            if (is3[TYP_OBJ] >> 2 == BPD && !isLongVal(is2))
                widen_primary(is2);
            gen(POP2, 0);                       // retrieve address
        }
    }
    else {                                      // direct target
        if (oper) {                             // ?=
            fetch(is);                          // fetch left side
            down2(oper, oper2, level1, is, is2);// parse right side
        }
        else {                                  //  =
            if (level1(is2))
                fetch(is2);                     // parse right side
            // widen RHS if LHS is long and RHS is not
            if (isLongVal(is3) && !isLongVal(is2))
                widen_primary(is2);
        }
    }
    store(is3);                                 // store result
    return 0;
}

// level2() parses the ternary operator Expression1 ? Expression2 : Expression3
int level2(int is1[]) {
    int is2[ISSIZE], is3[ISSIZE], k, flab, endlab;
    // Call level3() by way of down1() to parse the first expression.
    k = down1(level3, is1);
    // If next token is not ?, this is not a ternary operator, return to caller
    if (IsMatch("?") == 0) {
        return k;
    }
    // getlabel() is called to reserve a label number for use in jumping around
    // Expression2, and dropout() is called to generate code to perform that
    // jump if Expression1 is false.
    dropout(k, NE10f, flab = getlabel(), is1);
    // Expression2 is parsed by recursively calling level2() through down1().
    if (down1(level2, is2)) {
        fetch(is2); // fetch() obtains the expression's value from memory.
    }
    else if (is2[TYP_CNST]) {
        gen(GETw1n, is2[VAL_CNST]);
        if (is2[TYP_CNST] >> 2 == BPD)
            gen(GETdxn, is2[VAL_CNST_HI]);
    }
    // Enforce second character of operator (":")
    Require(":");
    gen(JMPm, endlab = getlabel());
    gen(LABm, flab);
    if (down1(level2, is3)) {
        fetch(is3);        // expression 3
    }
    else if (is3[TYP_CNST]) {
        gen(GETw1n, is3[VAL_CNST]);
        if (is3[TYP_CNST] >> 2 == BPD)
            gen(GETdxn, is3[VAL_CNST_HI]);
    }
    gen(LABm, endlab);
    is1[TYP_CNST] = is1[VAL_CNST] = 0;
    if (is2[TYP_CNST] && is3[TYP_CNST]) {                  // expr1 ? const2 : const3
        is1[TYP_ADR] = is1[TYP_OBJ] = is1[STG_ADR] = 0;
    }
    else if (is3[TYP_CNST]) {                        // expr1 ? var2 : const3
        is1[TYP_ADR] = is2[TYP_ADR];
        is1[TYP_OBJ] = is2[TYP_OBJ];
        is1[STG_ADR] = is2[STG_ADR];
    }
    else if ((is2[TYP_CNST])                         // expr1 ? const2 : var3
        || (is2[TYP_ADR] == is3[TYP_ADR])) {           // expr1 ? same2 : same3
        is1[TYP_ADR] = is3[TYP_ADR];
        is1[TYP_OBJ] = is3[TYP_OBJ];
        is1[STG_ADR] = is3[STG_ADR];
    }
    else
        error("mismatched expressions");
    return 0;
}

// level3() through level12() are essentially alike and very simple. Each one
// descends to the next lower level in the parsing hierarchy by way of either
// skim() or down(). In so doing, it passes a string of the operators to be
// recognized at the current level, the address of the target level function,
// the address of the is[] array that it received, and various other arguments.
// Basically, these function only serve to direct the flow of control through
// the central part of the expression analyzer. In so doing, they enforce the
// operator precedence rules. We have already seen the function that calls
// them (down1()) and the ones which they call (skim() and down()).

int level3(int is[]) {
    return skim("||", EQ10f, 1, 0, level4, is); 
}

int level4(int is[]) {
    return skim("&&", NE10f, 0, 1, level5, is); 
}

int level5(int is[]) {
    return down("|", 0, level6, is); 
}

int level6(int is[]) {
    return down("^", 1, level7, is); 
}

int level7(int is[]) {
    return down("&", 2, level8, is); 
}

int level8(int is[]) {
    return down("== !=", 3, level9, is); 
}

int level9(int is[]) {
    return down("<= >= < >", 5, level10, is);
}

int level10(int is[]) {
    return down(">> <<", 9, level11, is); 
}

int level11(int is[]) {
    return down("+ -", 11, level12, is); 
}

int level12(int is[]) {
    return down("* / %", 13, level13, is); 
}

// level13() parses the unary operators: ++, --, ~, !, -, *, &, and sizeof().
int level13(int is[]) {
    int k;
    char *ptr;
    if (IsMatch("++")) {                 // ++lval
        if (level13(is) == 0) {
            needlval();
            return 0;
        }
        ptr = is[SYMTAB_ADR];
        if (ptr && ptr[CLASS] & CONST_FLAG) {
            error("cannot modify const variable");
            return 0;
        }
        step(rINC1, is, 0);
        return 0;
    }
    else if (IsMatch("--")) {            // --lval
        if (level13(is) == 0) {
            needlval();
            return 0;
        }
        ptr = is[SYMTAB_ADR];
        if (ptr && ptr[CLASS] & CONST_FLAG) {
            error("cannot modify const variable");
            return 0;
        }
        step(rDEC1, is, 0);
        return 0;
    }
    else if (IsMatch("~")) {             // ~
        if (level13(is)) fetch(is);
        if (isLongVal(is))
            gen(COMd1, 0);
        else
            gen(COM1, 0);
        is[VAL_CNST] = ~is[VAL_CNST];
        if (is[TYP_CNST] >> 2 == BPD)
            is[VAL_CNST_HI] = ~is[VAL_CNST_HI];
        return (is[STG_ADR] = 0);
    }
    else if (IsMatch("!")) {             // !
        if (level13(is)) fetch(is);
        if (isLongVal(is))
            gen(LNEGd1, 0);
        else
            gen(LNEG1, 0);
        // result is always 16-bit 0/1
        is[TYP_OBJ] = 0;
        is[SYMTAB_ADR] = 0;
        if (is[TYP_CNST]) {
            int wasTrue;
            wasTrue = is[VAL_CNST] || is[VAL_CNST_HI];
            is[TYP_CNST] = TYPE_INT;
            is[VAL_CNST] = !wasTrue;
            is[VAL_CNST_HI] = 0;
        }
        return (is[STG_ADR] = 0);
    }
    else if (IsMatch("-")) {             // unary -
        if (level13(is)) fetch(is);
        if (isLongVal(is))
            gen(ANEGd1, 0);
        else
            gen(ANEG1, 0);
        if (is[TYP_CNST] >> 2 == BPD) {
            negate32(&is[VAL_CNST_HI], &is[VAL_CNST]);
        }
        else {
            is[VAL_CNST] = -is[VAL_CNST];
        }
        return (is[STG_ADR] = 0);
    }
    else if (IsMatch("*")) {             // unary *
        if (level13(is)) fetch(is);
        if (is[TYP_ADR]) {
            // Pointer cast in effect: use the cast's target type directly.
            // This lets *(unsigned char *)voidptr and similar work correctly.
            is[TYP_OBJ] = is[TYP_ADR];
        }
        else if (ptr = is[SYMTAB_ADR]) {
            if (ptr[IDENT] == IDENT_PTR_ARRAY)
                is[TYP_OBJ] = TYPE_UINT;
            else
                is[TYP_OBJ] = ptr[TYPE];
        }
        else
            is[TYP_OBJ] = TYPE_INT;
        if (is[TYP_OBJ] == TYPE_VOID) {
            error("cannot dereference void pointer");
            is[TYP_OBJ] = TYPE_INT;   // error recovery
        }
        is[STG_ADR] = is[TYP_ADR] = is[TYP_CNST] = 0; // no op0 stage addr, not addr or const
        is[VAL_CNST] = 1;                   // omit fetch() on func call
        return 1;
    }
    else if (amatch("sizeof", 6)) {    // sizeof()
        int sz, p;  char *ptr, sname[NAMESIZE];
        if (IsMatch("(")) {
            p = 1;
        }
        else {
            p = 0;
        }
        sz = 0;
        // Consume qualifiers that do not affect object size.
        if (amatch("const", 5))    ;
        if (amatch("volatile", 8)) ;
        if (amatch("signed", 6))   { sz = BPW; }  // signed alone == int (2 bytes)
        if (amatch("unsigned", 8)) {
            sz = BPW;
            if (amatch("long", 4)) {
                sz = BPD;
                amatch("int", 3);
            }
        }
        // short == int on 8086 (both 16-bit)
        if (amatch("short", 5)) {
            amatch("int", 3);
            if (sz == 0) sz = BPW;  // unsigned short still BPW
        }
        else if (amatch("long", 4)) {
            sz = BPD;
            amatch("int", 3);
        }
        else if (amatch("int", 3)) {
            if (sz == 0) sz = BPW;
        }
        else if (amatch("char", 4)) {
            sz = 1;
        }
        if (sz) { 
            if (IsMatch("*")) {      
                sz = BPW; 
            }
        }
        else if (amatch("void", 4)) {
            // sizeof(void*) == BPW (2); sizeof(void) == 0
            sz = IsMatch("*") ? BPW : 0;
        }
        else if (amatch("struct", 6)) {
            char *sp;
            sp = getTagPtr(KIND_STRUCT);
            if (sp == -1) error("unknown struct name");
            else sz = getStructSize(sp);
        }
        else if (amatch("union", 5)) {
            char *sp;
            sp = getTagPtr(KIND_UNION);
            if (sp == -1) error("unknown union name");
            else sz = getStructSize(sp);
        }
        else if (amatch("enum", 4)) {
            symname(sname);    // consume optional tag name
            sz = BPW;          // sizeof(enum X) == sizeof(int) == 2
        }
        else if (symname(sname) &&  
            ((ptr = findloc(sname)) || (ptr = findglb(sname))) &&
            ptr[IDENT] != IDENT_FUNCTION && ptr[IDENT] != IDENT_PTR_FUNCTION
            && ptr[IDENT] != IDENT_LABEL) {
            if ((ptr[CLASS] & 0x7F) == TYPEDEF) {
                // typedef name: derive size from type, not SIZE field (which is 0)
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
        else if (sz == 0) {
            error("must be object or type");
        }
        if (p) {
            Require(")");
        }
        is[TYP_CNST] = TYPE_INT;
        is[VAL_CNST] = sz;
        is[TYP_ADR] = is[TYP_OBJ] = is[SYMTAB_ADR] = 0;
        return 0;
    }
    else if (IsMatch("&")) {             // unary &
        if (level13(is) == 0) {
            error("illegal address");
            return 0;
        }
        ptr = is[SYMTAB_ADR];
        is[TYP_ADR] = ptr[TYPE];
        if (is[TYP_OBJ]) {
            // AX already has the address (POINT1s emitted by primary() for
            // local variables). The result of & is always a 16-bit pointer,
            // so clear TYP_OBJ — otherwise isLongVal() treats &(long) as a
            // long value and emits PUSHd1 (two words) instead of PUSH1 (one).
            is[TYP_OBJ] = 0;
            return 0;
        }
        gen(POINT1m, ptr);
        // TYP_OBJ stays 0: same reasoning as above.
        return 0;
    }
    else {
        k = level14(is);
        if (IsMatch("++")) {               // lval++
            if (k == 0) {
                needlval();
                return 0;
            }
            ptr = is[SYMTAB_ADR];
            if (ptr && ptr[CLASS] & CONST_FLAG) {
                error("cannot modify const variable");
                return 0;
            }
            step(rINC1, is, rDEC1);
            return 0;
        }
        else if (IsMatch("--")) {          // lval--
            if (k == 0) {
                needlval();
                return 0;
            }
            ptr = is[SYMTAB_ADR];
            if (ptr && ptr[CLASS] & CONST_FLAG) {
                error("cannot modify const variable");
                return 0;
            }
            step(rDEC1, is, rINC1);
            return 0;
        }
        else {
            return k;
        }
    }
}

// Level14() is the last (highest) precedence level before primary(). It
// recognizes only two operations, subscripting and calling functions. It also
// handles the case where a function's address is invoked by naming the
// function without a left parenthesis following.
int level14(int *is) {
    int k, val, elemType;
    char *ptr, *before, *start;
    int is2[ISSIZE];
    k = primary(is);
    ptr = is[SYMTAB_ADR];
    blanks();
    while (1) {
        if (IsMatch(".")) {
            // struct member access via dot operator
            if (ptr == 0 || ptr[TYPE] != TYPE_STRUCT) {
                error("dot requires struct");
                return 0;
            }
            // Auto-dereference struct pointers (hidden pointer params, etc.)
            if (ptr[IDENT] == IDENT_POINTER && is[TYP_OBJ] != TYPE_STRUCT) {
                if (k) fetch(is);
            }
            // Get struct base address into AX.
            // Locals: POINT1s was already emitted by primary(); AX = &struct.
            // Globals: primary() deferred; must emit now.
            else if (is[TYP_OBJ] == 0) {
                gen(POINT1m, ptr);
            }
            k = structmember(ptr, is);
        }
        else if (IsMatch("->")) {
            // struct member access via arrow operator
            if (ptr == 0 || ptr[TYPE] != TYPE_STRUCT) {
                error("-> requires pointer to struct");
                return 0;
            }
            if (ptr[IDENT] == IDENT_VARIABLE) {
                error("use . for struct variables");
                return 0;
            }
            // Load pointer value to get struct address into AX.
            if (k) fetch(is);
            k = structmember(ptr, is);
        }
        else if (IsMatch("[")) {
            // [subscript]
            if (ptr == 0) {
                error("can't subscript");
                skipToNextToken();
                Require("]");
                return 0;
            }
            if (is[TYP_ADR]) { 
                if (k) {
                    fetch(is); 
                }
            }
            else { 
                error("can't subscript"); 
                k = 0; 
            }
            setstage(&before, &start);
            is2[TYP_CNST] = 0;
            down2(0, 0, level1, is2, is2);
            Require("]");
            if (is[DIM_LEFT] > 1) {
                // multi-dim: use stride for this dimension
                int stride;
                stride = getDimStride(ptr,
                    ptr[NDIM] - is[DIM_LEFT]);
                if (is2[TYP_CNST]) {
                    ClearStage(before, 0);
                    if (is2[VAL_CNST]) {
                        gen(GETw2n,
                            is2[VAL_CNST] * stride);
                        gen(ADD12, 0);
                    }
                }
                else {
                    gen(PUSH2, 0);
                    gen(GETw2n, stride);
                    gen(MUL12, 0);
                    gen(POP2, 0);
                    gen(ADD12, 0);
                }
                is[DIM_LEFT]--;
                is[TYP_ADR] = ptr[TYPE];
                k = 0;
            }
            else {
                // final dimension or 1D: element-size scaling
                if (ptr && ptr[IDENT] == IDENT_PTR_ARRAY) {
                    // pointer array: elements are word-sized pointers
                    if (is2[TYP_CNST]) {
                        ClearStage(before, 0);
                        if (is2[VAL_CNST]) {
                            gen(GETw2n,
                                is2[VAL_CNST] << LBPW);
                            gen(ADD12, 0);
                        }
                    }
                    else {
                        gen(DBL1, 0);
                        gen(ADD12, 0);
                    }
                    is[DIM_LEFT] = 0;
                    is[TYP_OBJ] = TYPE_UINT;
                    is[TYP_ADR] = ptr[TYPE];
                    k = 1;
                }
                else {
                    // Use cast type if present, else the symbol's declared type.
                    elemType = is[TYP_ADR] ? is[TYP_ADR] : ptr[TYPE];
                    if (is2[TYP_CNST]) {
                        ClearStage(before, 0);
                        if (is2[VAL_CNST]) {
                            if (elemType == TYPE_STRUCT) {
                                gen(GETw2n, is2[VAL_CNST]
                                    * getStructSize(
                                        getClsPtr(ptr)));
                            }
                            else if (elemType >> 2 == BPD) {
                                gen(GETw2n,
                                    is2[VAL_CNST] << LBPD);
                            }
                            else if (elemType >> 2 == BPW) {
                                gen(GETw2n,
                                    is2[VAL_CNST] << LBPW);
                            }
                            else {
                                gen(GETw2n, is2[VAL_CNST]);
                            }
                            gen(ADD12, 0);
                        }
                    }
                    else {
                        if (elemType == TYPE_STRUCT) {
                            gen(PUSH2, 0);
                            gen(GETw2n, getStructSize(
                                getClsPtr(ptr)));
                            gen(MUL12, 0);
                            gen(POP2, 0);
                        }
                        else if (elemType >> 2 == BPD) {
                            gen(DBL1, 0);
                            gen(DBL1, 0);
                        }
                        else if (elemType >> 2 == BPW) {
                            gen(DBL1, 0);
                        }
                        gen(ADD12, 0);
                    }
                    is[DIM_LEFT] = 0;
                    is[TYP_ADR] = 0;
                    is[TYP_OBJ] = elemType;
                    k = 1;
                }
            }
        }
        else if (IsMatch("(")) {         // function(...)
            if (ptr == 0) {
                callfunc(0);
            }
            else if (ptr[IDENT] != IDENT_FUNCTION && ptr[IDENT] != IDENT_PTR_FUNCTION) {
                if (k && !is[VAL_CNST]) {
                    fetch(is);
                }
                callfunc(0);
            }
            else {
                callfunc(ptr);
            }
            if (ptr && ptr[IDENT] == IDENT_FUNCTION
                    && ptr[TYPE] == TYPE_STRUCT) {
                is[TYP_OBJ] = TYPE_STRUCT;
                is[SYMTAB_ADR] = AddSymbol("tmpRetStr",
                    IDENT_VARIABLE, TYPE_STRUCT,
                    getStructSize(getClsPtr(ptr)),
                    0, &locptr, AUTOMATIC);
                putint(getClsPtr(ptr),
                       is[SYMTAB_ADR] + CLASSPTR, 2);
                is[TYP_ADR] = is[TYP_CNST] = is[VAL_CNST] = 0;
                is[LAST_OP] = is[STG_ADR] = 0;
                k = 1;
            }
            else if (ptr && ptr[IDENT] == IDENT_PTR_FUNCTION) {
                // pointer-returning function: propagate pointer type to caller
                is[TYP_ADR]    = ptr[TYPE];
                is[TYP_OBJ]    = 0;
                is[TYP_CNST]   = is[VAL_CNST] = is[VAL_CNST_HI] = 0;
                is[LAST_OP]    = is[STG_ADR]  = 0;
                is[SYMTAB_ADR] = 0;
                k = 0;
            }
            else {
                k = is[SYMTAB_ADR] = is[TYP_CNST] = is[VAL_CNST] = 0;
                is[VAL_CNST_HI] = is[TYP_ADR] = is[LAST_OP] = is[STG_ADR] = 0;
                if (ptr && (ptr[TYPE] == TYPE_LONG || ptr[TYPE] == TYPE_ULONG))
                    is[TYP_OBJ] = ptr[TYPE];
                else
                    is[TYP_OBJ] = 0;
            }
        }
        else {
            break;
        }
        ptr = is[SYMTAB_ADR];
    }
    if (ptr && (ptr[IDENT] == IDENT_FUNCTION || ptr[IDENT] == IDENT_PTR_FUNCTION)) {
        gen(POINT1m, ptr);
        is[SYMTAB_ADR] = 0;
        return 0;
    }
    return k;
}

// Handle struct member lookup and address arithmetic for . and -> operators.
// Assumes AX already contains the struct base address.
// Returns 1 (result is an lvalue).
int structmember(char *ptr, int *is) {
    char memName[NAMESIZE], *member;
    int offset;
    if (!symname(memName)) {
        error("expected member name");
        return 0;
    }
    member = getStructMember(getClsPtr(ptr), memName);
    if (member == 0) {
        error("not a member of this struct");
        return 0;
    }
    offset = getint(member + STRMEM_OFFSET, 2);
    if (offset != 0) {
        gen(GETw2n, offset);
        gen(ADD12, 0);
    }
    is[SYMTAB_ADR] = AddSymbol("tmpStrMem",
        member[STRMEM_IDENT], member[STRMEM_TYPE],
        getint(member + STRMEM_SIZE, 2),
        getint(member + STRMEM_OFFSET, 2),
        &locptr, AUTOMATIC);
    is[TYP_OBJ] = member[STRMEM_TYPE];
    is[TYP_ADR] = 0;
    is[TYP_CNST] = is[VAL_CNST] = is[LAST_OP] = is[STG_ADR] = 0;
    // For nested structs, propagate struct definition pointer
    if (member[STRMEM_TYPE] == TYPE_STRUCT) {
        putint(getint(member + STRMEM_CLASSPTR, 2),
               is[SYMTAB_ADR] + CLASSPTR, 2);
    }
    // For array members, set TYP_ADR so subscripting works
    if (member[STRMEM_IDENT] == IDENT_ARRAY) {
        is[TYP_ADR] = member[STRMEM_TYPE];
        return 0;   // array name is not an lvalue
    }
    // For pointer members, set TYP_OBJ to address-sized, TYP_ADR to pointed-to type
    if (member[STRMEM_IDENT] == IDENT_POINTER) {
        is[TYP_OBJ] = TYPE_UINT;
        is[TYP_ADR] = member[STRMEM_TYPE];
    }
    return 1;
}

// Apply a type cast to the expression in is[].
// Called after the operand has been parsed and fetched.
// casttype: target TYPE_xxx constant.
void applycast(int casttype, int is[]) {
    int srcLong, dstLong, dstSize;
    dstSize = casttype >> 2;
    srcLong = isLongVal(is);
    dstLong = (dstSize == BPD);

    // Constant folding path
    if (is[TYP_CNST]) {
        if (dstLong && !(is[TYP_CNST] >> 2 == BPD)) {
            // Widen constant to 32-bit
            if (!(is[TYP_CNST] & IS_UNSIGNED)
                && (is[VAL_CNST] & 0x8000))
                is[VAL_CNST_HI] = -1;
            else
                is[VAL_CNST_HI] = 0;
            gen(GETdxn, is[VAL_CNST_HI]);
        }
        else if (!dstLong
            && is[TYP_CNST] >> 2 == BPD) {
            // Narrow from 32-bit
            is[VAL_CNST_HI] = 0;
            if (dstSize == 1) {
                is[VAL_CNST] = is[VAL_CNST] & 255;
                if (!(casttype & IS_UNSIGNED) && (is[VAL_CNST] & 0x80))
                    is[VAL_CNST] = is[VAL_CNST] | ~255;
            }
        }
        else if (dstSize == 1) {
            is[VAL_CNST] = is[VAL_CNST] & 255;
            // Sign-extend if target is signed char: e.g. (char)-1 == -1, not 255
            if (!(casttype & IS_UNSIGNED) && (is[VAL_CNST] & 0x80))
                is[VAL_CNST] = is[VAL_CNST] | ~255;
            is[VAL_CNST_HI] = 0;
        }
        is[TYP_CNST] = casttype;
        return;
    }

    // Runtime path
    if (dstLong && !srcLong) {
#ifdef ENABLE_WARNINGS
        // Warn if widening signed value into unsigned long.
        if ((casttype & IS_UNSIGNED) && !IsUnsigned(is)) {
            warning("sign-conversion: widening signed value to unsigned long");
        }
#endif
        widen_primary(is);
    }
    // Narrowing and same-size: no codegen needed

    // Update expression descriptor
    is[TYP_CNST] = 0;
    is[SYMTAB_ADR] = 0;
    is[TYP_ADR] = 0;
    is[STG_ADR] = 0;
    is[TYP_VAL] = casttype;
    if (dstLong)
        is[TYP_OBJ] = casttype;
    else
        is[TYP_OBJ] = 0;
}

// Try to parse a cast expression after '(' has been matched.
// Returns 1 if a cast was parsed, 0 if not (caller should
// treat '(' as start of parenthesized subexpression).
int trycast(int is[]) {
    char *saved;
    int  sc, snc, casttype, isUnsigned, isSigned, isShort;
    saved = lptr;
    sc = ch;
    snc = nch;
    casttype = 0;
    isUnsigned = isSigned = isShort = 0;

    // Consume const/volatile qualifiers: they do not affect the cast value.
    // "signed"/"unsigned"/"short" modify the target type.
    // "register"/"auto" are accepted as no-ops (unusual in a cast but legal).
    for (;;) {
        if (amatch("const", 5))        { /* no-op in cast */ }
        else if (amatch("volatile", 8)){ /* no-op in cast */ }
        else if (amatch("signed", 6))  { isSigned   = 1; }
        else if (amatch("unsigned", 8)){ isUnsigned  = 1; }
        else if (amatch("short", 5))   { isShort     = 1; }
        else break;
    }

    // short [int] -- on 8086 short == int
    if (isShort) {
        amatch("int", 3);
        casttype = isUnsigned ? TYPE_UINT : TYPE_INT;
    }
    else if (isUnsigned) {
        if (amatch("long", 4)) {
            amatch("int", 3);
            casttype = TYPE_ULONG;
        }
        else if (amatch("char", 4))
            casttype = TYPE_UCHR;
        else {
            amatch("int", 3);
            casttype = TYPE_UINT;
        }
    }
    else if (isSigned) {
        // signed is default; just determine the base type
        if (amatch("char", 4))
            casttype = TYPE_CHR;
        else if (amatch("long", 4)) {
            amatch("int", 3);
            casttype = TYPE_LONG;
        }
        else {
            amatch("int", 3);
            casttype = TYPE_INT;
        }
    }
    else if (amatch("long", 4)) {
        amatch("int", 3);
        casttype = TYPE_LONG;
    }
    else if (amatch("int", 3))
        casttype = TYPE_INT;
    else if (amatch("char", 4))
        casttype = TYPE_CHR;
    else if (amatch("void", 4))
        casttype = TYPE_VOID;
    else if (amatch("enum", 4)) {
        char sname[NAMESIZE];
        symname(sname);      // consume optional tag name
        casttype = TYPE_INT;
    }
    else if (amatch("struct", 6)) {
        char *sp;
        sp = getTagPtr(KIND_STRUCT);
        if (sp == -1) error("unknown struct name");
        casttype = TYPE_STRUCT;
    }
    else if (amatch("union", 5)) {
        char *sp;
        sp = getTagPtr(KIND_UNION);
        if (sp == -1) error("unknown union name");
        casttype = TYPE_STRUCT;
    }

    if (casttype == 0) {
        lptr = saved;
        ch = sc;
        nch = snc;
        return 0;
    }

    if (IsMatch("*")) {
        // Pointer cast: (type *)expr
        // The value in AX is left unchanged; only expression metadata is
        // updated to reflect "this is a pointer to casttype".
        Require(")");
        if (level13(is)) fetch(is);
        is[TYP_ADR]  = casttype;
        is[TYP_OBJ]  = 0;
        is[TYP_VAL]  = 0;
        is[TYP_CNST] = 0;
        return 1;
    }

    Require(")");

    if (level13(is)) fetch(is);

    applycast(casttype, is);
    return 1;
}

int primary(int *is) {
    char *ptr, sname[NAMESIZE];
    int k;
    if (IsMatch("(")) {                  // (cast) or (subexpression)
        if (trycast(is)) return 0;
        do {
            k = level1(is); 
        } while (IsMatch(","));
        Require(")");
        return k;
    }
    putint(0, is, ISSIZE << LBPW);    // clear "is" array
    if (symname(sname)) {              // is legal symbol
        if (ptr = findloc(sname)) {      // is local
            if (ptr[IDENT] == IDENT_LABEL) {
                experr();
                return 0;
            }
            if ((ptr[CLASS] & 0x7F) == STATIC) {
                // static local: redirect to the global symbol table entry
                // stored in the OFFSET field of the local entry
                ptr = getint(ptr + OFFSET, 2);
                is[SYMTAB_ADR] = ptr;
                if (ptr[IDENT] == IDENT_ARRAY) {
                    gen(POINT1m, ptr);
                    is[TYP_OBJ] = is[TYP_ADR] = ptr[TYPE];
                    if (ptr[NDIM] > 1)
                        is[DIM_LEFT] = ptr[NDIM];
                    return 0;
                }
                if (ptr[IDENT] == IDENT_PTR_ARRAY) {
                    gen(POINT1m, ptr);
                    is[TYP_OBJ] = is[TYP_ADR] = TYPE_UINT;
                    return 0;
                }
                if (ptr[IDENT] == IDENT_POINTER) {
                    is[TYP_ADR] = ptr[TYPE];
                    if (ptr[NDIM] > 1)
                        is[DIM_LEFT] = ptr[NDIM];
                }
                return 1;
            }
            gen(POINT1s, getint(ptr + OFFSET, 2));
            is[SYMTAB_ADR] = ptr;
            is[TYP_OBJ] = ptr[TYPE];
            if (ptr[IDENT] == IDENT_ARRAY) {
                is[TYP_ADR] = ptr[TYPE];
                if (ptr[NDIM] > 1)
                    is[DIM_LEFT] = ptr[NDIM];
                return 0;
            }
            if (ptr[IDENT] == IDENT_PTR_ARRAY) {
                is[TYP_ADR] = TYPE_UINT;
                return 0;
            }
            if (ptr[IDENT] == IDENT_POINTER) {
                is[TYP_OBJ] = TYPE_UINT;
                is[TYP_ADR] = ptr[TYPE];
                if (ptr[NDIM] > 1)
                    is[DIM_LEFT] = ptr[NDIM];
            }
            return 1;
        }
        if (ptr = findglb(sname)) {      // is global
            is[SYMTAB_ADR] = ptr;
            if ((ptr[CLASS] & 0x7F) == ENUMCONST) {
                // enum constant: fold as integer compile-time value
                is[TYP_CNST] = TYPE_INT;
                is[VAL_CNST] = getint(ptr + OFFSET, 2);
                is[SYMTAB_ADR] = is[TYP_OBJ] = is[TYP_ADR] = 0;
                return 0;
            }
            if (ptr[IDENT] != IDENT_FUNCTION && ptr[IDENT] != IDENT_PTR_FUNCTION) {
                if (ptr[IDENT] == IDENT_ARRAY) {
                    gen(POINT1m, ptr);
                    is[TYP_OBJ] = is[TYP_ADR] = ptr[TYPE];
                    if (ptr[NDIM] > 1)
                        is[DIM_LEFT] = ptr[NDIM];
                    return 0;
                }
                if (ptr[IDENT] == IDENT_PTR_ARRAY) {
                    gen(POINT1m, ptr);
                    is[TYP_OBJ] = is[TYP_ADR] = TYPE_UINT;
                    return 0;
                }
                if (ptr[IDENT] == IDENT_POINTER) {
                    is[TYP_ADR] = ptr[TYPE];
                    if (ptr[NDIM] > 1)
                        is[DIM_LEFT] = ptr[NDIM];
                }
                return 1;
            }
        }
        else {
            // If both searches fail, the symbol is added to the global table
            // as a function. Then, if it never is formally declared, it will
            // be identified to the assembler as an external reference before
            // the compiler quits.
            is[SYMTAB_ADR] = AddSymbol(sname, IDENT_FUNCTION, TYPE_INT, 
                0, 0, &glbptr, AUTOEXT);
        }
        return 0;
    }
    if (constant(is) == 0) {
        experr();
    }
    return 0;
}

void experr() {
    error("invalid expression");
    gen(GETw1n, 0);
    skipToNextToken();
}

int callfunc(char *ptr) {      // symbol table entry or 0
    int nargs, nargcnt, is[ISSIZE];
    int savedlocptr, retStructSize;
    int argLens[MAX_CALL_ARGS];  // ints stored in argbuf per user arg
    int argSizes[MAX_CALL_ARGS]; // BPW or BPD per user arg
    int argBufNext;              // absolute write cursor into argbuf
    int argbufBase;              // argbufcur at entry to this callfunc level
    int userArgCount;            // number of user (non-hidden) args collected
    int indSaveCsp;              // BP offset of saved funcptr (indirect calls)
    int *exprStart;
    int i, alen;
    nargs = 0;
    nargcnt = 0;
    retStructSize = 0;
    argbufBase = argbufcur;   // save so nested calls start ABOVE our data
    argBufNext = argbufcur;   // our local cursor starts at current global position
    userArgCount = 0;
    indSaveCsp = 0;
    blanks();                      // already saw open paren
#ifdef ENABLE_WARNINGS
#ifdef WARN_IMPLICIT
    // Warn if the function was called without a prior prototype or declaration.
    if (ptr && (ptr[CLASS] & 0x7F) == AUTOEXT && getint(ptr + FNPARAMPTR, 2) == 0)
        warningWithName("implicit declaration of function '", ptr + NAME, "'");
#endif
#endif
    // Upgrade PROTOEXT -> AUTOEXT on first call so trailer() emits EXTRN.
    if (ptr && (ptr[CLASS] & 0x7F) == PROTOEXT)
        ptr[CLASS] = AUTOEXT;
    // For struct-returning functions, allocate the return buffer on the caller's
    // stack and push a hidden pointer to it as the FIRST argument (highest BP
    // offset). User arguments follow at lower BP offsets via R-to-L push.
    if (ptr && ptr[TYPE] == TYPE_STRUCT) {
        retStructSize = getStructSize(getClsPtr(ptr));
        gen(ADDSP, csp - retStructSize);
        gen(POINT1s, csp);
        gen(PUSH1, 0);
        nargs += BPW;
        nargcnt++;
    }
    // For indirect calls (function pointer): save funcptr from AX to a temp
    // stack slot now, before arg evaluation clobbers AX.
    if (ptr == 0) {
        gen(ADDSP, csp - BPW);   // allocate one-word temp slot
        indSaveCsp = csp;        // BP offset of the temp slot
        gen(PUTws1, indSaveCsp); // [BP + indSaveCsp] = AX (save funcptr)
        nargs += BPW;            // temp slot cleaned up by ADDSP after call
    }
    // FIRST PASS: evaluate each argument L-to-R, collect p-codes into argbuf,
    // then restore snext to discard them from the staging buffer.
    while (streq(lptr, ")") == 0) {
        if (endst()) {
            break;
        }
        if (userArgCount >= MAX_CALL_ARGS) {
            error("too many arguments");
            break;
        }
        savedlocptr = locptr;
        exprStart = snext;
        // Evaluate the argument expression into the staging buffer.
        if (level1(is)) {
            char *argptr;
            if (is[TYP_OBJ] == TYPE_STRUCT) {
                // struct lvalue: address already in AX, no fetch needed
            }
            else if (is[TYP_OBJ] == 0 && (argptr = is[SYMTAB_ADR])
                     && argptr[TYPE] == TYPE_STRUCT
                     && argptr[IDENT] != IDENT_FUNCTION
                     && argptr[IDENT] != IDENT_PTR_FUNCTION) {
                gen(POINT1m, argptr);
            }
            else {
                fetch(is);
            }
        }
        locptr = savedlocptr;
        // Copy this arg's p-codes from stage[] into argbuf.
        alen = (int)(snext - exprStart);
        if (argBufNext + alen + 2 > ARGBUF_SIZE) {
            error("argument buffer overflow");
            break;
        }
        for (i = 0; i < alen; i++)
            argbuf[argBufNext + i] = exprStart[i];
        argLens[userArgCount] = alen;
        argSizes[userArgCount] = isLongVal(is) ? BPD : BPW;
        argBufNext += alen;
        argbufcur = argBufNext;   // advance global so next inner call starts here
        nargs += argSizes[userArgCount];
        // Discard this arg's p-codes from stage[]; next arg writes in same slot.
        snext = exprStart;
        userArgCount++;
        nargcnt++;
        if (IsMatch(",") == 0) {
            break;
        }
    }
    Require(")");
    // Check argument count against prototype if available.
    if (ptr && getint(ptr + FNPARAMPTR, 2) != 0) {
        int protoIdx, nbyte, isVariadic, nfixed, effectiveArgs;
        protoIdx = getint(ptr + FNPARAMPTR, 2);
        nbyte = (int)(paramTypes[protoIdx]) & 0xFF;
        isVariadic = (nbyte >> 7) & 1;
        nfixed = nbyte & 0x7F;
        effectiveArgs = nargcnt;
        if (ptr[TYPE] == TYPE_STRUCT)
            --effectiveArgs;
#ifdef ENABLE_WARNINGS
#ifdef WARN_ARGCOUNT
        if (isVariadic) {
            if (effectiveArgs < nfixed)
                warningWithName("too few arguments for variadic function '", ptr + NAME, "'");
        }
        else {
            if (effectiveArgs != nfixed)
                warningWithName("wrong number of arguments to '", ptr + NAME, "'");
        }
#endif
#endif
    }
    // SECOND PASS: emit user args in REVERSED order (last arg first).
    // This produces right-to-left push order so the first arg lands at [BP+4].
    // Walk backward through argbuf by subtracting lengths, avoiding a
    // separate argBufStart[] array on the (tight) stack.
    {
        int pos = argBufNext;   // one-past-end of all collected arg p-codes
        for (i = userArgCount - 1; i >= 0; i--) {
            int j, len;
            int *src;
            pos -= argLens[i];  // start of arg i's p-codes in argbuf
            len = argLens[i];
            src = argbuf + pos;
            if (snext + len + 2 > slast) {
                error("staging buffer overflow");
                break;
            }
            for (j = 0; j < len; j++)
                snext[j] = src[j];
            snext += len;
            gen(argSizes[i] == BPD ? PUSHd1 : PUSH1, 0);
        }
        argbufcur = argbufBase;   // restore/free: inner calls may reuse this space
    }
    // For indirect calls: reload the saved funcptr into AX before CALL1.
    if (ptr == 0) {
        gen(GETw1s, indSaveCsp);
    }
    if (ptr) {
        gen(CALLm, ptr);
    }
    else {
        gen(CALL1, 0);
    }
    gen(ADDSP, csp + nargs);
    if (retStructSize > 0) {
        gen(POINT1s, csp);
    }
}

// Return the pointer scale factor: number of bytes per element.
// Returns 0 if no scaling needed (not a pointer context).
// is1 is the pointer side, is2 is the non-pointer side.
int ptrScale(int oper, int is1[], int is2[]) {
    if ((oper != ADD12 && oper != SUB12)
        || (is1[TYP_ADR] == 0)
        || (is2[TYP_ADR])) {
        return 0;
    }
    if (is1[TYP_ADR] == TYPE_VOID) {
        error("void pointer arithmetic not allowed");
        return 0;
    }
    return is1[TYP_ADR] >> 2;
}

// true if is2's operand should be doubled (word pointer scaling)
int isDouble(int oper, int is1[], int is2[]) {
    return (ptrScale(oper, is1, is2) == BPW);
}

void step(int oper, int is[], int oper2) {
    int longval, stepsize;
    longval = isLongVal(is);
    fetch(is);
    stepsize = is[TYP_ADR] ? (is[TYP_ADR] >> 2) : 1;
    if (longval && !is[TYP_ADR]) {
        // 32-bit value increment/decrement
        gen(oper == rINC1 ? rINCd1 : rDECd1, stepsize);
        store(is);
        if (oper2)
            gen(oper2 == rDEC1 ? rDECd1 : rINCd1, stepsize);
    }
    else {
        gen(oper, stepsize);
        store(is);
        if (oper2)
            gen(oper2, stepsize);
    }
}

// Return 1 if expression in is[] yields a 32-bit (long) value.
// Pointers are always 16-bit, never long.
int isLongVal(int is[]) {
    char *ptr;
    if (is[TYP_CNST] >> 2 == BPD)
        return 1;
    if (is[TYP_OBJ] >> 2 == BPD)
        return 1;
    ptr = is[SYMTAB_ADR];
    if (ptr && is[TYP_OBJ] == 0 && is[TYP_ADR] == 0
        && ptr[IDENT] != IDENT_POINTER
        && ptr[TYPE] >> 2 == BPD)
        return 1;
    return 0;
}

// Sign- or zero-extend AX to DX:AX based on signedness of is[].
void widen_primary(int is[]) {
    if (IsUnsigned(is))
        gen(WIDENu, 0);
    else
        gen(WIDENs, 0);
}

// Sign- or zero-extend BX to CX:BX based on signedness of is[].
void widen_secondary(int is[]) {
    if (IsUnsigned(is))
        gen(WIDENu2, 0);
    else
        gen(WIDENs2, 0);
}

int toLongTab[] = {
    ADD12, ADDd12,   SUB12, SUBd12,   MUL12, MULd12,   DIV12, DIVd12,
    MOD12, MODd12,   AND12, ANDd12,   OR12,  ORd12,     XOR12, XORd12,
    ASL12, ASLd12,   ASR12, ASRd12,   EQ12,  EQd12,     NE12,  NEd12,
    LT12,  LTd12,    LE12,  LEd12,    GT12,  GTd12,     GE12,  GEd12,
    MUL12u,MULd12u,  DIV12u,DIVd12u,  MOD12u,MODd12u,
    LT12u, LTd12u,   LE12u, LEd12u,   GT12u, GTd12u,    GE12u, GEd12u,
    LSR12, ASRd12u,
    0, 0
};

int reverseTab[] = {
    LT12,  GT12,    GT12,  LT12,    LE12,  GE12,    GE12,  LE12,
    LT12u, GT12u,   GT12u, LT12u,   LE12u, GE12u,   GE12u, LE12u,
    EQ12,  EQ12,    NE12,  NE12,
    0, 0
};

int cmpOpTab[] = {
    EQd12,  1,  NEd12,  1,
    LTd12,  1,  LEd12,  1,  GTd12,  1,  GEd12,  1,
    LTd12u, 1,  LEd12u, 1,  GTd12u, 1,  GEd12u, 1,
    0, 0
};

// Search a {find, replace, ..., 0} table for key; return replace or 0.
int tableFind(int tab[], int key) {
    while (*tab) {
        if (*tab == key) return *(tab + 1);
        tab += 2;
    }
    return 0;
}

// Map a 16-bit signed operator to its 32-bit equivalent.
int toLongOp(int oper) {
    int r;
    r = tableFind(toLongTab, oper);
    return r ? r : oper;
}

// Reverse a comparison operator's operand sense (LT<->GT, LE<->GE).
// Returns the reversed op, or 0 if the operator is not reversible.
int reverseOp(int oper) {
    return tableFind(reverseTab, oper);
}

// Return 1 if oper is a comparison operator (result is always 16-bit).
int isCmpOp(int oper) {
    return tableFind(cmpOpTab, oper);
}

void store(int is[]) {
    char *ptr;
    if (is[TYP_OBJ]) {                    // putstk
        if (is[TYP_OBJ] >> 2 == BPD) {
            gen(PUTdp1, 0);
        }
        else if (is[TYP_OBJ] >> 2 == 1) {
            gen(PUTbp1, 0);
        }
        else {
            gen(PUTwp1, 0);
        }
    }
    else {                          // putmem
        ptr = is[SYMTAB_ADR];
        if (ptr[IDENT] != IDENT_POINTER && ptr[TYPE] >> 2 == BPD) {
            gen(PUTdm1, ptr);
        }
        else if (ptr[IDENT] != IDENT_POINTER && ptr[TYPE] >> 2 == 1) {
            gen(PUTbm1, ptr);
        }
        else {
            gen(PUTwm1, ptr);
        }
    }
}

void fetch(int is[]) {
    char *ptr;
    ptr = is[SYMTAB_ADR];
    if (is[TYP_OBJ]) {                                   // indirect
        if (is[TYP_OBJ] >> 2 == BPD) {
            gen(GETd1p, 0);
        }
        else if (is[TYP_OBJ] >> 2 == BPW) {
            gen(GETw1p, 0);
        }
        else {
            // Use TYP_OBJ (not ptr[TYPE]) so that pointer casts are respected.
            if (is[TYP_OBJ] & IS_UNSIGNED)
                gen(GETb1pu, 0);
            else
                gen(GETb1p, 0);
        }
    }
    else {                                         // direct
        if (ptr[IDENT] == IDENT_POINTER || ptr[TYPE] >> 2 == BPW) {
            gen(GETw1m, ptr);
        }
        else if (ptr[TYPE] >> 2 == BPD) {
            gen(GETd1m, ptr);
        }
        else {
            if (ptr[TYPE] & IS_UNSIGNED) 
                gen(GETb1mu, ptr);
            else
                gen(GETb1m, ptr);
        }
    }
}

int constant(int is[]) {
    int offset;
    if (is[TYP_CNST] = number(&is[VAL_CNST], &is[VAL_CNST_HI])) {
        if (is[TYP_CNST] >> 2 == BPD) {
            gen(GETw1n, is[VAL_CNST]);
            gen(GETdxn, is[VAL_CNST_HI]);
        }
        else {
            gen(GETw1n, is[VAL_CNST]);
        }
    }
    else if (is[TYP_CNST] = chrcon(&is[VAL_CNST], &is[VAL_CNST_HI])) {
        if (is[TYP_CNST] >> 2 == BPD) {
            gen(GETw1n, is[VAL_CNST]);
            gen(GETdxn, is[VAL_CNST_HI]);
        }
        else {
            gen(GETw1n, is[VAL_CNST]);
        }
    }
    else if (string(&offset))          gen(POINT1l, offset);
    else return 0;
    return 1;
}

// === 32-bit compile-time arithmetic helpers ================================
// These operate on hi:lo pairs for the 16-bit self-hosted compiler.

// Multiply khi:klo by a small factor (8, 10, or 16). Result in *hi:*lo.
void mul32x16(int *hi, int *lo, int factor) {
    int ahi, bhi, carry;
    unsigned alo, blo, newalo;
    // Shift-and-add: multiply *hi:*lo by factor
    ahi = 0;
    alo = 0;
    bhi = *hi;
    blo = *lo;
    while (factor) {
        if (factor & 1) {
            // add bhi:blo to ahi:alo (unsigned low word)
            newalo = alo + blo;
            carry = 0;
            if (newalo < alo) carry = 1;
            alo = newalo;
            ahi = ahi + bhi + carry;
        }
        // double bhi:blo
        carry = 0;
        if (blo & 0x8000) carry = 1;
        blo = blo << 1;
        bhi = (bhi << 1) + carry;
        factor = factor >> 1;
    }
    *lo = alo;
    *hi = ahi;
}

// Add a small unsigned value (0-15) to khi:klo.
void add32x16(int *hi, int *lo, int addend) {
    unsigned oldlo, newlo;
    oldlo = *lo;
    newlo = oldlo + addend;
    if (newlo < oldlo)
        *hi = *hi + 1;
    *lo = newlo;
}

// Two's complement negate hi:lo.
void negate32(int *hi, int *lo) {
    *hi = ~(*hi);
    *lo = ~(*lo);
    add32x16(hi, lo, 1);
}

// Return 1 if hi:lo fits in signed 16-bit range (-32768..32767).
int fits16s(int hi, int lo) {
    if (hi == 0 && !(lo & 0x8000)) return 1;   // 0..32767
    if (hi == -1 && (lo & 0x8000)) return 1;    // -32768..-1
    return 0;
}

// Return 1 if hi:lo fits in unsigned 16-bit range (0..65535).
int fits16u(int hi, int lo) {
    if (hi == 0) return 1;
    return 0;
}

int number(int *lo, int *hi) {
    int klo, khi, minus, hasL, hasU, base, digit;
    klo = khi = minus = hasL = hasU = 0;
    while (1) {
        if (IsMatch("+"));
        else if (IsMatch("-"))
            minus = 1;
        else
            break;
    }
    if (isdigit(ch) == 0) return 0;
    if (ch == '0') {
        while (ch == '0') inbyte();
        if (toupper(ch) == 'X') {
            inbyte();
            base = 16;
            while (isxdigit(ch)) {
                if (isdigit(ch))
                    digit = inbyte() - '0';
                else
                    digit = 10 + (toupper(inbyte()) - 'A');
                mul32x16(&khi, &klo, base);
                add32x16(&khi, &klo, digit);
            }
        }
        else {
            base = 8;
            while (ch >= '0' && ch <= '7') {
                digit = inbyte() - '0';
                mul32x16(&khi, &klo, base);
                add32x16(&khi, &klo, digit);
            }
        }
    }
    else {
        base = 10;
        while (isdigit(ch)) {
            digit = inbyte() - '0';
            mul32x16(&khi, &klo, base);
            add32x16(&khi, &klo, digit);
        }
    }
    // Parse suffixes: L, U, UL, LU (case-insensitive)
    if (toupper(ch) == 'U') {
        inbyte();
        hasU = 1;
        if (toupper(ch) == 'L') {
            inbyte();
            hasL = 1;
        }
    }
    else if (toupper(ch) == 'L') {
        inbyte();
        hasL = 1;
        if (toupper(ch) == 'U') {
            inbyte();
            hasU = 1;
        }
    }
    if (minus) {
        negate32(&khi, &klo);
    }
    *lo = klo;
    *hi = khi;
    // C89 type promotion rules:
    // Decimal no suffix: int -> long -> unsigned long
    // Octal/hex no suffix: int -> unsigned int -> long -> unsigned long
    // U suffix: unsigned int -> unsigned long
    // L suffix: long -> unsigned long
    // UL suffix: unsigned long
    if (hasU && hasL) {
        return TYPE_ULONG;
    }
    if (hasL) {
        // L suffix: long, or unsigned long if doesn't fit signed long
        if (khi & 0x8000)
            return TYPE_ULONG;
        return TYPE_LONG;
    }
    if (hasU) {
        // U suffix: unsigned int if fits, else unsigned long
        if (fits16u(khi, klo))
            return TYPE_UINT;
        return TYPE_ULONG;
    }
    // No suffix
    if (base == 10) {
        // decimal: int -> long -> unsigned long
        if (fits16s(khi, klo))
            return TYPE_INT;
        if (!(khi & 0x8000))
            return TYPE_LONG;
        return TYPE_ULONG;
    }
    else {
        // octal/hex: int -> unsigned int -> long -> unsigned long
        if (fits16s(khi, klo))
            return TYPE_INT;
        if (fits16u(khi, klo))
            return TYPE_UINT;
        if (!(khi & 0x8000))
            return TYPE_LONG;
        return TYPE_ULONG;
    }
}

int chrcon(int *lo, int *hi) {
    int klo, khi, carry;
    klo = khi = 0;
    if (IsMatch("'") == 0)
        return 0;
    while (ch != '\'') {
        // shift khi:klo left by 8 and add new char
        khi = (khi << 8) | ((klo >> 8) & 0xFF);
        klo = ((klo << 8) + (litchar() & 255));
    }
    gch();
    *lo = klo;
    *hi = khi;
    if (khi)
        return TYPE_LONG;
    return TYPE_INT;
}

// Parse a string literal, storing characters in litq[] and returning offset.
// Returns 1 if a string was parsed, 0 if not.
// Handles adjacent string literals by concatenatation. Macro-expanded string
// literatals are supported: blanks() calls preprocess() which expands macros.
// #define S "world" and then "hello " S will concatenate to "hello world".
int string(int *offset) {
    if (IsMatch("\"") == 0) {
        return 0;
    }
    *offset = litptr;
    do {
        while (ch != '"') {
            if (ch == 0)
                break;
            stowlit(litchar(), 1);
        }
        gch();
    } while (IsMatch("\""));
    litq[litptr++] = 0;
    return 1;
}

void stowlit(int value, int size) {
    if ((litptr + size) >= LITMAX) {
        error("literal queue overflow");
        abort(ERRCODE);
    }
    putint(value, litq + litptr, size);
    litptr += size;
}

int litchar() {
    int i, oct;
    if (ch != '\\' || nch == 0) 
        return gch();
    gch();
    switch (ch) {
        case 'n': gch();
            return NEWLINE;
        case 't': gch();
            return  9;  // HT
        case 'b': gch();
            return  8;  // BS
        case 'f': gch();
            return 12;  // FF
    }
    i = 3;
    oct = 0;
    while ((i--) > 0 && ch >= '0' && ch <= '7') {
        oct = (oct << 3) + gch() - '0';
    }
    if (i == 2)
        return gch();
    else
        return oct;
}

// === pipeline functions =====================================================

// skim over terms adjoining || and && operators
int skim(char *opstr, int tcode, int dropval, int endval, int (*level)(), int is[]) {
    int k, droplab, endlab;
    droplab = 0;
    while (1) {
        k = down1(level, is);
        if (nextop(opstr)) {
            bump(opsize);
            if (droplab == 0)
                droplab = getlabel();
            dropout(k, tcode, droplab, is);
        }
        else if (droplab) {
            dropout(k, tcode, droplab, is);
            gen(GETw1n, endval);
            gen(JMPm, endlab = getlabel());
            gen(LABm, droplab);
            gen(GETw1n, dropval);
            gen(LABm, endlab);
            is[TYP_OBJ] = is[TYP_ADR] = is[TYP_CNST] = is[VAL_CNST] = is[STG_ADR] = 0;
            return 0;
        }
        else
            return k;
    }
}

// test for early dropout from || or && sequences
void dropout(int k, int tcode, int exit1, int is[]) {
    if (k)
        fetch(is);
    else if (is[TYP_CNST]) {
        gen(GETw1n, is[VAL_CNST]);
        if (is[TYP_CNST] >> 2 == BPD)
            gen(GETdxn, is[VAL_CNST_HI]);
    }
    // use 32-bit truthiness test when expression is long
    if (isLongVal(is)) {
        if (tcode == NE10f)
            gen(NEd10f, exit1);
        else if (tcode == EQ10f)
            gen(EQd10f, exit1);
        else
            gen(tcode, exit1);
    }
    else
        gen(tcode, exit1);          // jumps on false
}

// drop to a lower level
int down(char *opstr, int opoff, int (*level)(), int is[]) {
    int k;
    k = down1(level, is);
    if (nextop(opstr) == 0)
        return k;
    if (k)
        fetch(is);
    while (1) {
        if (nextop(opstr)) {
            int is2[ISSIZE];     // allocate only if needed
            bump(opsize);
            opindex += opoff;
            down2(op[opindex], op2[opindex], level, is, is2);
        }
        else return 0;
    }
}

// unary drop to a lower level
int down1(int (*level)(), int is[]) {
    int k, *before, *start;
    setstage(&before, &start);
    k = (*level)(is);
    if (is[TYP_CNST]) {
        ClearStage(before, 0);  // load constant later
    }
    return k;
}

// binary drop to a lower level
void down2(int oper, int oper2, int (*level)(), int is[], int is2[]) {
    int *before, *start;
    char *ptr;
    int leftLong, rightLong, eitherLong, loper, sc;
    setstage(&before, &start);
    is[STG_ADR] = 0;                             // not "... op 0" syntax
    leftLong = isLongVal(is);
    if (is[TYP_CNST]) {                           // constant op unknown
        if (down1(level, is2))
            fetch(is2);
        rightLong = isLongVal(is2);
        eitherLong = leftLong || rightLong;
        if (eitherLong) {
            // widen RHS in DX:AX if it's 16-bit
            if (!rightLong) widen_primary(is2);
            if (is[VAL_CNST] == 0 && is[VAL_CNST_HI] == 0)
                is[STG_ADR] = snext;
            gen(GETw2n, is[VAL_CNST]);
            gen(GETcxn, is[VAL_CNST_HI]);
        }
        else {
            if (is[VAL_CNST] == 0)
                is[STG_ADR] = snext;
            sc = ptrScale(oper, is2, is); if (!sc) sc = 1;
            gen(GETw2n, is[VAL_CNST] * sc);
        }
    }
    else {                                  // variable op unknown
        if (leftLong)
            gen(PUSHd1, 0);
        else
            gen(PUSH1, 0);                      // at start in the buffer
        if (down1(level, is2)) fetch(is2);
        rightLong = isLongVal(is2);
        eitherLong = leftLong || rightLong;
        if (is2[TYP_CNST]) {                      // variable op constant
            if (is2[VAL_CNST] == 0 && is2[VAL_CNST_HI] == 0)
                is[STG_ADR] = start;
            if (leftLong)
                csp += BPD;
            else
                csp += BPW;                     // adjust stack and
            ClearStage(before, 0);          // discard the PUSH
            if (eitherLong) {
                if (!leftLong)
                    widen_primary(is);  // widen left in DX:AX
                if (oper == ADD12 || oper == ADDd12) {
                    // commutative: left stays in DX:AX
                    gen(GETw2n, is2[VAL_CNST]);
                    gen(GETcxn, is2[VAL_CNST_HI]);
                }
                else {
                    // non-commutative: swap
                    gen(MOVEd21, 0);
                    gen(GETw1n, is2[VAL_CNST]);
                    gen(GETdxn, is2[VAL_CNST_HI]);
                }
            }
            else {
                sc = ptrScale(oper, is, is2); if (!sc) sc = 1;
                if (oper == ADD12) {            // commutative
                    gen(GETw2n, is2[VAL_CNST] * sc);
                }
                else {                          // non-commutative
                    // If the operator pair can be reversed, swap operands and use GETw2n
                    // like the commutative case, avoiding MOVE21+GETw1n overhead.
                    int rev, rev2;
                    rev = reverseOp(oper);
                    rev2 = reverseOp(oper2);
                    if (rev && rev2 && !is[STG_ADR]) {
                        gen(GETw2n, is2[VAL_CNST] * sc);
                        oper = rev;
                        oper2 = rev2;
                    }
                    else {
                        gen(MOVE21, 0);
                        gen(GETw1n, is2[VAL_CNST] * sc);
                    }
                }
            }
        }
        else {                              // variable op variable
            // If the RHS was just loaded as a simple word-sized variable, reverse the operator
            // to emit GETw2m/GETw2s directly, enabling CMP-Memory/Stack peephole rules.
            if (!eitherLong && before && snext == before + 4
                && (before[2] == GETw1m || before[2] == GETw1s)) {
                // RHS is a simple word-sized global or stack variable.
                // Try to reverse the comparison so we can emit GETw2x
                // directly, enabling CMP-Memory/Stack optimizer rules.
                int rev, rev2, rhspc, rhsval;
                rev = reverseOp(oper);
                rev2 = reverseOp(oper2);
                if (rev && rev2) {
                    rhspc = before[2];
                    rhsval = before[3];
                    csp += BPW;
                    ClearStage(before, 0);
                    gen(rhspc == GETw1m ? GETw2m : GETw2s, rhsval);
                    oper = rev;
                    oper2 = rev2;
                }
                else {
                    gen(POP2, 0);
                }
            }
            else if (eitherLong) {
                // widen right (in DX:AX) if not long
                if (!rightLong) widen_primary(is2);
                // pop left
                if (leftLong)
                    gen(POPd2, 0);
                else {
                    gen(POP2, 0);
                    widen_secondary(is);
                }
            }
            else {
                gen(POP2, 0);
                if (ptrScale(oper, is, is2) == BPD) {
                    gen(DBL1, 0); gen(DBL1, 0);
                }
                else if (isDouble(oper, is, is2)) gen(DBL1, 0);
                if (ptrScale(oper, is2, is) == BPD) {
                    gen(DBL2, 0); gen(DBL2, 0);
                }
                else if (isDouble(oper, is2, is)) gen(DBL2, 0);
            }
        }
    }
    if (oper) {
        if (IsUnsigned(is) || IsUnsigned(is2)) {
            oper = oper2;
        }
        if (eitherLong)
            loper = toLongOp(oper);
        else
            loper = oper;
        if (is[TYP_CNST] && is2[TYP_CNST]) {               // constant result
            if (eitherLong) {
                // 32-bit constant folding
                int rhi, rlo, lhi, llo, reshi, reslo;
                // Widen 16-bit constants to 32-bit for folding
                llo = is[VAL_CNST];
                lhi = is[VAL_CNST_HI];
                if (!leftLong) {
                    // sign- or zero-extend left
                    if (is[TYP_CNST] == TYPE_UINT)
                        lhi = 0;
                    else
                        lhi = (llo & 0x8000) ? -1 : 0;
                }
                rlo = is2[VAL_CNST];
                rhi = is2[VAL_CNST_HI];
                if (!rightLong) {
                    if (is2[TYP_CNST] == TYPE_UINT)
                        rhi = 0;
                    else
                        rhi = (rlo & 0x8000) ? -1 : 0;
                }
                reshi = reslo = 0;
                // Route to appropriate folding function
                if (oper == LT12 || oper == LE12 ||
                    oper == GT12 || oper == GE12) {
                    CalcCmp32(lhi, llo, oper, rhi, rlo,
                              &reshi, &reslo);
                }
                else if (IsUnsigned(is) || IsUnsigned(is2)) {
                    CalcUConst32(lhi, llo, oper, rhi, rlo,
                                 &reshi, &reslo);
                }
                else {
                    CalcConst32(lhi, llo, oper, rhi, rlo,
                                &reshi, &reslo);
                }
                is[VAL_CNST] = reslo;
                is[VAL_CNST_HI] = reshi;
                // Determine result type
                if (oper == EQ12 || oper == NE12 ||
                    oper == LT12 || oper == LE12 ||
                    oper == GT12 || oper == GE12 ||
                    oper == LT12u || oper == LE12u ||
                    oper == GT12u || oper == GE12u) {
                    // comparison: result is 16-bit int
                    is[TYP_CNST] = TYPE_INT;
                    is[VAL_CNST_HI] = 0;
                }
                else if (IsUnsigned(is) || IsUnsigned(is2)) {
                    is[TYP_CNST] = TYPE_ULONG;
                }
                else {
                    is[TYP_CNST] = TYPE_LONG;
                }
            }
            else {
                is[VAL_CNST] = CalcConst(is[VAL_CNST], oper, is2[VAL_CNST]);
                if (is2[TYP_CNST] == TYPE_UINT) {
                    is[TYP_CNST] = TYPE_UINT;
                }
            }
            ClearStage(before, 0);
        }
        else {
            // variable result
            is[TYP_CNST] = 0;
            is[VAL_CNST_HI] = 0;
            gen(loper, 0);
            if (oper == SUB12 && is[TYP_ADR] >> 2 == BPW && is2[TYP_ADR] >> 2 == BPW) { 
                // difference of two word addresses
                gen(SWAP12, 0);
                gen(GETw1n, 1);
                gen(ASR12, 0);          // div by 2
            }
            if (oper == SUB12 && is[TYP_ADR] >> 2 == BPD && is2[TYP_ADR] >> 2 == BPD) { 
                // difference of two long addresses: div by 4
                gen(SWAP12, 0);
                gen(GETw1n, 2);
                gen(ASR12, 0);
            }
            // identify the operator
            is[LAST_OP] = loper;
            // comparison results are always 16-bit
            if (eitherLong && isCmpOp(loper)) {
                eitherLong = 0;
                is[TYP_OBJ] = 0;
                is[SYMTAB_ADR] = 0;
            }
            // non-comparison long ops produce a 32-bit result
            else if (eitherLong) {
                if (IsUnsigned(is) || IsUnsigned(is2))
                    is[TYP_OBJ] = TYPE_ULONG;
                else
                    is[TYP_OBJ] = TYPE_LONG;
                is[SYMTAB_ADR] = 0;
            }
        }
        if (oper == SUB12 || oper == ADD12) {
            if (is[TYP_ADR] && is2[TYP_ADR]) {
                //  addr +/- addr
                is[TYP_ADR] = 0;
            }
            else if (is2[TYP_ADR]) {
                // value +/- addr
                is[SYMTAB_ADR] = is2[SYMTAB_ADR];
                is[TYP_OBJ] = is2[TYP_OBJ];
                is[TYP_ADR] = is2[TYP_ADR];
            }
        }
        if (is[SYMTAB_ADR] == 0 || ((ptr = is2[SYMTAB_ADR]) && (ptr[TYPE] & IS_UNSIGNED))) {
            is[SYMTAB_ADR] = is2[SYMTAB_ADR];
        }
    }
}

// unsigned operand?
int IsUnsigned(int is[]) {
    char *ptr;
    if (is[TYP_ADR] || is[TYP_CNST] == TYPE_UINT || is[TYP_CNST] == TYPE_ULONG ||
        is[TYP_VAL] & IS_UNSIGNED ||
        ((ptr = is[SYMTAB_ADR]) && (ptr[TYPE] & IS_UNSIGNED)))
        return 1;
    return 0;
}

// calculate signed constant result
int CalcConst(int left, int oper, int right) {
    switch (oper) {
        case ADD12:
            return (left + right);
        case SUB12:
            return (left - right);
        case MUL12:
            return (left  *  right);
        case DIV12:
            return (left / right);
        case MOD12:
            return (left  %  right);
        case EQ12:
            return (left == right);
        case NE12:
            return (left != right);
        case LE12:
            return (left <= right);
        case GE12:
            return (left >= right);
        case LT12:
            return (left < right);
        case GT12:
            return (left > right);
        case AND12:
            return (left  &  right);
        case OR12: 
            return (left | right);
        case XOR12:
            return (left  ^  right);
        case ASR12:
            return (left >> right);
        case ASL12:
            return (left << right);
    }
    return CalcUConst(left, oper, right);
}

// calculate unsigned constant result
int CalcUConst(unsigned left, int oper, unsigned right) {
    switch (oper) {
        case MUL12u:
            return (left  *  right);
        case DIV12u:
            return (left / right);
        case MOD12u:
            return (left  %  right);
        case LE12u:
            return (left <= right);
        case GE12u:
            return (left >= right);
        case LT12u:
            return (left < right);
        case GT12u:
            return (left > right);
        case LSR12:
            return (left >> right);
    }
    return (0);
}

// 32-bit signed constant folding.  Operates on hi:lo pairs.
// Returns result type: TYPE_LONG or TYPE_ULONG.
// Result stored via reshi/reslo pointers.
void CalcConst32(int lhi, int llo, int oper, int rhi, int rlo,
            int *reshi, int *reslo) {
    unsigned alo, blo, newlo;
    int carry;
    switch (oper) {
        case ADD12:
            alo = llo;
            blo = rlo;
            newlo = alo + blo;
            carry = 0;
            if (newlo < alo) carry = 1;
            *reslo = newlo;
            *reshi = lhi + rhi + carry;
            return;
        case SUB12:
            alo = llo;
            blo = rlo;
            newlo = alo - blo;
            carry = 0;
            if (newlo > alo) carry = 1;
            *reslo = newlo;
            *reshi = lhi - rhi - carry;
            return;
        case AND12:
            *reslo = llo & rlo;
            *reshi = lhi & rhi;
            return;
        case OR12:
            *reslo = llo | rlo;
            *reshi = lhi | rhi;
            return;
        case XOR12:
            *reslo = llo ^ rlo;
            *reshi = lhi ^ rhi;
            return;
        case EQ12:
            *reshi = 0;
            *reslo = (lhi == rhi && llo == rlo) ? 1 : 0;
            return;
        case NE12:
            *reshi = 0;
            *reslo = (lhi != rhi || llo != rlo) ? 1 : 0;
            return;
        case ASL12:
            // shift left by rlo (rhi should be 0 for valid shift)
            *reshi = lhi;
            *reslo = llo;
            carry = rlo & 31;
            while (carry > 0) {
                *reshi = (*reshi << 1);
                if (*reslo & 0x8000) *reshi = *reshi | 1;
                *reslo = *reslo << 1;
                carry--;
            }
            return;
        case ASR12:
            // arithmetic shift right
            *reshi = lhi;
            *reslo = llo;
            carry = rlo & 31;
            while (carry > 0) {
                *reslo = (*reslo >> 1) & 0x7FFF;
                if (*reshi & 1) *reslo = *reslo | 0x8000;
                // arithmetic: preserve sign bit of hi
                if (*reshi & 0x8000)
                    *reshi = (*reshi >> 1) | 0x8000;
                else
                    *reshi = (*reshi >> 1) & 0x7FFF;
                carry--;
            }
            return;
        case LSR12:
            // 32-bit logical (unsigned) right shift
            *reshi = lhi;
            *reslo = llo;
            carry = rlo & 31;
            while (carry > 0) {
                *reslo = (*reslo >> 1) & 0x7FFF;
                if (*reshi & 1) *reslo = *reslo | 0x8000;
                *reshi = (*reshi >> 1) & 0x7FFF;  // zero-fill (logical)
                carry--;
            }
            return;
        default:
            // For MUL/DIV/MOD and comparisons, use mul32x16 approach
            // or fall through to no-fold
            *reslo = llo;
            *reshi = lhi;
            return;
    }
}

// 32-bit unsigned constant folding for comparison operators.
void CalcUConst32(int lhi, int llo, int oper, int rhi, int rlo,
             int *reshi, int *reslo) {
    unsigned ulhi, ullo, urhi, urlo;
    ulhi = lhi; ullo = llo; urhi = rhi; urlo = rlo;
    *reshi = 0;
    switch (oper) {
        case LT12:
        case LT12u:
            if (ulhi < urhi) *reslo = 1;
            else if (ulhi > urhi) *reslo = 0;
            else *reslo = (ullo < urlo) ? 1 : 0;
            return;
        case LE12:
        case LE12u:
            if (ulhi < urhi) *reslo = 1;
            else if (ulhi > urhi) *reslo = 0;
            else *reslo = (ullo <= urlo) ? 1 : 0;
            return;
        case GT12:
        case GT12u:
            if (ulhi > urhi) *reslo = 1;
            else if (ulhi < urhi) *reslo = 0;
            else *reslo = (ullo > urlo) ? 1 : 0;
            return;
        case GE12:
        case GE12u:
            if (ulhi > urhi) *reslo = 1;
            else if (ulhi < urhi) *reslo = 0;
            else *reslo = (ullo >= urlo) ? 1 : 0;
            return;
        case MUL12u:
        case DIV12u:
        case MOD12u:
            // Too complex for 16-bit compile-time folding; don't fold
            *reslo = llo;
            *reshi = lhi;
            return;
        default:
            // delegate to signed version for non-comparison ops
            CalcConst32(lhi, llo, oper, rhi, rlo, reshi, reslo);
            return;
    }
}

// Signed 32-bit comparison folding.
void CalcCmp32(int lhi, int llo, int oper, int rhi, int rlo,
          int *reshi, int *reslo) {
    unsigned ullo, urlo;
    ullo = llo; urlo = rlo;
    *reshi = 0;
    switch (oper) {
        case LT12:
            if (lhi < rhi) *reslo = 1;
            else if (lhi > rhi) *reslo = 0;
            else *reslo = (ullo < urlo) ? 1 : 0;
            return;
        case LE12:
            if (lhi < rhi) *reslo = 1;
            else if (lhi > rhi) *reslo = 0;
            else *reslo = (ullo <= urlo) ? 1 : 0;
            return;
        case GT12:
            if (lhi > rhi) *reslo = 1;
            else if (lhi < rhi) *reslo = 0;
            else *reslo = (ullo > urlo) ? 1 : 0;
            return;
        case GE12:
            if (lhi > rhi) *reslo = 1;
            else if (lhi < rhi) *reslo = 0;
            else *reslo = (ullo >= urlo) ? 1 : 0;
            return;
        default:
            *reslo = 0;
            return;
    }
}
