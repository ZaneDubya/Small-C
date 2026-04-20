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
    opindex, opsize, *snext, *slast, *argbuf, argbufcur, argtop, rettype,
    rettypeSubPtr, lastNdim, lastStrides[MAX_DIMS];
#ifdef ENABLE_DIAGNOSTICS
int argbufcurMax;       // high-water mark of argbufcur (int slots)
extern int litptrMax;
#endif

// forward declarations for this file:
void applyResultType(int oper, int *is, int *is2);
void calcCmp32(int lhi, int llo, int oper, int rhi, int rlo, int *reshi, int *reslo);
int calcConst(int left, int oper, int right);
void calcConst32(int lhi, int llo, int oper, int rhi, int rlo, int *reshi, int *reslo);
int calcUConst(unsigned left, int oper, unsigned right);
void calcUConst32(int lhi, int llo, int oper, int rhi, int rlo, int *reshi, int *reslo);
void checkFnArgCountAndTypes(char *ptr, int userArgCount, int *argDepths, char **argSyms);
int chrcon(int *lo, int *hi);
int collectOneArg(int *is, int *argLens, int *argSizes, int idx, int *pNargs);
int constant(int *is);
int down(char *opstr, int opoff, int (*level)(), int *is);
int down1(int (*level)(), int *is);
void down2(int oper, int oper2, int (*level)(), int *is, int *is2);
void dropout(int k, int tcode, int exit1, int *is);
void emitConst16Rhs(int *oper, int *oper2, int *is, int *is2);
void emitConst32Rhs(int oper, int leftLong, int *is, int *is2);
void error(char *msg);
void experr();
void fetchBF(int *is);
void foldConst32(int oper, int leftLong, int rightLong, int *is, int *is2);
void genBFStep(int oper, int *is, int oper2);
void genConstLoad(int *is);
void genJmpIfZero(int oper, int label, int *is);
int isPtrLike(char *ptr);
int level14(int *is);
int litchar();
void negate32(int *hi, int *lo);
int number(int *lo, int *hi);
void prepLongRegs(int leftLong, int rightLong, int *is, int *is2);
void scalePtrOperands(int oper, int *is, int *is2);
void shift32r(int *reshi, int *reslo, int count, int signExt);
int skim(char *opstr, int tcode, int dropval, int endval, int (*level)(), int *is);
void step(int oper, int *is, int oper2);
void storeBF(int *is);
void store(int *is);
int structmember(char *ptr, int *is);
int tryReverse(int *oper, int *oper2, int guard);
void widenPrimary(int *is);

// ****************************************************************************
// lead-in functions
// ****************************************************************************

int isConstExpr(int *val) {
    int isConst;
    int *before, *start;
    setstage(&before, &start);
    parseExpr(&isConst, val);
    clearStage(before, 0);     // scratch generated code
    if (isConst == 0) {
        error("must be constant expression");
    }
    return isConst;
}

void parseExpr32(int *con, int *val, int *val_hi) {
    int is[ISSIZE], savedlocptr;
    savedlocptr = locptr;
    if (level1(is)) fetch(is);
    *con = is[TYP_CNST];
    *val = is[VAL_CNST];
    *val_hi = is[VAL_CNST_HI];
    locptr = savedlocptr;
}

int isConstExpr32(int *val, int *val_hi) {
    int isConst;
    int *before, *start;
    setstage(&before, &start);
    parseExpr32(&isConst, val, val_hi);
    clearStage(before, 0);
    if (isConst == 0)
        error("must be constant expression");
    return isConst;
}

void parseExpr(int *con, int *val) {
    int is[ISSIZE], savedlocptr;
    savedlocptr = locptr;
    if (level1(is)) {
        fetch(is);
    }
    *con = is[TYP_CNST];
    *val = is[VAL_CNST];
    locptr = savedlocptr;
}

// Like parseExpr, but when destIsLong is true and the expression
// evaluates to a 16-bit value, emits WIDENs or WIDENu to extend AX to DX:AX.
// Use this when storing into a long/unsigned-long slot to ensure DX is set.
void parseExprWidened(int *con, int *val, int destIsLong) {
    int is[ISSIZE], savedlocptr;
    savedlocptr = locptr;
    if (level1(is)) fetch(is);
    *con = is[TYP_CNST];
    *val = is[VAL_CNST];
    if (destIsLong && !isLongVal(is))
        widenPrimary(is);   // CWD or XOR DX,DX depending on signedness of is
    locptr = savedlocptr;
}

// Emit p-codes to load a compile-time constant into AX (and DX for longs).
// Caller must guarantee is[TYP_CNST] != 0.
void genConstLoad(int *is) {
    gen(GETw1n, is[VAL_CNST]);
    if (is[TYP_CNST] >> 2 == BPD)
        gen(GETdxn, is[VAL_CNST_HI]);
}

void genTestAndJmp(int label, int reqParens) {
    int is[ISSIZE];
    int *before, *start;
    if (reqParens) {
        require("(");
    }
    while (1) {
        setstage(&before, &start);
        if (level1(is)) {
            fetch(is);
        }
        if (isMatch(",")) {
            clearStage(before, start);
        }
        else {
            break;
        }
    }
    if (reqParens) {
        require(")");
    }
    if (is[TYP_CNST]) {             // constant expression
        clearStage(before, 0);
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
                genJmpIfZero(EQ10f, label, is); 
                break;
            case NE12:
            case GT12u: 
                genJmpIfZero(NE10f, label, is); 
                break;
            case GT12:  
                genJmpIfZero(GT10f, label, is); 
                break;
            case GE12:  
                genJmpIfZero(GE10f, label, is); 
                break;
            case GE12u: 
                clearStage(is[STG_ADR], 0);      
                break;
            case LT12:  
                genJmpIfZero(LT10f, label, is); 
                break;
            case LT12u: 
                genJmpIfZero(JMPm, label, is); 
                break;
            case LE12:  
                genJmpIfZero(LE10f, label, is); 
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
    clearStage(before, start);
}

// test primary register against zero and generate a jump instruction if false
void genJmpIfZero(int oper, int label, int *is) {
    clearStage(is[STG_ADR], 0);       // purge conventional code
    gen(oper, label);
}

// ============================================================================
// Expression Analysis
// Functions level1() through level14() correspond to the precedence levels of
// the expression operators, such that level1() recognizes the operators at the
// lowest precedence level and level14() the highest.
// ============================================================================

// Lazily register structcp as AUTOEXT on first struct assignment, so that
// trailer() emits EXTRN structcp only in files that actually use it.
// Returns the symbol table entry pointer (as int).
int extrnStructCp() {
    char *cpyfn = findglb("structcp");
    if (cpyfn == 0) {
        cpyfn = addSymbol("structcp", IDENT_FUNCTION, TYPE_INT, 0, 0, &glbptr, AUTOEXT);
    } else if ((cpyfn[CLASS] & 0x7F) == PROTOEXT) {
        cpyfn[CLASS] = AUTOEXT;  // ensure trailer() emits EXTRN
    }
    return cpyfn;
}

// Parse and emit code for a struct-to-struct assignment (simple '=' only).
// Returns 1 if a struct LHS was detected and assignment code was emitted
//   (or an error was reported); the caller must then return 0.
// Returns 0 if the LHS is not a struct type; the caller falls through to
//   the scalar assignment paths.
int doStructAssign(int *is) {
    int is2[ISSIZE], k, savedcsp, structSize, computedLHS;
    char *lhsPtr, *rhsPtr;
    // Detect struct LHS -- two representations:
    // (a) is[TYP_OBJ] == TYPE_STRUCT: LHS address already in AX (result of
    //     member access or pointer dereference through a struct pointer).
    // (b) is[TYP_OBJ] == 0, SYMTAB_ADR points to a named struct variable.
    lhsPtr = is[SYMTAB_ADR];
    computedLHS = (is[TYP_OBJ] == TYPE_STRUCT);
    if (!computedLHS) {
        if (is[TYP_OBJ] != 0 || !lhsPtr
            || lhsPtr[TYPE] != TYPE_STRUCT || lhsPtr[IDENT] != IDENT_VARIABLE)
            return 0;   // not a struct LHS
    }
    savedcsp = csp;
    structSize = getStructSize(getClsPtr(lhsPtr));
    // Computed-address LHS: &dst is in AX right now; save it before parsing RHS
    // since the RHS parse may contain function calls that clobber AX.
    if (computedLHS)
        gen(PUSH1, 0);                   // save &dst
    // Parse RHS (shared for both LHS kinds).
    k = level1(is2);
    if (k && is2[TYP_OBJ] == TYPE_STRUCT) {
        // AX = &src, placed there by a struct-returning function call
    } else if (k && is2[TYP_OBJ] == 0
               && (rhsPtr = is2[SYMTAB_ADR])
               && rhsPtr[TYPE] == TYPE_STRUCT) {
        gen(POINT1m, rhsPtr);            // AX = &src (named variable)
    } else {
        error("type mismatch in struct assignment");
        gen(ADDSP, savedcsp);
        return 1;
    }
    // Build structcp(dst, src, n) args right-to-left:
    //   [BP+8] = n,  [BP+6] = &src,  [BP+4] = &dst.
    gen(MOVE21, 0);                      // BX = &src
    gen(GETw1n, structSize);             // AX = n
    gen(PUSH1, 0);                       // push n         ([BP+8])
    gen(SWAP12, 0);                      // AX = &src (from BX)
    gen(PUSH1, 0);                       // push &src      ([BP+6])
    if (computedLHS)
        gen(GETw1s, savedcsp - BPW);     // AX = &dst (from saved stack slot)
    else
        gen(POINT1m, lhsPtr);            // AX = &dst (global label)
    gen(PUSH1, 0);                       // push &dst      ([BP+4])
    gen(CALLm, extrnStructCp());
    gen(ADDSP, savedcsp);
    return 1;
}

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
int level1(int *is) {
    int k, is2[ISSIZE], is3[ISSIZE], oper, oper2;
    char *lhsPtr;
    k = down1(level2, is);
    // This block runs in two contexts:
    // (1) Outermost call: down1() discards stage code for constant sub-expressions
    //     so that down2() can use immediate-mode p-codes.  For bare expressions
    //     (e.g. return 0) that don't go through down2(), we re-emit here.
    // (2) Recursive RHS call (e.g. x = 5): down1() sets TYP_CNST but emits nothing.
    //     The caller won't call fetch() when level1 returns 0, so we must load
    //     the constant into AX now.
    if (is[TYP_CNST])
        genConstLoad(is);
    // check if this is an assignment operator, requiring an lvalue.
    if (isMatch("|=")) {
        oper = oper2 = OR12;
    }
    else if (isMatch("^=")) {
        oper = oper2 = XOR12;
    }
    else if (isMatch("&=")) { 
        oper = oper2 = AND12; 
    }
    else if (isMatch("+=")) { 
        oper = oper2 = ADD12;
    }
    else if (isMatch("-=")) {
        oper = oper2 = SUB12;
    }
    else if (isMatch("*=")) {
        oper = MUL12;
        oper2 = MUL12u;
    }
    else if (isMatch("/=")) {
        oper = DIV12;
        oper2 = DIV12u;
    }
    else if (isMatch("%=")) {
        oper = MOD12;
        oper2 = MOD12u;
    }
    else if (isMatch(">>=")) {
        oper = ASR12;
        oper2 = LSR12;
    }
    else if (isMatch("<<=")) {
        oper = oper2 = ASL12;
    }
    else if (isMatch("=")) {
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
    // Snapshot the LHS descriptor for store() and for the widening checks below.
    // is3 carries only SYMTAB_ADR and TYP_OBJ; the other fields must be zero so
    // that isLongVal(is3) does not false-positive on stale stack data.
    is3[SYMTAB_ADR] = is[SYMTAB_ADR];
    is3[TYP_OBJ]    = is[TYP_OBJ];
    is3[TYP_ADR]    = 0;
    is3[TYP_CNST]   = 0;
    // Snapshot LHS pointer depth for the depth-mismatch check below.
    is3[IS_PTRDEPTH] = is[IS_PTRDEPTH];
    // Snapshot bit-field state so store(is3) has full bit-field info.
    is3[IS_BITFIELD] = is[IS_BITFIELD];
    is3[BF_WIDTH]    = is[BF_WIDTH];
    is3[BF_OFFSET]   = is[BF_OFFSET];
    is3[BF_SIGNED]   = is[BF_SIGNED];
    // Struct-to-struct deep copy -- handled only for simple '='.
    if (oper == 0 && doStructAssign(is)) return 0;
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
                widenPrimary(is2);
#ifdef ENABLE_WARNINGS
            // ptr-depth-mismatch-warning
            if (is3[IS_PTRDEPTH] >= 1 && is2[IS_PTRDEPTH] >= 1
                && is3[IS_PTRDEPTH] != is2[IS_PTRDEPTH]
                && !(is2[TYP_CNST] && is2[VAL_CNST] == 0))
                warning("pointer assignment: depth mismatch");
#endif
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
                widenPrimary(is2);
#ifdef ENABLE_WARNINGS
            // ptr-depth-mismatch-warning
            if (is3[IS_PTRDEPTH] >= 1 && is2[IS_PTRDEPTH] >= 1
                && is3[IS_PTRDEPTH] != is2[IS_PTRDEPTH]
                && !(is2[TYP_CNST] && is2[VAL_CNST] == 0))
                warning("pointer assignment: depth mismatch");
#endif
        }
    }
    store(is3);                                 // store result
    return 0;
}

