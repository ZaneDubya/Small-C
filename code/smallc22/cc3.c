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
    ssname[NAMESIZE];
extern int ch, csp, litlab, litptr, nch, op[16], op2[16], 
    opindex, opsize, *snext, argtop, rettype, rettypeSubPtr,
    lastNdim, lastStrides[MAX_DIMS];
    
// ****************************************************************************
// lead-in functions
// ****************************************************************************

IsConstExpr(int *val) {
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

ParseExpression(int *con, int *val) {
    int is[8], savedlocptr;
    // Referenced struct member info is stored in the local symbol table,
    // thus we must restore the local symbol table ptr after each expression.
    savedlocptr = locptr;
    if (level1(is)) {
        fetch(is);
    }
    *con = is[TYP_CNST];
    *val = is[VAL_CNST];
    locptr = savedlocptr;
}

GenTestAndJmp(int label, int reqParens) {
    int is[8];
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
        if (is[VAL_CNST]) {
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
                gen(NE10f, label);          
                break;
        }
    }
    else {
        gen(NE10f, label);
    }
    ClearStage(before, start);
}

// test primary register against zero and generate a jump instruction if false
GenJmpIfZero(int oper, int label, int is[]) {
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
level1(int is[]) {
    int k, is2[8], is3[2], oper, oper2;
    char *lhsPtr, *rhsPtr;
    int isStructLhs, savedcsp, structSize;
    char *cpyfn;
    k = down1(level2, is);
    if (is[TYP_CNST]) {
        gen(GETw1n, is[VAL_CNST]);
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
        oper = oper2 = ASR12;
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
    is3[SYMTAB_ADR] = is[SYMTAB_ADR];
    is3[TYP_OBJ] = is[TYP_OBJ];
    // Struct-to-struct deep copy (simple = only)
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
                // AX = &src.  Build contiguous structcp(dst,src,n)
                // args so callfunc's return buffer can't separate
                // &dst (saved above) from the rest.
                // N.B. use PUSH1 (not PUSH2) so gen() tracks csp.
                gen(MOVE21, 0);
                gen(GETw1s, savedcsp - BPW);
                gen(PUSH1, 0);
                gen(SWAP12, 0);
                gen(PUSH1, 0);
                gen(GETw1n, structSize);
                gen(PUSH1, 0);
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
                // AX = &src.  Build structcp(dst, src, n) args.
                gen(PUSH1, 0);
                gen(POINT1m, lhsPtr);
                gen(SWAP1s, 0);
                gen(PUSH1, 0);
                gen(GETw1n, structSize);
                gen(PUSH1, 0);
            }
            cpyfn = findglb("structcp");
            if (cpyfn == 0) {
                cpyfn = AddSymbol("structcp", IDENT_FUNCTION, TYPE_INT,
                    0, 0, &glbptr, AUTOEXT);
            }
            gen(ARGCNTn, 3);
            gen(CALLm, cpyfn);
            gen(ADDSP, savedcsp);
            return 0;
        }
    }
    if (is[TYP_OBJ]) {                          // indirect target
        if (oper) {                             // ?=
            gen(PUSH1, 0);                      // save address
            fetch(is);                          // fetch left side
        }
        down2(oper, oper2, level1, is, is2);    // parse right side
        if (oper)
            gen(POP2, 0);                       // retrieve address
    }
    else {                                      // direct target
        if (oper) {                             // ?=
            fetch(is);                          // fetch left side
            down2(oper, oper2, level1, is, is2);// parse right side
        }
        else {                                  //  =
            if (level1(is2))
                fetch(is2);                     // parse right side
        }
    }
    store(is3);                                 // store result
    return 0;
}

