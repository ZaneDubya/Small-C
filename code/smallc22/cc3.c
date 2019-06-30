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

#include <stdio.h>
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

#define SYMTAB_ADR 0    // is[SYMTAB_ADR] contains the address of the symbol
                        // table entry that describes the operand. Information
                        // from the symbol table is used when generating code
                        // that access the operand. Constants are not included
                        // in the symbol table, and thus this is zero. Only the
                        // primary operand has an address in the symbol table,
                        // thus when the primary operand combines into a larger
                        // entity, this value is reset to zero.
                        
#define TYP_OBJ 1       // is[TYP_OBJ] contains the data type of an indirectly
                        // referenced object; these are referenced by way of an
                        // address in a register. This includes (1) function
                        // arguments, (2) local objects, and (3) globally
                        // declared arrays. In the first two cases, the address
                        // is calculated relative to BP, the stack frame ptr.
                        // In the third case an array element's address is
                        // calculated relative to the label which identifies
                        // the array. Static objects other than arrays have a
                        // zero in this element because they are directly
                        // reference by their label. In some cases (array names
                        // without subscripts or leading ampersand), we only 
                        // need the address, so this element does not matter.
                        
#define TYP_ADR 2       // is[TYP_ADR] contains the data type of an address
                        // (pointer, array, &variable); otherwise, zero.
                        
#define TYP_CNST 3      // is[TYP_CNST] contains the data type (INT or UINT) if
                        // the (sub)expression is a constant value; otherwise,
                        // zero. The unsigned designation applies only to
                        // values over 32767 that are written without a sign.
                        
#define VAL_CNST 4      // is[VAL_CNST] contains the value produced by a
                        // constant (sub)expression. It is used for other
                        // purposes when the (sub)expression is not constant.
                        
#define LAST_OP 5       // is[LAST_OP] contains the p-code that generates the
                        // highest binary operator in an expression. Example:
                        // for the expression "left oper zero" (where left is
                        // the left subexpression, oper is a binary operator,
                        // and zero is a subexpression which evaluates to zero)
                        // which form of optimized code to generate. This
                        // element is used by test(). 
                        
#define STG_ADR 6       // is[STG_ADR] contains the staging buffer address
                        // where code that evaluates the oper zero part of an
                        // expression of the form "left oper zero" is stored.
                        // If not zero, it tells test() that better code can be
                        // generated and how much of the end of the staging
                        // buffer to replace.

extern char *litq, *glbptr, *locptr, *lptr, ssname[NAMESIZE];
extern int ch, csp, litlab, litptr, nch, op[16], op2[16], 
    opindex, opsize, *snext;
    
// ****************************************************************************
// lead-in functions
// ****************************************************************************

IsConstExpr(int *val) {
    int const;
    int *before, *start;
    setstage(&before, &start);
    ParseExpression(&const, val);
    ClearStage(before, 0);     /* scratch generated code */
    if (const == 0) {
        error("must be constant expression");
    }
    return const;
}