// level2() parses the ternary operator Expression1 ? Expression2 : Expression3
int level2(int *is1) {
    int is2[ISSIZE], is3[ISSIZE], k, flab, endlab;
    // Call level3() by way of down1() to parse the first expression.
    k = down1(level3, is1);
    // If next token is not ?, this is not a ternary operator, return to caller
    if (isMatch("?") == 0) {
        return k;
    }
    // getlabel() is called to reserve a label number for use in jumping around
    // Expression2, and dropout() is called to generate code to perform that
    // jump if Expression1 is false.
    dropout(k, NE10f, flab = getlabel(), is1);
    // Expression2 is parsed by recursively calling level2() through down1().
    if (down1(level2, is2))
        fetch(is2); // fetch() obtains the expression's value from memory.
    else if (is2[TYP_CNST])
        genConstLoad(is2);
    // Enforce second character of operator (":")
    require(":");
    gen(JMPm, endlab = getlabel());
    gen(LABm, flab);
    if (down1(level2, is3))
        fetch(is3);        // expression 3
    else if (is3[TYP_CNST])
        genConstLoad(is3);
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

int level3(int *is) {
    return skim("||", EQ10f, 1, 0, level4, is); 
}

int level4(int *is) {
    return skim("&&", NE10f, 0, 1, level5, is); 
}

int level5(int *is) {
    return down("|", 0, level6, is); 
}

int level6(int *is) {
    return down("^", 1, level7, is); 
}

int level7(int *is) {
    return down("&", 2, level8, is); 
}

int level8(int *is) {
    return down("== !=", 3, level9, is); 
}

int level9(int *is) {
    return down("<= >= < >", 5, level10, is);
}

int level10(int *is) {
    return down(">> <<", 9, level11, is); 
}

int level11(int *is) {
    return down("+ -", 11, level12, is); 
}

int level12(int *is) {
    return down("* / %", 13, level13, is); 
}

// Guard helper shared by all ++/-- arms.
// k is the lvalue-flag returned by the descent into level13 or level14.
// Returns 1 if modification is allowed; 0 if an error was already emitted.
// Two distinct rejections are possible:
//   - k == 0: the operand is an rvalue (no storage address); needlval() reports it.
//   - CONST_FLAG set: the symbol was declared const; modification is forbidden.
int checkModifiable(int k, int *is) {
    char *ptr;
    if (k == 0) {
        needlval();
        return 0;
    }
    // Look up the symbol entry.  If the CLASS field carries CONST_FLAG the
    // variable was declared const and may not be written through.
    ptr = is[SYMTAB_ADR];
    if (ptr && ptr[CLASS] & CONST_FLAG) {
        error("cannot modify const variable");
        return 0;
    }
    return 1;
}

// Determine the pointed-to element type for a unary * dereference.
// The lookup has three priority tiers so that explicit pointer casts take
// precedence over the symbol's declared type:
//   1. TYP_ADR non-zero: an explicit (type*) cast is in effect; use its target
//      type directly.  This is what makes *(unsigned char*)p fetch a single
//      unsigned byte even when p was declared as int*.
//   2. SYMTAB_ADR non-null: derive the element type from the symbol table entry.
//      IDENT_PTR_ARRAY is a special case: the compiler represents a pointer-
//      to-array as an unsigned pointer, so force TYPE_UINT.
//   3. Fallback: assume TYPE_INT (the default pointed-to type for untyped
//      expressions such as a function return value used immediately as a pointer).
// IMPORTANT: must be called before is[TYP_ADR] and is[SYMTAB_ADR] are cleared.
int resolveDerefType(int *is) {
    char *ptr;
    if (is[TYP_ADR])
        return is[TYP_ADR];
    if (ptr = is[SYMTAB_ADR])
        return (ptr[IDENT] == IDENT_PTR_ARRAY) ? TYPE_UINT : ptr[TYPE];
    return TYPE_INT;
}

// level13() parses all unary prefix operators and the two postfix operators
// (++ and --), which are handled here because their operand is parsed by
// level14 and the token comes after the operand.
// Operator dispatch order:
//   ++expr  --expr          prefix increment / decrement
//   ~expr                   bitwise complement
//   !expr                   logical not (result always 16-bit 0/1)
//   -expr                   arithmetic negation
//   *expr                   pointer dereference
//   sizeof(type-or-name)    compile-time size query
//   &expr                   address-of
//   expr++ expr--           postfix increment / decrement (via level14)
int level13(int *is) {
    int k, dir;
    char *ptr;

    // ---- prefix ++ and -- ----
    // Match the operator first, then parse the operand with a recursive call
    // so that chains like ++*p resolve correctly.  The undo argument to step()
    // is 0 for prefix: the pre-modified value is discarded, not preserved.
    if      (isMatch("++")) dir = rINC1;
    else if (isMatch("--")) dir = rDEC1;
    else                    dir = 0;
    if (dir) {
        if (!checkModifiable(level13(is), is)) return 0;
        step(dir, is, 0);
        return 0;
    }

    else if (isMatch("~")) {             // ~
        // Fetch the operand, choose the 16-bit or 32-bit complement p-code,
        // then fold the constant value in is[] so downstream levels can
        // constant-fold the full expression without emitting code.
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
    else if (isMatch("!")) {             // !
        // Logical not always produces a 16-bit 0 or 1 regardless of operand
        // width, so clear TYP_OBJ and SYMTAB_ADR after the emit.
        if (level13(is)) fetch(is);
        if (isLongVal(is))
            gen(LNEGd1, 0);
        else
            gen(LNEG1, 0);
        is[TYP_OBJ] = 0;
        is[SYMTAB_ADR] = 0;
        // Constant fold: collapse any non-zero hi:lo pair to 0; zero to 1.
        if (is[TYP_CNST]) {
            int wasTrue;
            wasTrue = is[VAL_CNST] || is[VAL_CNST_HI];
            is[TYP_CNST] = TYPE_INT;
            is[VAL_CNST] = !wasTrue;
            is[VAL_CNST_HI] = 0;
        }
        return (is[STG_ADR] = 0);
    }
    else if (isMatch("-")) {             // unary -
        // Two's complement negate.  For 32-bit operands use negate32() which
        // handles carry across the word boundary; for 16-bit just negate lo.
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
    else if (isMatch("*")) {             // unary *
        // Dereference: AX holds a pointer; after this node AX holds the value
        // at that address (or still a pointer if depth > 1).
        if (level13(is)) fetch(is);
        if (is[IS_PTRDEPTH] > 1) {
            // Multi-level: removing one pointer indirection still leaves a pointer.
            // After decrement the result is still an lvalue: AX holds the intermediate
            // pointer value (always word-sized).  The next fetch (outer * or caller)
            // will load the pointer via GETw1p (TYPE_UINT -> BPW), then resolveDerefType
            // handles the final element type when IS_PTRDEPTH reaches 1.
            is[IS_PTRDEPTH]--;
            is[TYP_OBJ] = TYPE_UINT;   // intermediate pointer is always word-sized
            // TYP_ADR stays: it is the ultimate base type of the pointer chain.
            is[STG_ADR] = is[TYP_CNST] = 0;
            is[VAL_CNST] = 1;
            return 1;
        }
        // Final (or only) dereference: existing behaviour.
        // resolveDerefType() inspects is[] before any fields are cleared.
        is[TYP_OBJ] = resolveDerefType(is);
        if (is[TYP_OBJ] == TYPE_VOID) {
            error("cannot dereference void pointer");
            is[TYP_OBJ] = TYPE_INT;   // error recovery
        }
        // Clear address/cast/const fields: the result is a plain value in AX,
        // not a pointer or a constant.  VAL_CNST = 1 tells callfunc() to skip
        // an extra fetch() for immediate function-call results.
        is[STG_ADR] = is[TYP_ADR] = is[TYP_CNST] = 0;
        is[IS_PTRDEPTH] = 0;
        is[VAL_CNST] = 1;
        return 1;
    }
    else if (amatch("sizeof", 6)) {      // sizeof()
        // parseSizeofType() delegates to dotype() for the full keyword
        // grammar and handles the variable-name fallback.  The result is
        // a compile-time TYPE_INT constant; no code is emitted.
        int sz, p;
        if (isMatch("("))
            p = 1;
        else
            p = 0;
        sz = parseSizeofType();
        if (p) require(")");
        is[TYP_CNST] = TYPE_INT;
        is[VAL_CNST] = sz;
        is[TYP_ADR] = is[TYP_OBJ] = is[SYMTAB_ADR] = 0;
        return 0;
    }
    else if (isMatch("&")) {             // unary &
        // Address-of: the operand must be an lvalue (level13 returned 1).
        // Record the pointed-to type in TYP_ADR, then get the address into AX.
        // Two code paths depending on whether the address is already in AX:
        //   - TYP_OBJ non-zero means primary() emitted POINT1s (local variable):
        //     AX already holds the address; just update is[] and return.
        //   - TYP_OBJ zero means primary() deferred the load (global variable):
        //     emit POINT1m to materialise the address now.
        // In both cases TYP_OBJ is cleared so isLongVal() sees a 16-bit pointer,
        // not the 32-bit element type — otherwise a later PUSH would emit PUSHd1
        // (two words) instead of PUSH1 (one word).
        if (level13(is) == 0) {
            error("illegal address");
            return 0;
        }
        if (is[IS_BITFIELD]) {
            error("cannot take address of bit-field");
            return 0;
        }
        ptr = is[SYMTAB_ADR];
        is[TYP_ADR] = ptr[TYPE];
        is[IS_PTRDEPTH]++;
        if (is[TYP_OBJ]) {
            is[TYP_OBJ] = 0;
            return 0;
        }
        gen(POINT1m, ptr);
        return 0;
    }
    else {
        // ---- postfix ++ and -- ----
        // Descend to the next level to parse the operand, then check for a
        // trailing ++ or --.  The undo argument to step() is the opposite
        // direction: step() applies the increment/decrement, stores, then
        // reverses the change in the register so the caller sees the
        // pre-modification value.
        k = level14(is);
        if      (isMatch("++")) dir = rINC1;
        else if (isMatch("--")) dir = rDEC1;
        else                    dir = 0;
        if (dir) {
            if (!checkModifiable(k, is)) return 0;
            step(dir, is, dir == rINC1 ? rDEC1 : rINC1);
            return 0;
        }
        return k;
    }
}

// Reset is[] state after a function call for non-struct, non-pointer-returning
// functions.  Sets TYP_OBJ to the function's return type when long, else 0.
// Returns 0 so callers can write: k = clearIsAfterCall(is, ptr).
int clearIsAfterCall(int *is, char *ptr) {
    is[SYMTAB_ADR] = is[TYP_CNST] = is[VAL_CNST] = 0;
    is[VAL_CNST_HI] = is[TYP_ADR] = is[LAST_OP] = is[STG_ADR] = 0;
    if (ptr && (ptr[TYPE] == TYPE_LONG || ptr[TYPE] == TYPE_ULONG))
        is[TYP_OBJ] = ptr[TYPE];
    else
        is[TYP_OBJ] = 0;
    return 0;
}

// Emit code to add index * stride to the array base address in AX.
// Used for intermediate dimensions of multi-dim arrays where the
// stride is a precomputed table value rather than a simple element size.
// If the index is a compile-time constant the index expression is folded
// away; otherwise a runtime multiply is emitted.  'before' is the
// setstage() snapshot taken before the index expression was parsed.
int addStrideOff(int stride, int *is2, char *before) {
    if (is2[TYP_CNST]) {
        clearStage(before, 0);
        if (is2[VAL_CNST]) {
            gen(GETw2n, is2[VAL_CNST] * stride);
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
    return 0;
}

// Emit code to add index * elemSize to the array base address in AX.
// Used for the final (innermost) subscript dimension.
// elemType: TYPE_xxx constant that determines the per-element byte size.
// ptr:      symbol-table entry for the array (needed for struct size lookup).
// For compile-time constant indexes the multiplication is folded at compile
// time; for runtime indexes shifts (pow-of-2 sizes) or a multiply (struct)
// are emitted.  'before' is the setstage() snapshot taken before the index
// expression was parsed.
int applyElemOffset(int elemType, char *ptr, int *is2, char *before) {
    if (is2[TYP_CNST]) {
        clearStage(before, 0);
        if (is2[VAL_CNST]) {
            if (elemType == TYPE_STRUCT)
                gen(GETw2n, is2[VAL_CNST] * getStructSize(getClsPtr(ptr)));
            else if (elemType >> 2 == BPD)
                gen(GETw2n, is2[VAL_CNST] << LBPD);
            else if (elemType >> 2 == BPW)
                gen(GETw2n, is2[VAL_CNST] << LBPW);
            else
                gen(GETw2n, is2[VAL_CNST]);
            gen(ADD12, 0);
        }
    }
    else {
        if (elemType == TYPE_STRUCT) {
            gen(PUSH2, 0);
            gen(GETw2n, getStructSize(getClsPtr(ptr)));
            gen(MUL12, 0);
            gen(POP2, 0);
        }
        else if (elemType >> 2 == BPD) {
            gen(DBL1, 0);
            gen(DBL1, 0);
        }
        else if (elemType >> 2 == BPW)
            gen(DBL1, 0);
        gen(ADD12, 0);
    }
    return 0;
}

// Level14() is the last (highest) precedence level before primary(). It
// recognizes only two operations, subscripting and calling functions. It also
// handles the case where a function's address is invoked by naming the
// function without a left parenthesis following.
int level14(int *is) {
    int k, val, elemType;
    char *ptr, *before, *start;
    int is2[ISSIZE];
    // k=1: AX holds an lvalue address; caller must fetch() to load the value.
    // k=0: AX already holds the final value or rvalue address.
    k = primary(is);
    ptr = is[SYMTAB_ADR];
    blanks();
    while (1) {
        if (isMatch(".")) {
            // struct member access via dot operator
            if (ptr == 0 || ptr[TYPE] != TYPE_STRUCT) {
                error("dot requires struct");
                return 0;
            }
            // Auto-dereference struct pointers (hidden pointer params, etc.)
            if (ptr[IDENT] == IDENT_POINTER && is[TYP_OBJ] != TYPE_STRUCT) {
                if (k) fetch(is);   // load pointer value; AX = struct base address
            }
            // Get struct base address into AX.
            // Locals: POINT1s was already emitted by primary(); AX = &struct.
            // Globals: primary() deferred; must emit now.
            else if (is[TYP_OBJ] == 0) {
                gen(POINT1m, ptr);
            }
            // else: local struct variable -- POINT1s already put &struct in AX.
            k = structmember(ptr, is);
        }
        else if (isMatch("->")) {
            // struct member access via arrow operator
            if (ptr == 0 || ptr[TYPE] != TYPE_STRUCT) {
                error("-> requires pointer to struct");
                return 0;
            }
            if (ptr[IDENT] == IDENT_VARIABLE) {
                error("use . for struct variables");  // -> needs a pointer, not a plain struct
                return 0;
            }
            // Load pointer value to get struct address into AX.
            if (k) fetch(is);
            k = structmember(ptr, is);
        }
        else if (isMatch("[")) {
            // [subscript]
            if (ptr == 0 && is[TYP_ADR] == 0) {
                error("can't subscript");
                skipToNextToken();
                require("]");
                return 0;
            }
            // TYP_ADR non-zero means the expression has pointer/array character.
            if (is[TYP_ADR]) { 
                if (k) {
                    fetch(is);   // load pointer/array base address into AX
                }
            }
            else { 
                error("can't subscript"); 
                k = 0; 
            }
            // Snapshot the p-code stream; clearStage can rewind to here if the
            // index expression turns out to be a compile-time constant.
            setstage(&before, &start);
            is2[TYP_CNST] = 0;
            down2(0, 0, level1, is2, is2);   // parse subscript expression into *is2
            require("]");
            if (is[DIM_LEFT] > 1) {
                // Intermediate dimension: scale by precomputed stride (not element size).
                // ptr[NDIM]-is[DIM_LEFT] converts dims-remaining to the outer dim index.
                int stride;
                stride = getDimStride(ptr,
                    ptr[NDIM] - is[DIM_LEFT]);
                addStrideOff(stride, is2, before);
                is[DIM_LEFT]--;          // one fewer dimension remains
                is[TYP_ADR] = ptr[TYPE]; // result is still a pointer into the sub-array
                k = 0;                   // AX holds an address; no fetch needed yet
            }
            else {
                // final dimension or 1D: element-size scaling
                if (ptr && ptr[IDENT] == IDENT_PTR_ARRAY) {
                    // Pointer-array element: always word-sized (the stored pointer value).
                    applyElemOffset(TYPE_UINT, ptr, is2, before);
                    is[DIM_LEFT] = 0;
                    is[TYP_OBJ] = TYPE_UINT;   // element is a pointer value (unsigned int)
                    is[TYP_ADR] = ptr[TYPE];   // what that pointer points to
                    is[SYMTAB_ADR] = 0;        // clear: loop-bottom "ptr = is[SYMTAB_ADR]"
                                               // must not re-load the stale array symbol
                    k = 1;                     // AX has element address; fetch needed
                }
                else {
                    // Use cast type if present, else the symbol's declared type.
                    // Exception: if the subscript base is a depth-2+ pointer
                    // (e.g. char**, char***), each stored slot is itself a
                    // pointer = BPW bytes, regardless of the base element type.
                    // Use TYPE_UINT to force BPW scale and BPW-wide fetch;
                    // preserve TYP_ADR so outer derefs know the base type.
                    if (is[IS_PTRDEPTH] >= 2) {
                        applyElemOffset(TYPE_UINT, ptr, is2, before);
                        is[DIM_LEFT] = 0;
                        is[IS_PTRDEPTH]--;      // one pointer level consumed
                        is[TYP_OBJ] = TYPE_UINT; // result is a BPW-wide pointer value
                        // is[TYP_ADR] stays: still the base element type
                    }
                    else {
                        elemType = is[TYP_ADR] ? is[TYP_ADR] : ptr[TYPE];
                        applyElemOffset(elemType, ptr, is2, before);
                        is[DIM_LEFT] = 0;
                        is[TYP_ADR] = 0;           // subscripted result is a scalar, not a pointer
                        is[TYP_OBJ] = elemType;
                    }
                    k = 1;                     // AX has element address; fetch needed
                }
            }
        }
        else if (isMatch("(")) {         // function(...)
            // Three dispatch paths: indirect through value already in AX,
            // indirect through a fetched function-pointer variable, or direct
            // call to a named function.
            if (ptr == 0) {
                callfunc(0);               // unnamed: call address already in AX
            }
            else if (ptr[IDENT] != IDENT_FUNCTION && ptr[IDENT] != IDENT_PTR_FUNCTION) {
                if (k && !is[VAL_CNST]) {
                    fetch(is);             // load the function-pointer value into AX
                }
                if (ptr[IDENT] == IDENT_FNPTR_VAR)
                    callfunc(ptr);         // pass ptr for sig-check; callfunc emits CALL1
                else
                    callfunc(0);           // indirect call through value in AX
            }
            else {
                callfunc(ptr);             // direct call to named function
            }
            if (ptr && ptr[IDENT] == IDENT_FUNCTION
                    && ptr[TYPE] == TYPE_STRUCT) {
                // Struct return: create a scratch local so chained member access
                // works (e.g. getRect().x).  callfunc() stored the result at the
                // top of the stack frame; tmpRetStr gives that slot a symbol entry.
                is[TYP_OBJ] = TYPE_STRUCT;
                is[SYMTAB_ADR] = addSymbol("tmpRetStr",
                    IDENT_VARIABLE, TYPE_STRUCT,
                    getStructSize(getClsPtr(ptr)),
                    0, &locptr, AUTOMATIC);
                putint(getClsPtr(ptr),
                       is[SYMTAB_ADR] + CLASSPTR, 2);  // copy struct class ptr to scratch entry
                is[TYP_ADR] = is[TYP_CNST] = is[VAL_CNST] = 0;
                is[LAST_OP] = is[STG_ADR] = 0;
                k = 1;                     // lvalue: AX has address of the scratch local
            }
            else if (ptr && ptr[IDENT] == IDENT_PTR_FUNCTION) {
                // pointer-returning function: propagate pointer type to caller
                is[TYP_ADR]     = ptr[TYPE];
                is[IS_PTRDEPTH] = ptr[PTRDEPTH] ? ptr[PTRDEPTH] : 1;
                is[TYP_OBJ]    = 0;
                is[TYP_CNST]   = is[VAL_CNST] = is[VAL_CNST_HI] = 0;
                is[LAST_OP]    = is[STG_ADR]  = 0;
                is[SYMTAB_ADR] = 0;
                k = 0;                     // AX holds the returned pointer value
            }
            else {
                k = clearIsAfterCall(is, ptr); // scalar/void return: clear all type state
            }
        }
        else {
            break;
        }
        // Refresh ptr so the next postfix operator sees the updated type info.
        ptr = is[SYMTAB_ADR];
    }
    // Named function used without (): emit its address so it can be stored or passed.
    if (ptr && (ptr[IDENT] == IDENT_FUNCTION || ptr[IDENT] == IDENT_PTR_FUNCTION)) {
        gen(POINT1m, ptr);   // AX = address of the function label
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
    is[SYMTAB_ADR] = addSymbol("tmpStrMem",
        member[STRMEM_IDENT], member[STRMEM_TYPE],
        getint(member + STRMEM_SIZE, 2),
        getint(member + STRMEM_OFFSET, 2),
        &locptr, AUTOMATIC);
    is[TYP_OBJ] = member[STRMEM_TYPE];
    is[TYP_ADR] = 0;
    is[TYP_CNST] = is[VAL_CNST] = is[LAST_OP] = is[STG_ADR] = 0;
    is[IS_BITFIELD] = 0;
    /* Propagate bit-field metadata if this member is a bit-field */
    if (member[STRMEM_BITWIDTH]) {
        is[IS_BITFIELD] = 1;
        is[BF_WIDTH]    = member[STRMEM_BITWIDTH];
        is[BF_OFFSET]   = member[STRMEM_BITOFF];
        is[BF_SIGNED]   = !(member[STRMEM_TYPE] & IS_UNSIGNED);
        /* Result type of a bit-field read is int (signed) or unsigned int */
        is[TYP_OBJ]     = is[BF_SIGNED] ? TYPE_INT : TYPE_UINT;
    }
    /* For nested structs, propagate struct definition pointer */
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
        is[IS_PTRDEPTH] = member[STRMEM_PTRDEPTH];
    }
    return 1;
}

// Apply a type cast to the expression in is[].
// Called after the operand has been parsed and fetched.
// casttype: target TYPE_xxx constant.
void applycast(int casttype, int *is) {
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
        if ((casttype & IS_UNSIGNED) && !isUnsigned(is)) {
            warning("sign-conversion: widening signed value to unsigned long");
        }
#endif
        widenPrimary(is);
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
int trycast(int *is) {
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
        sp = getStructPtr(KIND_STRUCT);
        if (sp == -1) error("unknown struct name");
        casttype = TYPE_STRUCT;
    }
    else if (amatch("union", 5)) {
        char *sp;
        sp = getStructPtr(KIND_UNION);
        if (sp == -1) error("unknown union name");
        casttype = TYPE_STRUCT;
    }

    if (casttype == 0) {
        lptr = saved;
        ch = sc;
        nch = snc;
        return 0;
    }

    if (isMatch("*")) {
        // Pointer cast: (type **)expr or (type *)expr
        // Count all '*' tokens to get the full pointer depth.
        int castDepth;
        castDepth = 1;
        while (isMatch("*")) castDepth++;
        // The value in AX is left unchanged; only expression metadata is
        // updated to reflect "this is a pointer to casttype".
        require(")");
        if (level13(is)) fetch(is);
        is[TYP_ADR]     = casttype;
        is[IS_PTRDEPTH] = castDepth;
        is[TYP_OBJ]     = 0;
        is[TYP_VAL]     = 0;
        is[TYP_CNST]    = 0;
        return 1;
    }

    require(")");

    if (level13(is)) fetch(is);

    applycast(casttype, is);
    return 1;
}

int primary(int *is) {
    char *ptr, sname[NAMESIZE];
    int k;
    if (isMatch("(")) {                  // (cast) or (subexpression)
        if (trycast(is)) return 0;
        do {
            k = level1(is); 
        } while (isMatch(","));
        require(")");
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
                if (isPtrLike(ptr)) {
                    is[TYP_ADR] = ptr[TYPE];
                    is[IS_PTRDEPTH] = ptr[PTRDEPTH];
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
            if (isPtrLike(ptr)) {
                is[TYP_OBJ] = TYPE_UINT;
                is[TYP_ADR] = ptr[TYPE];
                is[IS_PTRDEPTH] = ptr[PTRDEPTH];
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
                if (isPtrLike(ptr)) {
                    is[TYP_ADR] = ptr[TYPE];
                    is[IS_PTRDEPTH] = ptr[PTRDEPTH];
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
            is[SYMTAB_ADR] = addSymbol(sname, IDENT_FUNCTION, TYPE_INT, 
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

// True when ptr's IDENT marks it as a pointer-like lvalue
// (plain pointer variable or function-pointer variable).
int isPtrLike(char *ptr) {
    int id;
    id = ptr[IDENT];
    return id == IDENT_POINTER || id == IDENT_FNPTR_VAR;
}

// Phase D helper: check caller's argument count against prototype.
// ptr   — symbol table entry for the callee
// userArgCount — number of user-visible arguments supplied at this call site
//                (does NOT include the hidden struct-return pointer)
// argSyms      — symbol table entry for each actual argument (SYMTAB_ADR),
//                or NULL when the argument is not a simple named symbol
void checkFnArgCountAndTypes(char *ptr, int userArgCount, int *argDepths, char **argSyms) {
    int nbyte, isVariadic, nfixed;
    int sigBase;    /* index into paramTypes[] where nparams_byte lives */

    /* Select the right base depending on symbol kind. */
    if (ptr[IDENT] == IDENT_FNPTR_VAR) {
        /* FNPTRPARAMPTR (= CLASSPTR) holds the sub-sig index. */
        /* Sub-sig layout: [retType][nparams_byte][pairs...] */
        sigBase = getint(ptr + FNPTRPARAMPTR, 2);
        if (sigBase == 0) return;  /* no sub-sig recorded */
        sigBase++;                 /* skip retType; nparams_byte is at +1 */
    } else {
        /* Regular function: FNPARAMPTR (= SIZE) holds the proto index. */
        sigBase = getint(ptr + FNPARAMPTR, 2);
        if (sigBase == 0) return;
    }

    nbyte      = (int)(paramTypes[sigBase]) & 0xFF;
    isVariadic = (nbyte >> 7) & 1;
    nfixed     = nbyte & 0x7F;
#ifdef WARN_ARGCOUNT
    if (isVariadic) {
        if (userArgCount < nfixed)
            warningWithName("too few arguments for variadic function '",
                            ptr + NAME, "'");
    }
    else {
        if (userArgCount != nfixed)
            warningWithName("wrong number of arguments to '",
                            ptr + NAME, "'");
    }
#endif
#ifdef ENABLE_WARNINGS
    /* TODO: per-parameter type/depth check using argDepths[] and argSyms[] */
#endif
}

// collectOneArg(): evaluate one call argument and store its p-codes in argbuf.
// Returns 1 on success; 0 if the argument buffer would overflow (error issued).
//
// is[]        — expression state; written by level1()/fetch() inside
// argLens[]   — records the p-code slot count for this argument (output)
// argSizes[]  — records BPW or BPD for this argument (output)
// idx         — index of this argument in both arrays (0-based)
// pNargs      — running byte-count of all pushed args; updated on success
//
// Globals touched (intentionally, as in the rest of cc3.c):
//   snext, locptr, argbufcur — all updated on return
int collectOneArg(int *is, int *argLens, int *argSizes, int idx, int *pNargs) {
    int savedlocptr, alen, i;
    int *exprStart;
    savedlocptr = locptr;
    exprStart   = snext;
    // Evaluate the argument expression into the staging buffer.
    // Three dispatch cases for lvalue results:
    if (level1(is)) {
        char *argptr;
        if (is[TYP_OBJ] == TYPE_STRUCT) {
            // Struct indirect (TYP_OBJ set): AX already holds the address;
            // no fetch needed — the struct is passed by address.
        }
        else if (is[TYP_OBJ] == 0 && (argptr = is[SYMTAB_ADR])
                 && argptr[TYPE] == TYPE_STRUCT
                 && argptr[IDENT] != IDENT_FUNCTION
                 && argptr[IDENT] != IDENT_PTR_FUNCTION) {
            // Struct direct (no TYP_OBJ, but symbol type is struct):
            // emit POINT1m so AX = address of the named struct variable.
            gen(POINT1m, argptr);
        }
        else {
            // Scalar or pointer lvalue: fetch value into AX (or DX:AX for long).
            fetch(is);
        }
    }
    locptr = savedlocptr;
    // Copy this arg's p-codes from the staging buffer into argbuf.
    alen = (int)(snext - exprStart);
    if (argbufcur + alen + 2 > ARGBUFSZ) {
        error("argument buffer overflow");
        return 0;
    }
    for (i = 0; i < alen; i++)
        argbuf[argbufcur + i] = exprStart[i];
    argLens[idx]  = alen;
    argSizes[idx] = isLongVal(is) ? BPD : BPW;
    argbufcur += alen;   // advance global cursor: nested calls start above here
#ifdef ENABLE_DIAGNOSTICS
    if (argbufcur > argbufcurMax) argbufcurMax = argbufcur;
#endif
    *pNargs   += argSizes[idx];
    // Discard this arg's p-codes from stage[]; the next arg reuses the slot.
    snext = exprStart;
    return 1;
}

// callfunc — emit a complete function call sequence.
//   ptr  — symbol table entry for a named function, or NULL for an indirect
//           call (function address already in AX on entry).
//
// Overview of the two-pass scheme:
//   FIRST PASS:  evaluate arguments left-to-right; store each arg's p-codes
//                in argbuf[] (global scratch), then discard from stage[].
//                argbufcur is advanced after each arg so that nested calls
//                (args that are themselves calls) write above the parent's data.
//   SECOND PASS: replay argbuf chunks in REVERSE order, appending PUSH1 or
//                PUSHd1 after each.  This produces cdecl right-to-left push
//                order so the first argument lands at [BP+4].
//   EMIT CALL:   for indirect calls reload the saved funcptr into AX first;
//                then emit CALLm (named) or CALL1 (indirect), followed by
//                ADDSP to clean up the caller's stack frame.
int callfunc(char *ptr) {      // symbol table entry or 0
    int nargs, is[ISSIZE];
    int retStructSize;
    int argLens[MAX_CALL_ARGS];   // p-code slot count per user arg (for reverse walk)
    int argSizes[MAX_CALL_ARGS];  // BPW or BPD per user arg (for PUSH selection)
    int argDepths[MAX_CALL_ARGS]; // IS_PTRDEPTH per user arg (for type check)
    char *argSyms[MAX_CALL_ARGS]; // SYMTAB_ADR per user arg (for fn-ptr sub-sig check)
    int argbufBase;              // argbufcur at entry; restored after second pass
    int userArgCount;            // number of user (non-hidden) args collected
    int indSaveCsp;              // BP offset of saved funcptr (indirect calls)
    int isFnPtrVar;              // 1 when ptr is IDENT_FNPTR_VAR (checked call + indirect emit)
    int i;

    // --- Phase 0: initialise ---
    nargs        = 0;
    retStructSize = 0;
    argbufBase   = argbufcur;   // mark our region; nested calls write above this
    userArgCount = 0;
    indSaveCsp   = 0;
    isFnPtrVar   = (ptr && ptr[IDENT] == IDENT_FNPTR_VAR);
    blanks();                   // consume whitespace after the open paren already consumed

    // --- Phase 1: entry-side effects ---

#ifdef ENABLE_WARNINGS
#ifdef WARN_IMPLICIT
    // Warn if the function was called without a prior prototype or declaration.
    if (ptr && !isFnPtrVar
        && (ptr[CLASS] & 0x7F) == AUTOEXT && getint(ptr + FNPARAMPTR, 2) == 0)
        warningWithName("implicit declaration of function '", ptr + NAME, "'");
#endif
#endif
    // Upgrade PROTOEXT -> AUTOEXT on first call so trailer() emits EXTRN.
    if (ptr && !isFnPtrVar && (ptr[CLASS] & 0x7F) == PROTOEXT)
        ptr[CLASS] = AUTOEXT;

    // Struct-returning function: allocate the return buffer on the caller's
    // stack and push a hidden pointer to it as the LAST-pushed (outermost)
    // argument (highest BP offset).  User arguments follow at lower offsets.
    // Not applied to fn-ptr variables (their struct-return calling convention
    // is not tracked; fall back to plain indirect for those).
    if (!isFnPtrVar && ptr && ptr[TYPE] == TYPE_STRUCT) {
        retStructSize = getStructSize(getClsPtr(ptr));
        gen(ADDSP, csp - retStructSize);
        gen(POINT1s, csp);
        gen(PUSH1, 0);
        nargs += BPW;           // hidden pointer counts toward ADDSP cleanup
    }
    // Indirect call: save funcptr from AX into a temp stack slot now,
    // before argument evaluation clobbers AX.
    if (ptr == 0 || isFnPtrVar) {
        gen(ADDSP, csp - BPW);   // allocate one-word temp slot
        indSaveCsp = csp;        // BP offset of the temp slot
        gen(PUTws1, indSaveCsp); // [BP + indSaveCsp] = AX
        nargs += BPW;            // temp slot cleaned up by ADDSP after call
    }

    // --- Phase 2: first pass — collect args left-to-right into argbuf ---
    while (streq(lptr, ")") == 0) {
        if (endst())
            break;
        if (userArgCount >= MAX_CALL_ARGS) {
            error("too many arguments");
            break;
        }
        if (collectOneArg(is, argLens, argSizes, userArgCount, &nargs) == 0)
            break;
        argDepths[userArgCount] = is[IS_PTRDEPTH];
        argSyms[userArgCount]   = (char *)is[SYMTAB_ADR];
        userArgCount++;
        if (isMatch(",") == 0)
            break;
    }
    require(")");

    // --- Phase 3: prototype argument count + type check ---
    if (ptr) {
        checkFnArgCountAndTypes(ptr, userArgCount, argDepths, argSyms);
    }
    // --- Phase 4: second pass — replay args in reverse for R-to-L push ---
    // Walk backward through argbuf using cumulative length subtraction,
    // avoiding a separate argBufStart[] array (stack-pressure tradeoff for
    // the 8086 self-hosted environment).
    {
        int pos = argbufcur;   // one-past-end of all collected arg p-codes
        for (i = userArgCount - 1; i >= 0; i--) {
            int j, len;
            int *src;
            pos -= argLens[i];  // rewind to start of arg i's p-codes
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
        argbufcur = argbufBase;   // free our region; nested-call data above is gone
    }

    // --- Phase 5: emit call and stack cleanup ---
    // For indirect calls (ptr==0 or isFnPtrVar): reload saved funcptr into AX.
    if (ptr == 0 || isFnPtrVar)
        gen(GETw1s, indSaveCsp);
    if (ptr && !isFnPtrVar)
        gen(CALLm, ptr);
    else
        gen(CALL1, 0);
    gen(ADDSP, csp + nargs);       // restore SP past all pushed args + hidden slots
    if (retStructSize > 0)
        gen(POINT1s, csp);         // AX = address of struct return buffer on caller stack
}

// Return the pointer scale factor: number of bytes per element.
// Returns 0 if no scaling needed (not a pointer context).
// is1 is the pointer side, is2 is the non-pointer side.
int ptrScale(int oper, int *is1, int *is2) {
    if ((oper != ADD12 && oper != SUB12)
        || (is1[TYP_ADR] == 0)
        || (is2[TYP_ADR])) {
        return 0;
    }
    if (is1[TYP_ADR] == TYPE_VOID) {
        error("void pointer arithmetic not allowed");
        return 0;
    }
    // Multi-level pointer (T** or deeper): the element is itself a pointer,
    // always BPW bytes regardless of the ultimate base type.
    if (is1[IS_PTRDEPTH] > 1)
        return BPW;
    return is1[TYP_ADR] >> 2;
}

// true if is2's operand should be doubled (word pointer scaling)
int isDouble(int oper, int *is1, int *is2) {
    return (ptrScale(oper, is1, is2) == BPW);
}

void step(int oper, int *is, int oper2) {
    int longval, stepsize;
    if (is[IS_BITFIELD]) {
        genBFStep(oper, is, oper2);
        return;
    }
    longval = isLongVal(is);
    fetch(is);
    // Multi-level pointer (T** or deeper): element is itself a pointer -> always BPW.
    if (is[TYP_ADR] && is[IS_PTRDEPTH] > 1)
        stepsize = BPW;
    else
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
int isLongVal(int *is) {
    char *ptr;
    if (is[TYP_CNST] >> 2 == BPD)
        return 1;
    // TYP_OBJ is the element type of an indirect reference.  When TYP_ADR is
    // also set the expression value is a 16-bit pointer (address), not the
    // 32-bit element.  Example: long la[] — TYP_OBJ=TYPE_LONG, TYP_ADR=TYPE_LONG,
    // AX holds the array address.  Do NOT treat that as a long value.
    if (is[TYP_OBJ] >> 2 == BPD && is[TYP_ADR] == 0)
        return 1;
    ptr = is[SYMTAB_ADR];
    if (ptr && is[TYP_OBJ] == 0 && is[TYP_ADR] == 0
        && !isPtrLike(ptr)
        && ptr[TYPE] >> 2 == BPD)
        return 1;
    return 0;
}

// Sign- or zero-extend AX to DX:AX based on signedness of is[].
void widenPrimary(int *is) {
    if (isUnsigned(is))
        gen(WIDENu, 0);
    else
        gen(WIDENs, 0);
}

// Sign- or zero-extend BX to CX:BX based on signedness of is[].
void widenSecondary(int *is) {
    if (isUnsigned(is))
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
int tableFind(int *tab, int key) {
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

// fetchBF(is[]) — emit code to extract a bit-field value into AX.
// Entry: AX = address of the 16-bit word containing the field.
// Exit:  AX = field value (masked, sign- or zero-extended to 16 bits).
//        BX = address of containing word (preserved by AND1n/SHR1n/SHL1n/SAR1n).
void fetchBF(int *is) {
    int bw, boff, is_signed, mask;
    bw = is[BF_WIDTH];
    boff = is[BF_OFFSET];
    is_signed = is[BF_SIGNED];
    if (bw == 16)
        mask = -1;
    else
        mask = (1 << bw) - 1;
    gen(GETw1p, 0);              /* auto-MOVE21: BX=addr; then AX=[BX]=word     */
    if (boff > 0)
        gen(SHR1n, boff);        /* AX >>= boff (logical, unsigned fill)        */
    gen(AND1n, mask);            /* AX &= mask  (isolate field at low bits)      */
    if (is_signed && bw < 16) {
        gen(SHL1n, 16 - bw);    /* bring sign bit to bit 15                    */
        gen(SAR1n, 16 - bw);    /* arithmetic right shift — sign-extends AX    */
    }
}

// storeBF(is[]) — emit a read-modify-write sequence to store AX into a bit-field.
// Entry: AX = new field value (only low bw bits matter), BX = address of word.
// Exit:  [BX] updated; AX = merged word (caller need not use AX afterward).
void storeBF(int *is) {
    int bw, boff, mask, clear_mask;
    bw = is[BF_WIDTH];
    boff = is[BF_OFFSET];
    if (bw == 16)
        mask = -1;
    else
        mask = (1 << bw) - 1;
    if (boff == 0)
        clear_mask = ~mask;
    else
        clear_mask = ~(mask << boff);
    /* 1. Truncate new value to field width (BX = address, preserved by AND1n) */
    gen(AND1n, mask);
    /* 2. Position the new value (BX = address, preserved by SHL1n) */
    if (boff > 0)
        gen(SHL1n, boff);
    /* AX = positioned_new, BX = address */
    /* 3. Read–modify–write */
    gen(PUSH2, 0);               /* PUSH BX  — save address; stack=[...,addr] */
    gen(GETw2p, 0);              /* BX = [BX] = curr_word; AX = positioned_new */
    gen(SWAP12, 0);              /* AX = curr_word, BX = positioned_new        */
    gen(AND1n, clear_mask);      /* AX = curr_word with field bits zeroed       */
    gen(OR12, 0);                /* AX = merged word                           */
    gen(POP2, 0);                /* BX = address; stack=[...]                  */
    gen(PUTwp1, 0);              /* [BX] = AX  (write merged word back)        */
}

// genBFStep(oper, is, oper2) — emit increment/decrement of a bit-field.
// Entry: AX = address of the 16-bit word containing the field.
// oper  = rINC1 or rDEC1 (the modification to apply).
// oper2 = 0 for prefix ++/--; rDEC1 or rINC1 for postfix (to return pre-mod value).
//
// Stack protocol (both prefix and postfix):
//   PUSH1           barrier + save addr;            stack=[addr]
//   GETw1p          BX=addr, AX=curr_word           (MOVE21 inside GETw1p cannot
//                                                    fold with POINT1s across PUSH1)
//   extract field -> AX=field_value, BX=addr
//   postfix: PUSH1  save old field value;            stack=[addr, old_fv]
//   gen(oper,1)     AX = new_field_value, BX=addr
//   prefix:  PUSH1  save new field value;            stack=[addr, new_fv]
//   storeBF         RMW (internally balanced PUSH2/POP2); BX=addr, AX=merged_word
//   POP2            BX = result_fv;                  stack=[addr]
//   SWAP12          AX = result_fv, BX = merged_word
//   POP2            BX = addr (discard);             stack=[]
//   AX = result_fv  (new value for prefix, old value for postfix)
void genBFStep(int oper, int *is, int oper2) {
    int bw, boff, is_signed, mask;
    bw = is[BF_WIDTH];
    boff = is[BF_OFFSET];
    is_signed = is[BF_SIGNED];
    if (bw == 16)
        mask = -1;
    else
        mask = (1 << bw) - 1;
    /* Push address: acts as optimizer barrier AND saves addr for cleanup */
    gen(PUSH1, 0);               /* stack=[addr]; AX=addr                      */
    /* GETw1p auto-emits its own MOVE21; PUSH1 above prevents the peephole    */
    /* optimizer from folding POINT1s+MOVE21 -> LEA BX (which would leave AX  */
    /* stale and break the entire extract/modify/store sequence).              */
    gen(GETw1p, 0);              /* MOVE21: BX=addr; MOV AX,[BX]: AX=curr_word */
    /* Extract field from AX (all *1n p-codes are BX-preserving): */
    if (boff > 0)
        gen(SHR1n, boff);        /* AX >>= boff (logical; BX=addr preserved)   */
    gen(AND1n, mask);            /* AX &= mask  (BX=addr preserved)            */
    if (is_signed && bw < 16) {
        gen(SHL1n, 16 - bw);    /* bring sign bit to bit 15 (BX preserved)    */
        gen(SAR1n, 16 - bw);    /* sign-extend AX (BX preserved)              */
    }
    /* AX = field_value, BX = addr */
    if (oper2)
        gen(PUSH1, 0);           /* postfix: save old_fv; stack=[addr,old_fv]  */
    gen(oper, 1);                /* AX = new_field_value; BX = addr            */
    if (!oper2)
        gen(PUSH1, 0);           /* prefix:  save new_fv; stack=[addr,new_fv]  */
    /* storeBF entry: AX=new_fv, BX=addr; internally PUSH2/POP2 balanced      */
    storeBF(is);
    /* After storeBF: BX=addr, AX=merged_word; stack=[addr, result_fv]        */
    gen(POP2, 0);                /* BX = result_fv; stack=[addr]               */
    gen(SWAP12, 0);              /* AX = result_fv, BX = merged_word           */
    gen(POP2, 0);                /* BX = addr (discard); stack=[]              */
    /* AX = new_fv (prefix) or old_fv (postfix) — correct expression value    */
}

void store(int *is) {
    char *ptr;
    if (is[IS_BITFIELD]) {
        storeBF(is);
        return;
    }
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
        if (!isPtrLike(ptr) && ptr[TYPE] >> 2 == BPD) {
            gen(PUTdm1, ptr);
        }
        else if (!isPtrLike(ptr) && ptr[TYPE] >> 2 == 1) {
            gen(PUTbm1, ptr);
        }
        else {
            gen(PUTwm1, ptr);
        }
    }
}

void fetch(int *is) {
    char *ptr;
    if (is[IS_BITFIELD]) {
        fetchBF(is);
        return;
    }
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
        if (isPtrLike(ptr) || ptr[TYPE] >> 2 == BPW) {
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

int constant(int *is) {
    int offset;
    if (is[TYP_CNST] = number(&is[VAL_CNST], &is[VAL_CNST_HI]))
        genConstLoad(is);
    else if (is[TYP_CNST] = chrcon(&is[VAL_CNST], &is[VAL_CNST_HI]))
        genConstLoad(is);
    else if (string(&offset)) {
        gen(POINT1l, offset);
        is[TYP_ADR] = TYPE_CHR;   // AX is a char *; allows subscripting
        is[TYP_OBJ] = TYPE_CHR;
    }
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
        if (isMatch("+"));
        else if (isMatch("-"))
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
    if (isMatch("'") == 0)
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
    if (isMatch("\"") == 0) {
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
    } while (isMatch("\""));
    litq[litptr++] = 0;
#ifdef ENABLE_DIAGNOSTICS
    if (litptr > litptrMax) litptrMax = litptr;
#endif
    return 1;
}

void stowlit(int value, int size) {
    if ((litptr + size) >= LITMAX) {
        error("literal queue overflow");
        abort(ERRCODE);
    }
    putint(value, litq + litptr, size);
    litptr += size;
#ifdef ENABLE_DIAGNOSTICS
    if (litptr > litptrMax) litptrMax = litptr;
#endif
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
int skim(char *opstr, int tcode, int dropval, int endval, int (*level)(), int *is) {
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
void dropout(int k, int tcode, int exit1, int *is) {
    if (k)
        fetch(is);
    else if (is[TYP_CNST])
        genConstLoad(is);
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
int down(char *opstr, int opoff, int (*level)(), int *is) {
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

// Unary drop to a lower level.
// Calls level() to parse the next sub-expression into is[].
// If the result is a compile-time constant, the p-codes generated by level()
// are discarded immediately; they will be emitted efficiently once the full
// binary operation context is known in down2.
// Returns the lvalue flag from level() for use by the caller.
int down1(int (*level)(), int *is) {
    int k, *before, *start;
    setstage(&before, &start);
    k = (*level)(is);
    if (is[TYP_CNST]) {
        // Discard code emitted for a constant sub-expression; down2 will
        // load the constant value directly when it knows the full context.
        clearStage(before, 0);
    }
    return k;
}

// Attempt to reverse a comparison operator pair in-place.
// Swaps operand sense (LT<->GT, LE<->GE) to avoid an extra register move.
// Updates *oper and *oper2 only when both can be reversed and guard is non-zero.
// Returns 1 if the reversal was applied, 0 otherwise.
int tryReverse(int *oper, int *oper2, int guard) {
    int rev, rev2;
    rev  = reverseOp(*oper);
    rev2 = reverseOp(*oper2);
    if (rev && rev2 && guard) {
        *oper  = rev;
        *oper2 = rev2;
        return 1;
    }
    return 0;
}

// Fold a 32-bit constant binary operation into is[] at compile time.
// Called only when both sides are compile-time constants and eitherLong is true.
// Widens any 16-bit constant half to 32 bits, dispatches to the appropriate
// calcXxx32 function, then writes the folded value and result type into is[].
void foldConst32(int oper, int leftLong, int rightLong, int *is, int *is2) {
    int lhi, llo, rhi, rlo, reshi, reslo;
    llo = is[VAL_CNST];
    lhi = is[VAL_CNST_HI];
    if (!leftLong) {
        // Widen the 16-bit left constant: zero-extend if unsigned, sign-extend if signed.
        lhi = (is[TYP_CNST] == TYPE_UINT) ? 0 : ((llo & 0x8000) ? -1 : 0);
    }
    rlo = is2[VAL_CNST];
    rhi = is2[VAL_CNST_HI];
    if (!rightLong) {
        // Widen the 16-bit right constant: zero-extend if unsigned, sign-extend if signed.
        rhi = (is2[TYP_CNST] == TYPE_UINT) ? 0 : ((rlo & 0x8000) ? -1 : 0);
    }
    reshi = reslo = 0;
    // Dispatch to the correct folding function based on operator and signedness.
    if (oper == LT12 || oper == LE12 || oper == GT12 || oper == GE12) {
        calcCmp32(lhi, llo, oper, rhi, rlo, &reshi, &reslo);
    }
    else if (isUnsigned(is) || isUnsigned(is2)) {
        calcUConst32(lhi, llo, oper, rhi, rlo, &reshi, &reslo);
    }
    else {
        calcConst32(lhi, llo, oper, rhi, rlo, &reshi, &reslo);
    }
    is[VAL_CNST]    = reslo;
    is[VAL_CNST_HI] = reshi;
    // Set the result type.  Comparisons always yield a 16-bit int; arithmetic
    // keeps the 32-bit type of the wider operand (signed or unsigned long).
    if (oper == EQ12 || oper == NE12 ||
        oper == LT12 || oper == LE12 || oper == GT12 || oper == GE12 ||
        oper == LT12u || oper == LE12u || oper == GT12u || oper == GE12u) {
        is[TYP_CNST]    = TYPE_INT;
        is[VAL_CNST_HI] = 0;
    }
    else if (isUnsigned(is) || isUnsigned(is2)) {
        is[TYP_CNST] = TYPE_ULONG;
    }
    else {
        is[TYP_CNST] = TYPE_LONG;
    }
}

// Propagate pointer type and signedness from the operands into the result is[].
// Called after both the constant-folding and variable code-emission paths in down2.
void applyResultType(int oper, int *is, int *is2) {
    char *ptr;
#ifdef ENABLE_WARNINGS
    // ptr-depth-mismatch-warning
    // Warn when comparing two pointers whose depths differ.
    // Null constant (0) is always allowed without warning.
    if ((oper == EQ12 || oper == NE12 ||
         oper == LT12 || oper == LE12 || oper == GT12 || oper == GE12 ||
         oper == LT12u || oper == LE12u || oper == GT12u || oper == GE12u)
        && is[TYP_ADR] && is2[TYP_ADR]
        && is[IS_PTRDEPTH] >= 1 && is2[IS_PTRDEPTH] >= 1
        && is[IS_PTRDEPTH] != is2[IS_PTRDEPTH]
        && !(is[TYP_CNST]  && is[VAL_CNST]  == 0)
        && !(is2[TYP_CNST] && is2[VAL_CNST] == 0))
        warning("pointer comparison: depth mismatch");
#endif
    // For add/subtract involving pointer operands, fix up the result address type.
    if (oper == SUB12 || oper == ADD12) {
        if (is[TYP_ADR] && is2[TYP_ADR]) {
            // pointer - pointer: result is a plain integer offset, not a pointer.
            is[TYP_ADR]     = 0;
            is[IS_PTRDEPTH] = 0;
        }
        else if (is2[TYP_ADR]) {
            // integer +/- pointer: result inherits the RHS pointer type.
            is[SYMTAB_ADR]  = is2[SYMTAB_ADR];
            is[TYP_OBJ]     = is2[TYP_OBJ];
            is[TYP_ADR]     = is2[TYP_ADR];
            is[IS_PTRDEPTH] = is2[IS_PTRDEPTH];
        }
        // else: pointer +/- integer: IS_PTRDEPTH stays from the LHS (already in is[])
    }
    // Propagate SYMTAB_ADR from the RHS when the LHS has no symbol entry,
    // or when the RHS symbol is unsigned (unsigned-ness infects the result type).
    if (is[SYMTAB_ADR] == 0 ||
        ((ptr = is2[SYMTAB_ADR]) && (ptr[TYPE] & IS_UNSIGNED))) {
        is[SYMTAB_ADR] = is2[SYMTAB_ADR];
    }
}

// Emit register-setup code for Arm B when at least one operand is 32-bit
// and the RHS is a compile-time constant.
// Called after the speculative PUSH has been retracted and clearStage run.
// On entry: LHS value is in AX (or DX:AX if leftLong; callee widens if not).
// On return:
//   ADD12: LHS stays in DX:AX (primary); RHS constant loaded into CX:BX.
//   other: LHS moved to CX:BX (secondary); RHS constant loaded into DX:AX.
void emitConst32Rhs(int oper, int leftLong, int *is, int *is2) {
    if (!leftLong)
        widenPrimary(is);
    if (oper == ADD12) {
        // Addition is commutative: leave LHS in DX:AX as the primary,
        // and load the 32-bit RHS constant into the secondary (CX:BX).
        gen(GETw2n, is2[VAL_CNST]);
        gen(GETcxn, is2[VAL_CNST_HI]);
    }
    else {
        // Non-commutative: move LHS to secondary (CX:BX), then load
        // the 32-bit RHS constant into the primary (DX:AX).
        gen(MOVEd21, 0);
        gen(GETw1n, is2[VAL_CNST]);
        gen(GETdxn, is2[VAL_CNST_HI]);
    }
}

// Emit register-setup code for Arm B when both operands are 16-bit
// and the RHS is a compile-time constant.
// Called after the speculative PUSH has been retracted and clearStage run.
// On entry: LHS value is in AX.
// *oper/*oper2 may be updated in-place by tryReverse() so that the
// operator-emit tail sees the (possibly reversed) operator.
void emitConst16Rhs(int *oper, int *oper2, int *is, int *is2) {
    int sc;
    sc = ptrScale(*oper, is, is2);
    if (!sc) sc = 1;
    if (*oper == ADD12) {
        // Addition is commutative: load the (scaled) RHS constant
        // directly into the secondary register (BX).
        gen(GETw2n, is2[VAL_CNST] * sc);
    }
    else {
        // Non-commutative.  Try reversing the operator so the constant
        // can go into BX like the commutative case, avoiding MOVE21+GETw1n.
        // Reversal is suppressed when STG_ADR is set (the zero-test signal
        // is active and must not be invalidated by swapping operands).
        if (tryReverse(oper, oper2, !is[STG_ADR])) {
            gen(GETw2n, is2[VAL_CNST] * sc);
        }
        else {
            // Cannot reverse: move LHS to BX, then load RHS into AX.
            gen(MOVE21, 0);
            gen(GETw1n, is2[VAL_CNST] * sc);
        }
    }
}

// Prepare registers for Arm C when at least one operand is long.
// On entry: RHS is in AX (or DX:AX if rightLong); LHS is on the stack.
// On return:
//   primary   (DX:AX) = RHS, widened to 32 bits if it was only 16 bits.
//   secondary (CX:BX) = LHS, popped from the stack and widened if needed.
void prepLongRegs(int leftLong, int rightLong, int *is, int *is2) {
    // Widen the RHS to DX:AX if it arrived as only 16 bits.
    if (!rightLong)
        widenPrimary(is2);
    // Pop the LHS from the stack into the secondary register pair.
    if (leftLong) {
        gen(POPd2, 0);
    }
    else {
        // LHS is 16-bit but the operation is 32-bit: pop into BX
        // then sign/zero-extend to fill CX:BX.
        gen(POP2, 0);
        widenSecondary(is);
    }
}

// Scale primary and secondary registers for pointer arithmetic in Arm C (16-bit path).
// Called after gen(POP2, 0) has moved LHS into BX; RHS is in AX.
// If LHS (is[]) is a pointer, the integer side (RHS = AX = DBL1) is scaled.
// If RHS (is2[]) is a pointer, the integer side (LHS = BX = DBL2) is scaled.
// DBL doubles a register; two DBLs scale by 4 for long* arithmetic.
void scalePtrOperands(int oper, int *is, int *is2) {
    if (ptrScale(oper, is, is2) == BPD) {
        gen(DBL1, 0); gen(DBL1, 0);  // LHS is a long*: scale RHS (AX) by 4
    }
    else if (isDouble(oper, is, is2)) {
        gen(DBL1, 0);                 // LHS is a word*: scale RHS (AX) by 2
    }
    if (ptrScale(oper, is2, is) == BPD) {
        gen(DBL2, 0); gen(DBL2, 0);  // RHS is a long*: scale LHS (BX) by 4
    }
    else if (isDouble(oper, is2, is)) {
        gen(DBL2, 0);                 // RHS is a word*: scale LHS (BX) by 2
    }
}

// down2() evaluates (LHS oper RHS) where LHS is already described by is[].
// Parses the RHS by recursing into level(), then arranges the register pair:
//   primary   (AX / DX:AX) = RHS
//   secondary (BX / CX:BX) = LHS
// After setup, emits the operator p-code and updates is[] with the result type.
// When oper == 0 (subscript mode, called from level14 for array indexing):
// only the operand setup is performed and is[] is not updated.
void down2(int oper, int oper2, int (*level)(), int *is, int *is2) {
    int *before, *start;
    int leftLong, rightLong, eitherLong, loper, sc;
    setstage(&before, &start);
    // Clear the STG_ADR signal; it will be set below only when one operand is zero,
    // allowing test() to emit a more compact "compare to zero" instruction sequence.
    is[STG_ADR] = 0;
    leftLong = isLongVal(is);

    if (is[TYP_CNST]) {
        // ---- Arm A: constant LHS, unknown RHS ----
        // Invariant: TYP_CNST and TYP_ADR are mutually exclusive.  number() and
        // chrcon() never set TYP_ADR; pointer casts clear TYP_CNST; applycast()
        // clears TYP_ADR on scalar results.  Therefore ptrScale(oper, is2, is)
        // below correctly treats is2 (the RHS) as the potential pointer --
        // is[TYP_ADR] is guaranteed 0 in this arm.
        // Parse and fetch the RHS from level(); it lands in AX (or DX:AX for longs).
        if (down1(level, is2))
            fetch(is2);
        rightLong  = isLongVal(is2);
        eitherLong = leftLong || rightLong;
        if (eitherLong) {
            // RHS is in AX.  If it is only 16-bit, sign/zero-extend it to DX:AX.
            if (!rightLong)
                widenPrimary(is2);
            // If the LHS constant is zero, record this position so test() can
            // replace the upcoming GETw2n/GETcxn pair with a cheaper zero-test.
            if (is[VAL_CNST] == 0 && is[VAL_CNST_HI] == 0)
                is[STG_ADR] = snext;
            // Load the 32-bit LHS constant into the secondary register pair (CX:BX).
            gen(GETw2n, is[VAL_CNST]);
            gen(GETcxn, is[VAL_CNST_HI]);
        }
        else {
            // If the LHS constant is zero, record this position so test() can
            // replace the upcoming GETw2n with a cheaper zero-test.
            if (is[VAL_CNST] == 0)
                is[STG_ADR] = snext;
            // Apply pointer-element-size scaling for pointer + integer arithmetic.
            sc = ptrScale(oper, is2, is);
            if (!sc) sc = 1;
            // Load the (possibly scaled) 16-bit LHS constant into the secondary (BX).
            gen(GETw2n, is[VAL_CNST] * sc);
        }
    }
    else {
        // ---- Arms B and C: variable LHS, unknown RHS ----
        // Push the LHS value onto the stack to free the primary register for the RHS.
        if (leftLong)
            gen(PUSHd1, 0);
        else
            gen(PUSH1, 0);
        // Parse and fetch the RHS into the primary register (AX / DX:AX).
        if (down1(level, is2))
            fetch(is2);
        rightLong  = isLongVal(is2);
        eitherLong = leftLong || rightLong;

        if (is2[TYP_CNST]) {
            // ---- Arm B: variable LHS, constant RHS ----
            // If the RHS constant is zero, record the start of this binary operation
            // so test() can replace the operand-setup sequence with a zero-test.
            if (is2[VAL_CNST] == 0 && is2[VAL_CNST_HI] == 0)
                is[STG_ADR] = start;
            // Retract the speculative PUSH: we no longer need the stack slot because
            // the constant will be loaded directly into a register.
            if (leftLong)
                csp += BPD;
            else
                csp += BPW;
            clearStage(before, 0);
            if (eitherLong) {
                emitConst32Rhs(oper, leftLong, is, is2);
            }
            else {
                emitConst16Rhs(&oper, &oper2, is, is2);
            }
        }
        else {
            // ---- Arm C: variable LHS, variable RHS ----
            // RHS is now in AX; LHS is on the stack waiting to be popped into BX.
            if (!eitherLong && before && snext == before + 4
                && (before[2] == GETw1m || before[2] == GETw1s)) {
                // The RHS was just loaded by a single GETw1m/GETw1s instruction.
                // Stage buffer layout: int pairs (pc, arg); before[2] is the p-code
                // of that instruction and before[3] is its address operand.
                // Retract that load, reverse the operator, and re-emit a GETw2m/GETw2s
                // so the CMP-Memory/Stack peephole rules in the optimizer can fire.
                int rhspc  = before[2];
                int rhsval = before[3];
                // Guard is 1, not !is[STG_ADR]: STG_ADR is always 0 at this
                // point (cleared at entry, never set in Arm A or on the Arm C path).
                if (tryReverse(&oper, &oper2, 1)) {
                    csp += BPW;
                    clearStage(before, 0);
                    gen(rhspc == GETw1m ? GETw2m : GETw2s, rhsval);
                }
                else {
                    // Operator is not reversible: pop LHS into BX the normal way.
                    gen(POP2, 0);
                }
            }
            else if (eitherLong) {
                prepLongRegs(leftLong, rightLong, is, is2);
            }
            else {
                // Both operands are 16-bit: pop LHS into BX.
                gen(POP2, 0);
                // Scale operands for pointer arithmetic if either side is a pointer.
                scalePtrOperands(oper, is, is2);
            }
        }
    }

    // When oper == 0 this function was called from level14 in subscript mode
    // (array indexing).  The register setup above is all that is needed; there
    // is no operator to emit and is[] must not be updated.
    if (oper == 0)
        return;

    // ---- Select the actual operator p-code to emit ----
    // Switch to the unsigned variant when either operand is unsigned.
    if (isUnsigned(is) || isUnsigned(is2))
        oper = oper2;
    // Map the 16-bit operator to its 32-bit counterpart when either side is long.
    loper = eitherLong ? toLongOp(oper) : oper;

    if (is[TYP_CNST] && is2[TYP_CNST]) {
        // ---- Both operands are constants: fold the result at compile time ----
        if (eitherLong) {
            foldConst32(oper, leftLong, rightLong, is, is2);
        }
        else {
            // 16-bit constant fold.
            is[VAL_CNST] = calcConst(is[VAL_CNST], oper, is2[VAL_CNST]);
            // If the RHS was an unsigned constant, the result is also unsigned.
            if (is2[TYP_CNST] == TYPE_UINT)
                is[TYP_CNST] = TYPE_UINT;
        }
        // Discard all p-codes emitted during operand setup; a compile-time
        // constant result needs no code.
        clearStage(before, 0);
    }
    else {
        // ---- At least one operand is variable: emit the operator p-code ----
        is[TYP_CNST]    = 0;
        is[VAL_CNST_HI] = 0;
        gen(loper, 0);
        // For pointer subtraction the raw result is a byte difference.
        // Divide it by the element size to produce an element count.
        if (oper == SUB12 && is[TYP_ADR] && is2[TYP_ADR]) {
            int elemsz;
            // Multi-level pointer: elements are pointers themselves (BPW bytes).
            // Single-level: use the element size encoded in TYP_ADR.
            if (is[IS_PTRDEPTH] > 1)
                elemsz = BPW;
            else
                elemsz = is[TYP_ADR] >> 2;
            if (elemsz == BPW) {
                gen(SWAP12, 0);
                gen(GETw1n, 1);
                gen(ASR12, 0);
            }
            else if (elemsz == BPD) {
                gen(SWAP12, 0);
                gen(GETw1n, 2);
                gen(ASR12, 0);
            }
            // else elemsz == 1 (char* - char*): no division needed
        }
        // Record the emitted operator so test() can recognise "expr op 0" patterns
        // and choose a more compact jump sequence.
        is[LAST_OP] = loper;
        if (eitherLong && isCmpOp(loper)) {
            // A long comparison always produces a 16-bit int result (0 or 1).
            eitherLong     = 0;
            is[TYP_OBJ]    = 0;
            is[SYMTAB_ADR] = 0;
        }
        else if (eitherLong) {
            // A non-comparison long operation produces a 32-bit result.
            is[TYP_OBJ]    = (isUnsigned(is) || isUnsigned(is2)) ? TYPE_ULONG : TYPE_LONG;
            is[SYMTAB_ADR] = 0;
        }
    }

    // Propagate pointer type and signedness from the operands into the result.
    applyResultType(oper, is, is2);
}

// unsigned operand?
int isUnsigned(int *is) {
    char *ptr;
    if (is[TYP_ADR] || is[TYP_CNST] == TYPE_UINT || is[TYP_CNST] == TYPE_ULONG ||
        is[TYP_VAL] & IS_UNSIGNED ||
        ((ptr = is[SYMTAB_ADR]) && (ptr[TYPE] & IS_UNSIGNED)))
        return 1;
    return 0;
}

// calculate signed constant result
int calcConst(int left, int oper, int right) {
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
    return calcUConst(left, oper, right);
}

// calculate unsigned constant result
int calcUConst(unsigned left, int oper, unsigned right) {
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
void calcConst32(int lhi, int llo, int oper, int rhi, int rlo,
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
        case ASR12:                           // arithmetic right shift; preserves sign
            *reshi = lhi;
            *reslo = llo;
            shift32r(reshi, reslo, rlo & 31, 1);
            return;
        case LSR12:                           // logical right shift; zero-fills
            *reshi = lhi;
            *reslo = llo;
            shift32r(reshi, reslo, rlo & 31, 0);
            return;
        default:
            // MUL/DIV/MOD: too complex for 16-bit compile-time folding
            *reslo = llo;
            *reshi = lhi;
            return;
    }
}

// Shared 32-bit right-shift loop for calcConst32.
// signExt=1: arithmetic shift (fill with sign bit); signExt=0: logical shift (fill with 0).
void shift32r(int *reshi, int *reslo, int count, int signExt) {
    while (count > 0) {
        *reslo = (*reslo >> 1) & 0x7FFF;
        if (*reshi & 1) 
            *reslo = *reslo | 0x8000;  // carry hi bit 0 into lo bit 15
        if (signExt && (*reshi & 0x8000))
            *reshi = (*reshi >> 1) | 0x8000;        // arithmetic: preserve sign
        else
            *reshi = (*reshi >> 1) & 0x7FFF;        // logical: zero-fill
        count--;
    }
}

// 32-bit unsigned constant folding for comparison operators.
void calcUConst32(int lhi, int llo, int oper, int rhi, int rlo,
             int *reshi, int *reslo) {
    unsigned ulhi, ullo, urhi, urlo;
    ulhi = lhi;
    ullo = llo;
    urhi = rhi;
    urlo = rlo;
    *reshi = 0;
    switch (oper) {
        case LT12: case LT12u:
            *reslo = (ulhi < urhi) ? 1 : (ulhi > urhi) ? 0 : (ullo < urlo)  ? 1 : 0; 
            return;
        case LE12: case LE12u:
            *reslo = (ulhi < urhi) ? 1 : (ulhi > urhi) ? 0 : (ullo <= urlo) ? 1 : 0; 
            return;
        case GT12: case GT12u:
            *reslo = (ulhi > urhi) ? 1 : (ulhi < urhi) ? 0 : (ullo > urlo)  ? 1 : 0; 
            return;
        case GE12: case GE12u:
            *reslo = (ulhi > urhi) ? 1 : (ulhi < urhi) ? 0 : (ullo >= urlo) ? 1 : 0; 
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
            calcConst32(lhi, llo, oper, rhi, rlo, reshi, reslo);
            return;
    }
}

// Signed 32-bit comparison folding.
void calcCmp32(int lhi, int llo, int oper, int rhi, int rlo,
          int *reshi, int *reslo) {
    unsigned ullo, urlo;
    ullo = llo;
    urlo = rlo;
    *reshi = 0;
    switch (oper) {
        case LT12: 
            *reslo = (lhi < rhi) ? 1 : (lhi > rhi) ? 0 : (ullo < urlo)  ? 1 : 0; 
            return;
        case LE12: 
            *reslo = (lhi < rhi) ? 1 : (lhi > rhi) ? 0 : (ullo <= urlo) ? 1 : 0; 
            return;
        case GT12: 
            *reslo = (lhi > rhi) ? 1 : (lhi < rhi) ? 0 : (ullo > urlo)  ? 1 : 0; 
            return;
        case GE12: 
            *reslo = (lhi > rhi) ? 1 : (lhi < rhi) ? 0 : (ullo >= urlo) ? 1 : 0; 
            return;
        default:   
            *reslo = 0; 
            return;
    }
}