// level2() parses the ternary operator Expression1 ? Expression2 : Expression3
level2(int is1[]) {
    int is2[8], is3[8], k, flab, endlab, *before, *after;
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
        gen(GETw1n, is2[VAL_CNST]); // constant value is loaded directly.
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

level3(int is[]) {
    return skim("||", EQ10f, 1, 0, level4, is); 
}

level4(int is[]) {
    return skim("&&", NE10f, 0, 1, level5, is); 
}

level5(int is[]) {
    return down("|", 0, level6, is); 
}

level6(int is[]) {
    return down("^", 1, level7, is); 
}

level7(int is[]) {
    return down("&", 2, level8, is); 
}

level8(int is[]) {
    return down("== !=", 3, level9, is); 
}

level9(int is[]) {
    return down("<= >= < >", 5, level10, is);
}

level10(int is[]) {
    return down(">> <<", 9, level11, is); 
}

level11(int is[]) {
    return down("+ -", 11, level12, is); 
}

level12(int is[]) {
    return down("* / %", 13, level13, is); 
}

// level13() parses the unary operators: ++, --, ~, !, -, *, &, and sizeof().
level13(int is[]) {
    int k;
    char *ptr;
    if (IsMatch("++")) {                 // ++lval
        if (level13(is) == 0) {
            needlval();
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
        step(rDEC1, is, 0);
        return 0;
    }
    else if (IsMatch("~")) {             // ~
        if (level13(is)) fetch(is);
        gen(COM1, 0);
        is[VAL_CNST] = ~is[VAL_CNST];
        return (is[STG_ADR] = 0);
    }
    else if (IsMatch("!")) {             // !
        if (level13(is)) fetch(is);
        gen(LNEG1, 0);
        is[VAL_CNST] = !is[VAL_CNST];
        return (is[STG_ADR] = 0);
    }
    else if (IsMatch("-")) {             // unary -
        if (level13(is)) fetch(is);
        gen(ANEG1, 0);
        is[VAL_CNST] = -is[VAL_CNST];
        return (is[STG_ADR] = 0);
    }
    else if (IsMatch("*")) {             // unary *
        if (level13(is)) fetch(is);
        if (ptr = is[SYMTAB_ADR]) {
            if (ptr[IDENT] == IDENT_PTR_ARRAY)
                is[TYP_OBJ] = TYPE_UINT;
            else
                is[TYP_OBJ] = ptr[TYPE];
        }
        else
            is[TYP_OBJ] = TYPE_INT;
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
        if (amatch("unsigned", 8)) {
            sz = BPW;
        }
        if (amatch("int", 3)) {
            sz = BPW;
        }
        else if (amatch("char", 4)) {
            sz = 1;
        }
        if (sz) { 
            if (IsMatch("*")) {      
                sz = BPW; 
            }
        }
        else if (amatch("struct", 6)) {
            char *sp;
            sp = getStructPtr();
            if (sp == -1) error("unknown struct name");
            else sz = getStructSize(sp);
        }
        else if (symname(sname) &&  
            ((ptr = findloc(sname)) || (ptr = findglb(sname))) &&
            ptr[IDENT] != IDENT_FUNCTION && ptr[IDENT] != IDENT_LABEL) {
            sz = getint(ptr + SIZE, 2);
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
            return 0;
        }
        gen(POINT1m, ptr);
        is[TYP_OBJ] = ptr[TYPE];
        return 0;
    }
    else {
        k = level14(is);
        if (IsMatch("++")) {               // lval++
            if (k == 0) {
                needlval();
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
level14(int *is) {
    int k, val;
    char *ptr, *before, *start;
    int is2[8];
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
                skip();
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
                    if (is2[TYP_CNST]) {
                        ClearStage(before, 0);
                        if (is2[VAL_CNST]) {
                            if (ptr[TYPE] == TYPE_STRUCT) {
                                gen(GETw2n, is2[VAL_CNST]
                                    * getStructSize(
                                        getClsPtr(ptr)));
                            }
                            else if (ptr[TYPE] >> 2 == BPW) {
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
                        if (ptr[TYPE] == TYPE_STRUCT) {
                            gen(PUSH2, 0);
                            gen(GETw2n, getStructSize(
                                getClsPtr(ptr)));
                            gen(MUL12, 0);
                            gen(POP2, 0);
                        }
                        else if (ptr[TYPE] >> 2 == BPW) {
                            gen(DBL1, 0);
                        }
                        gen(ADD12, 0);
                    }
                    is[DIM_LEFT] = 0;
                    is[TYP_ADR] = 0;
                    is[TYP_OBJ] = ptr[TYPE];
                    k = 1;
                }
            }
        }
        else if (IsMatch("(")) {         // function(...)
            if (ptr == 0) {
                callfunc(0);
            }
            else if (ptr[IDENT] != IDENT_FUNCTION) {
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
            else {
                k = is[SYMTAB_ADR] = is[TYP_CNST] = is[VAL_CNST] = 0;
            }
        }
        else {
            break;
        }
        ptr = is[SYMTAB_ADR];
    }
    if (ptr && ptr[IDENT] == IDENT_FUNCTION) {
        gen(POINT1m, ptr);
        is[SYMTAB_ADR] = 0;
        return 0;
    }
    return k;
}

// Handle struct member lookup and address arithmetic for . and -> operators.
// Assumes AX already contains the struct base address.
// Returns 1 (result is an lvalue).
structmember(char *ptr, int *is) {
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

primary(int *is) {
    char *ptr, sname[NAMESIZE];
    int k;
    if (IsMatch("(")) {                  // (subexpression)
        do {
            k = level1(is); 
        } while (IsMatch(","));
        Require(")");
        return k;
    }
    putint(0, is, 8 << LBPW);         // clear "is" array
    if (symname(sname)) {              // is legal symbol
        if (ptr = findloc(sname)) {      // is local
            if (ptr[IDENT] == IDENT_LABEL) {
                experr();
                return 0;
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
            if (ptr[IDENT] != IDENT_FUNCTION) {
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

experr() {
    error("invalid expression");
    gen(GETw1n, 0);
    skip();
}

callfunc(char *ptr) {      // symbol table entry or 0
    int nargs, is[8];
    int savedlocptr, retStructSize;
    nargs = 0;
    retStructSize = 0;
    blanks();                      // already saw open paren
    // For struct-returning functions, allocate return buffer
    // and push hidden first arg before user args.
    if (ptr && ptr[TYPE] == TYPE_STRUCT) {
        retStructSize = getStructSize(getClsPtr(ptr));
        gen(ADDSP, csp - retStructSize);
        gen(POINT1s, csp);
        gen(PUSH1, 0);
        nargs += BPW;
    }
    while (streq(lptr, ")") == 0) {
        if (endst()) {
            break;
        }
        savedlocptr = locptr;
        if (ptr) {
            if (level1(is)) {
                char *argptr;
                if (is[TYP_OBJ] == TYPE_STRUCT) {
                    // struct lvalue: pass address, not value
                }
                else if (is[TYP_OBJ] == 0 && (argptr = is[SYMTAB_ADR])
                         && argptr[TYPE] == TYPE_STRUCT
                         && argptr[IDENT] != IDENT_FUNCTION) {
                    gen(POINT1m, argptr);
                }
                else {
                    fetch(is);
                }
            }
            locptr = savedlocptr;
            gen(PUSH1, 0);
        }
        else {
            gen(PUSH1, 0);
            if (level1(is)) {
                char *argptr;
                if (is[TYP_OBJ] == TYPE_STRUCT) {
                    // struct lvalue: pass address
                }
                else if (is[TYP_OBJ] == 0 && (argptr = is[SYMTAB_ADR])
                         && argptr[TYPE] == TYPE_STRUCT
                         && argptr[IDENT] != IDENT_FUNCTION) {
                    gen(POINT1m, argptr);
                }
                else {
                    fetch(is);
                }
            }
            locptr = savedlocptr;
            gen(SWAP1s, 0);            // don't push addr
        }
        nargs = nargs + BPW;         // count args*BPW
        if (IsMatch(",") == 0) {
            break;
        }
    }
    Require(")");
    if (streq(ptr + NAME, "CCARGC") == 0) {
        gen(ARGCNTn, nargs >> LBPW);
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

// true if is2's operand should be doubled
isDouble(int oper, int is1[], int is2[]) {
    if ((oper != ADD12 && oper != SUB12) 
        || (is1[TYP_ADR] >> 2 != BPW) 
        || (is2[TYP_ADR])) {
        return 0;
    }
    return 1;
}

step(int oper, int is[], int oper2) {
    fetch(is);
    gen(oper, is[TYP_ADR] ? (is[TYP_ADR] >> 2) : 1);
    store(is);
    if (oper2) {
        gen(oper2, is[TYP_ADR] ? (is[TYP_ADR] >> 2) : 1);
    }
}

store(int is[]) {
    char *ptr;
    if (is[TYP_OBJ]) {                    // putstk
        if (is[TYP_OBJ] >> 2 == 1) {
            gen(PUTbp1, 0);
        }
        else {
            gen(PUTwp1, 0);
        }
    }
    else {                          // putmem
        ptr = is[SYMTAB_ADR];
        if (ptr[IDENT] != IDENT_POINTER && ptr[TYPE] >> 2 == 1) {
            gen(PUTbm1, ptr);
        }
        else {
            gen(PUTwm1, ptr);
        }
    }
}

fetch(int is[]) {
    char *ptr;
    ptr = is[SYMTAB_ADR];
    if (is[TYP_OBJ]) {                                   // indirect
        if (is[TYP_OBJ] >> 2 == BPW) {
            gen(GETw1p, 0);
        }
        else {
            if (ptr[TYPE] & IS_UNSIGNED)
                gen(GETb1pu, 0);
            else
                gen(GETb1p, 0);
        }
    }
    else {                                         // direct
        if (ptr[IDENT] == IDENT_POINTER || ptr[TYPE] >> 2 == BPW) {
            gen(GETw1m, ptr);
        }
        else {
            if (ptr[TYPE] & IS_UNSIGNED) 
                gen(GETb1mu, ptr);
            else
                gen(GETb1m, ptr);
        }
    }
}

constant(int is[]) {
    int offset;
    if (is[TYP_CNST] = number(is + VAL_CNST)) gen(GETw1n, is[VAL_CNST]);
    else if (is[TYP_CNST] = chrcon(is + VAL_CNST)) gen(GETw1n, is[VAL_CNST]);
    else if (string(&offset))          gen(POINT1l, offset);
    else return 0;
    return 1;
}

number(int *value) {
    int k, minus;
    k = minus = 0;
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
            while (isxdigit(ch)) {
                if (isdigit(ch))
                    k = k * 16 + (inbyte() - '0');
                else 
                    k = k * 16 + 10 + (toupper(inbyte()) - 'A');
            }
        }
        else while (ch >= '0' && ch <= '7') {
            k = k * 8 + (inbyte() - '0');
        }
    }
    else while (isdigit(ch)) {
        k = k * 10 + (inbyte() - '0');
    }
    if (minus) {
        *value = -k;
        return TYPE_INT;
    }
    if ((*value = k) < 0) 
        return TYPE_UINT;
    else
        return TYPE_INT;
}

chrcon(int *value) {
    int k;
    k = 0;
    if (IsMatch("'") == 0) 
        return 0;
    while (ch != '\'') 
        k = (k << 8) + (litchar() & 255);
    gch();
    *value = k;
    return TYPE_INT;
}

string(int *offset) {
    char c;
    if (IsMatch("\"") == 0)
        return 0;
    *offset = litptr;
    while (ch != '"') {
        if (ch == 0)
            break;
        stowlit(litchar(), 1);
    }
    gch();
    litq[litptr++] = 0;
    return 1;
}

stowlit(int value, int size) {
    if ((litptr + size) >= LITMAX) {
        error("literal queue overflow");
        abort(ERRCODE);
    }
    putint(value, litq + litptr, size);
    litptr += size;
}

litchar() {
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
skim(char *opstr, int tcode, int dropval, int endval, int (*level)(), int is[]){
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
dropout(int k, int tcode, int exit1, int is[]) {
    if (k)
        fetch(is);
    else if (is[TYP_CNST])
        gen(GETw1n, is[VAL_CNST]);
    gen(tcode, exit1);          // jumps on false
}

// drop to a lower level
down(char *opstr, int opoff, int (*level)(), int is[]) {
    int k;
    k = down1(level, is);
    if (nextop(opstr) == 0)
        return k;
    if (k)
        fetch(is);
    while (1) {
        if (nextop(opstr)) {
            int is2[8];     // allocate only if needed
            bump(opsize);
            opindex += opoff;
            down2(op[opindex], op2[opindex], level, is, is2);
        }
        else return 0;
    }
}

// unary drop to a lower level
down1(int (*level)(), int is[]) {
    int k, *before, *start;
    setstage(&before, &start);
    k = (*level)(is);
    if (is[TYP_CNST])  {
        ClearStage(before, 0);  // load constant later
    }
    return k;
}

// binary drop to a lower level
down2(int oper, int oper2, int (*level)(), int is[], int is2[]) {
    int *before, *start;
    char *ptr;
    setstage(&before, &start);
    is[STG_ADR] = 0;                             // not "... op 0" syntax
    if (is[TYP_CNST]) {                           // consant op unknown
        if (down1(level, is2))
            fetch(is2);
        if (is[VAL_CNST] == 0)
            is[STG_ADR] = snext;
        gen(GETw2n, is[VAL_CNST] << isDouble(oper, is2, is));
    }
    else {                                  // variable op unknown
        gen(PUSH1, 0);                      // at start in the buffer
        if (down1(level, is2)) fetch(is2);
        if (is2[TYP_CNST]) {                      // variable op constant
            if (is2[VAL_CNST] == 0) is[STG_ADR] = start;
            csp += BPW;                     // adjust stack and
            ClearStage(before, 0);          // discard the PUSH
            if (oper == ADD12) {            // commutative
                gen(GETw2n, is2[VAL_CNST] << isDouble(oper, is, is2));
            }
            else {                          // non-commutative
                gen(MOVE21, 0);
                gen(GETw1n, is2[VAL_CNST] << isDouble(oper, is, is2));
            }
        }
        else {                              // variable op variable
            gen(POP2, 0);
            if (isDouble(oper, is, is2)) gen(DBL1, 0);
            if (isDouble(oper, is2, is)) gen(DBL2, 0);
        }
    }
    if (oper) {
        if (IsUnsigned(is) || IsUnsigned(is2)) {
            oper = oper2;
        }
        if (is[TYP_CNST] = is[TYP_CNST] & is2[TYP_CNST]) {               // constant result
            is[VAL_CNST] = CalcConst(is[VAL_CNST], oper, is2[VAL_CNST]);
            ClearStage(before, 0);
            if (is2[TYP_CNST] == TYPE_UINT) {
                is[TYP_CNST] = TYPE_UINT;
            }
        }
        else {
            // variable result
            gen(oper, 0);
            if (oper == SUB12 && is[TYP_ADR] >> 2 == BPW && is2[TYP_ADR] >> 2 == BPW) { 
                // difference of two word addresses
                gen(SWAP12, 0);
                gen(GETw1n, 1);
                gen(ASR12, 0);          // div by 2
            }
            // identify the operator
            is[LAST_OP] = oper;
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
IsUnsigned(int is[]) {
    char *ptr;
    if (is[TYP_ADR] || is[TYP_CNST] == TYPE_UINT || 
        ((ptr = is[SYMTAB_ADR]) && (ptr[TYPE] & IS_UNSIGNED)))
        return 1;
    return 0;
}

// calculate signed constant result
CalcConst(int left, int oper, int right) {
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
CalcUConst(unsigned left, int oper, unsigned right) {
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
    }
    return (0);
}