ParseExpression(int *con, int *val) {
    int is[7], savedlocptr;
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
    int is[7];
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
    if (is[TYP_CNST]) {             /* constant expression */
        ClearStage(before, 0);
        if (is[VAL_CNST]) {
            return;
        }
        gen(JMPm, label);
        return;
    }
    if (is[STG_ADR]) {             /* stage address of "oper 0" code */
        switch (is[LAST_OP]) {       /* operator code */
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
    ClearStage(is[STG_ADR], 0);       /* purge conventional code */
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
    int k, is2[7], is3[2], oper, oper2;
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
    if (is[TYP_OBJ]) {                          /* indirect target */
        if (oper) {                             /* ?= */
            gen(PUSH1, 0);                      /* save address */
            fetch(is);                          /* fetch left side */
        }
        down2(oper, oper2, level1, is, is2);    /* parse right side */
        if (oper)
            gen(POP2, 0);                       /* retrieve address */
    }
    else {                                      /* direct target */
        if (oper) {                             /* ?= */
            fetch(is);                          /* fetch left side */
            down2(oper, oper2, level1, is, is2);/* parse right side */
        }
        else {                                  /*  = */
            if (level1(is2))
                fetch(is2);                     /* parse right side */
        }
    }
    store(is3);                                 /* store result */
    return 0;
}

// level2() parses the ternary operator Expression1 ? Expression2 : Expression3
level2(int is1[]) {
    int is2[7], is3[7], k, flab, endlab, *before, *after;
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
        fetch(is3);        /* expression 3 */
    }
    else if (is3[TYP_CNST]) {
        gen(GETw1n, is3[VAL_CNST]);
    }
    gen(LABm, endlab);
    is1[TYP_CNST] = is1[VAL_CNST] = 0;
    if (is2[TYP_CNST] && is3[TYP_CNST]) {                  /* expr1 ? const2 : const3 */
        is1[TYP_ADR] = is1[TYP_OBJ] = is1[STG_ADR] = 0;
    }
    else if (is3[TYP_CNST]) {                        /* expr1 ? var2 : const3 */
        is1[TYP_ADR] = is2[TYP_ADR];
        is1[TYP_OBJ] = is2[TYP_OBJ];
        is1[STG_ADR] = is2[STG_ADR];
    }
    else if ((is2[TYP_CNST])                         /* expr1 ? const2 : var3 */
        || (is2[TYP_ADR] == is3[TYP_ADR])) {           /* expr1 ? same2 : same3 */
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
    if (IsMatch("++")) {                 /* ++lval */
        if (level13(is) == 0) {
            needlval();
            return 0;
        }
        step(rINC1, is, 0);
        return 0;
    }
    else if (IsMatch("--")) {            /* --lval */
        if (level13(is) == 0) {
            needlval();
            return 0;
        }
        step(rDEC1, is, 0);
        return 0;
    }
    else if (IsMatch("~")) {             /* ~ */
        if (level13(is)) fetch(is);
        gen(COM1, 0);
        is[VAL_CNST] = ~is[VAL_CNST];
        return (is[STG_ADR] = 0);
    }
    else if (IsMatch("!")) {             /* ! */
        if (level13(is)) fetch(is);
        gen(LNEG1, 0);
        is[VAL_CNST] = !is[VAL_CNST];
        return (is[STG_ADR] = 0);
    }
    else if (IsMatch("-")) {             /* unary - */
        if (level13(is)) fetch(is);
        gen(ANEG1, 0);
        is[VAL_CNST] = -is[VAL_CNST];
        return (is[STG_ADR] = 0);
    }
    else if (IsMatch("*")) {             /* unary * */
        if (level13(is)) fetch(is);
        if (ptr = is[SYMTAB_ADR])
            is[TYP_OBJ] = ptr[TYPE];
        else
            is[TYP_OBJ] = TYPE_INT;
        is[STG_ADR] = is[TYP_ADR] = is[TYP_CNST] = 0; // no op0 stage addr, not addr or const
        is[VAL_CNST] = 1;                   /* omit fetch() on func call */
        return 1;
    }
    else if (amatch("sizeof", 6)) {    /* sizeof() */
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
    else if (IsMatch("&")) {             /* unary & */
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
        if (IsMatch("++")) {               /* lval++ */
            if (k == 0) {
                needlval();
                return 0;
            }
            step(rINC1, is, rDEC1);
            return 0;
        }
        else if (IsMatch("--")) {          /* lval-- */
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
    int k, const, val;
    char *ptr, *before, *start;
    k = primary(is);
    ptr = is[SYMTAB_ADR];
    blanks();
    if (IsMatch(".")) {
        // Recognize the struct dot operator:
        // structs are proving very difficult to implement. I'm leaving it
        // here for now. I already recognize the dot operator and get the
        // information of the referenced member. What's left to do is update
        // is[] with the information of the member, and (possibly) create a
        // new symbol table reference for the member. The issue is that this
        // symbol table entry should be temporary; it should be removed as
        // soon as this line is complete.
        if (is[TYP_OBJ] == TYPE_STRUCT) {
            // Get the information of the referenced member:
            char memName[NAMESIZE], *member;
            if (!symname(memName)) {
                error("No valid name following struct member operator");
                return 0;
            }
            member = getStructMember(getint(ptr + CLASSPTR, 2), memName);
            if (member == 0) {
                error("Not a valid member of this struct");
                return 0;
            }
            printIsDebug(is);
            printSymTabDebug(ptr);
            // Add the member information to the local symbol table (temporary, 
            // will be removed at the end of ParseExpression().
            // Update is[] with information of the member
            is[SYMTAB_ADR] = AddSymbol("tmpStrMem", // "temp struct member"
                member[STRMEM_IDENT], // ident
                member[STRMEM_TYPE],  // type
                getint(member + STRMEM_SIZE, 2), // size 
                getint(member + STRMEM_OFFSET, 2), // offset
                &locptr, AUTOMATIC);
            is[TYP_OBJ] = member[STRMEM_TYPE];
            is[TYP_ADR] = 0;
                        // is[TYP_ADR] contains the data type of an address
                        // (pointer, array, &variable); otherwise, zero.
            // !!!
            is[TYP_CNST] = is[VAL_CNST] = is[LAST_OP] = is[STG_ADR] = 0;
            // note: can we handle nested structs?
            // ----
            // Handle offset to member
            // setstage(&before, &start);
            // ClearStage(before, 0);
            // gen(GETw2n, is2[VAL_CNST]);
            // gen(ADD12, 0);
            // ---
            printf("  looks good!\n");
        }
        else {
            // must account for arrays and ptrs
            error("can't use struct member operator on non-struct variables");
        }
    }
    else if (ch == '[' || ch == '(') {
        int is2[7];                     /* allocate only if needed */
        while (1) {
            if (IsMatch("[")) {
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
                if (is2[TYP_CNST]) {
                    ClearStage(before, 0);
                    if (is2[VAL_CNST]) {             /* only add if non-zero */
                        if (ptr[TYPE] >> 2 == BPW) {
                            gen(GETw2n, is2[VAL_CNST] << LBPW);
                        }
                        else {
                            gen(GETw2n, is2[VAL_CNST]);
                        }
                        gen(ADD12, 0);
                    }
                }
                else {
                    if (ptr[TYPE] >> 2 == BPW) {
                        gen(DBL1, 0);
                    }
                    gen(ADD12, 0);
                }
                is[TYP_ADR] = 0;
                is[TYP_OBJ] = ptr[TYPE];
                k = 1;
            }
            else if (IsMatch("(")) {         /* function(...) */
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
                k = is[SYMTAB_ADR] = is[TYP_CNST] = is[VAL_CNST] = 0;
            }
            else {
                return k;
            }
        }
    }
    if (ptr && ptr[IDENT] == IDENT_FUNCTION) {
        gen(POINT1m, ptr);
        is[SYMTAB_ADR] = 0;
        return 0;
    }
    return k;
}

primary(int *is) {
    char *ptr, sname[NAMESIZE];
    int k;
    if (IsMatch("(")) {                  /* (subexpression) */
        do {
            k = level1(is); 
        } while (IsMatch(","));
        Require(")");
        return k;
    }
    putint(0, is, 7 << LBPW);         /* clear "is" array */
    if (symname(sname)) {              /* is legal symbol */
        if (ptr = findloc(sname)) {      /* is local */
            if (ptr[IDENT] == IDENT_LABEL) {
                experr();
                return 0;
            }
            gen(POINT1s, getint(ptr + OFFSET, 2));
            is[SYMTAB_ADR] = ptr;
            is[TYP_OBJ] = ptr[TYPE];
            if (ptr[IDENT] == IDENT_ARRAY) {
                is[TYP_ADR] = ptr[TYPE];
                return 0;
            }
            if (ptr[IDENT] == IDENT_POINTER) {
                is[TYP_OBJ] = TYPE_UINT;
                is[TYP_ADR] = ptr[TYPE];
            }
            return 1;
        }
        if (ptr = findglb(sname)) {      /* is global */
            is[SYMTAB_ADR] = ptr;
            if (ptr[IDENT] != IDENT_FUNCTION) {
                if (ptr[IDENT] == IDENT_ARRAY) {
                    gen(POINT1m, ptr);
                    is[TYP_OBJ] = is[TYP_ADR] = ptr[TYPE];
                    return 0;
                }
                if (ptr[IDENT] == IDENT_POINTER) {
                    is[TYP_ADR] = ptr[TYPE];
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

callfunc(char *ptr) {      /* symbol table entry or 0 */
    int nargs, const, val;
    nargs = 0;
    blanks();                      /* already saw open paren */
    while (streq(lptr, ")") == 0) {
        if (endst()) {
            break;
        }
        if (ptr) {
            ParseExpression(&const, &val);
            gen(PUSH1, 0);
        }
        else {
            gen(PUSH1, 0);
            ParseExpression(&const, &val);
            gen(SWAP1s, 0);            /* don't push addr */
        }
        nargs = nargs + BPW;         /* count args*BPW */
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
}

/*
** true if is2's operand should be doubled
*/
double(int oper, int is1[], int is2[]) {
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
    if (is[TYP_OBJ]) {                    /* putstk */
        if (is[TYP_OBJ] >> 2 == 1) {
            gen(PUTbp1, 0);
        }
        else {
            gen(PUTwp1, 0);
        }
    }
    else {                          /* putmem */
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
    if (is[TYP_OBJ]) {                                   /* indirect */
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
    else {                                         /* direct */
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
            return  9;  /* HT */
        case 'b': gch();
            return  8;  /* BS */
        case 'f': gch();
            return 12;  /* FF */
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

/***************** pipeline functions ******************/

/*
** skim over terms adjoining || and && operators
*/
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

/*
** test for early dropout from || or && sequences
*/
dropout(int k, int tcode, int exit1, int is[]) {
    if (k)
        fetch(is);
    else if (is[TYP_CNST])
        gen(GETw1n, is[VAL_CNST]);
    gen(tcode, exit1);          /* jumps on false */
}

/*
** drop to a lower level
*/
down(char *opstr, int opoff, int (*level)(), int is[]) {
    int k;
    k = down1(level, is);
    if (nextop(opstr) == 0)
        return k;
    if (k)
        fetch(is);
    while (1) {
        if (nextop(opstr)) {
            int is2[7];     /* allocate only if needed */
            bump(opsize);
            opindex += opoff;
            down2(op[opindex], op2[opindex], level, is, is2);
        }
        else return 0;
    }
}

/*
** unary drop to a lower level
*/
down1(int (*level)(), int is[]) {
    int k, *before, *start;
    setstage(&before, &start);
    k = (*level)(is);
    if (is[TYP_CNST])  {
        ClearStage(before, 0);  /* load constant later */
    }
    return k;
}

/*
** binary drop to a lower level
*/
down2(int oper, int oper2, int (*level)(), int is[], int is2[]) {
    int *before, *start;
    char *ptr;
    setstage(&before, &start);
    is[STG_ADR] = 0;                             /* not "... op 0" syntax */
    if (is[TYP_CNST]) {                           /* consant op unknown */
        if (down1(level, is2))
            fetch(is2);
        if (is[VAL_CNST] == 0)
            is[STG_ADR] = snext;
        gen(GETw2n, is[VAL_CNST] << double(oper, is2, is));
    }
    else {                                  /* variable op unknown */
        gen(PUSH1, 0);                      /* at start in the buffer */
        if (down1(level, is2)) fetch(is2);
        if (is2[TYP_CNST]) {                      /* variable op constant */
            if (is2[VAL_CNST] == 0) is[STG_ADR] = start;
            csp += BPW;                     /* adjust stack and */
            ClearStage(before, 0);          /* discard the PUSH */
            if (oper == ADD12) {            /* commutative */
                gen(GETw2n, is2[VAL_CNST] << double(oper, is, is2));
            }
            else {                          /* non-commutative */
                gen(MOVE21, 0);
                gen(GETw1n, is2[VAL_CNST] << double(oper, is, is2));
            }
        }
        else {                              /* variable op variable */
            gen(POP2, 0);
            if (double(oper, is, is2)) gen(DBL1, 0);
            if (double(oper, is2, is)) gen(DBL2, 0);
        }
    }
    if (oper) {
        if (IsUnsigned(is) || IsUnsigned(is2)) {
            oper = oper2;
        }
        if (is[TYP_CNST] = is[TYP_CNST] & is2[TYP_CNST]) {               /* constant result */
            is[VAL_CNST] = CalcConst(is[VAL_CNST], oper, is2[VAL_CNST]);
            ClearStage(before, 0);
            if (is2[TYP_CNST] == TYPE_UINT) {
                is[TYP_CNST] = TYPE_UINT;
            }
        }
        else {
            /* variable result */
            gen(oper, 0);
            if (oper == SUB12 && is[TYP_ADR] >> 2 == BPW && is2[TYP_ADR] >> 2 == BPW) { 
                /* difference of two word addresses */
                gen(SWAP12, 0);
                gen(GETw1n, 1);
                gen(ASR12, 0);          /* div by 2 */
            }
            /* identify the operator */
            is[LAST_OP] = oper;
        }
        if (oper == SUB12 || oper == ADD12) {
            if (is[TYP_ADR] && is2[TYP_ADR]) {
                /*  addr +/- addr */
                is[TYP_ADR] = 0;
            }
            else if (is2[TYP_ADR]) {
                /* value +/- addr */
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

/*
** unsigned operand?
*/
IsUnsigned(int is[]) {
    char *ptr;
    if (is[TYP_ADR] || is[TYP_CNST] == TYPE_UINT || 
        ((ptr = is[SYMTAB_ADR]) && (ptr[TYPE] & IS_UNSIGNED)))
        return 1;
    return 0;
}

/*
** calculate signed constant result
*/
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

/*
** calculate unsigned constant result
*/
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
    
// ****************************************************************************
// debug functions
// ****************************************************************************

printIsDebug(int *is) {
    printf("  IsInfo st=0x%x to=%u ta=%u tc=%u vc=%u lo=%u sa=%u\n",
        is[0], is[1], is[2], is[3], is[4], is[5], is[6]);
}

printSymTabDebug(char *ptr) {
    printf("  SymTab id=%u typ=%u cls=%u &cl=0x%x sz=%u ofs=%d\n",
        ptr[IDENT], ptr[TYPE],  ptr[CLASS], 
        getint(ptr + CLASSPTR, 2), 
        getint(ptr + SIZE, 2), 
        getint(ptr + OFFSET, 2));
}
