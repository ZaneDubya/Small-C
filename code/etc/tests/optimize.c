// optimize.c -- Test program for SETSFLG/fset peephole optimization.
//
// Purpose:
//   Verify that the compiler's peephole optimizer correctly eliminates the
//   redundant "OR AX,AX" flag-setting instruction that precedes a zero-test
//   conditional branch when the preceding ALU operation already sets the
//   flags.  The tests confirm correct runtime behavior after optimization.
//
// Functionality covered:
//   - AND12 (bitwise AND) directly followed by NE10f and EQ10f branches
//   - OR12  (bitwise OR)  directly followed by NE10f and EQ10f branches
//   - XOR12 (bitwise XOR) directly followed by NE10f branch
//   - ADD12 (addition)    directly followed by NE10f branch
//   - SUB12 (subtraction) directly followed by NE10f branch
//   - ANEG1 (arithmetic negate) directly followed by NE10f branch
//   - DBL1  (double / left shift by 1) directly followed by NE10f branch
//   - ASL12 (arithmetic shift left)  directly followed by NE10f branch
//   - ASR12 (arithmetic shift right) directly followed by NE10f branch
//   - SETSFLG flag propagation: each optimized op emits NE10fp/EQ10fp
//     instead of NE10f/EQ10f, removing the intervening OR AX,AX

#include "../../smallc22/stdio.h"

int passed, failed;

void check(char *desc, int cond) {
    if (cond) {
        printf("  PASS: %s\n", desc);
        passed++;
    } else {
        printf("  FAIL: %s\n", desc);
        failed++;
        getchar();
    }
}

// ============================================================================
// AND12 + NE10f / EQ10f
// ============================================================================

int test_and(int a, int b) {
    int r;
    r = 0;
    if (a & b) r = 1;        // AND12 + NE10f → NE10fp expected
    return r;
}

int test_and_eq(int a, int b) {
    int r;
    r = 0;
    if (!(a & b)) r = 1;     // AND12 + EQ10f → EQ10fp expected
    return r;
}

// ============================================================================
// OR12 + NE10f / EQ10f
// ============================================================================

int test_or(int a, int b) {
    int r;
    r = 0;
    if (a | b) r = 1;        // OR12 + NE10f
    return r;
}

int test_or_eq(int a, int b) {
    int r;
    r = 0;
    if (!(a | b)) r = 1;     // OR12 + EQ10f
    return r;
}

// ============================================================================
// XOR12 + NE10f / EQ10f
// ============================================================================

int test_xor(int a, int b) {
    int r;
    r = 0;
    if (a ^ b) r = 1;        // XOR12 + NE10f
    return r;
}

// ============================================================================
// ADD12 + NE10f / EQ10f
// ============================================================================

int test_add(int a, int b) {
    int r;
    r = 0;
    if (a + b) r = 1;        // ADD12 + NE10f
    return r;
}

// ============================================================================
// SUB12 + NE10f / EQ10f
// ============================================================================

int test_sub(int a, int b) {
    int r;
    r = 0;
    if (a - b) r = 1;        // SUB12 + NE10f
    return r;
}

// ============================================================================
// ANEG1 + NE10f
// ============================================================================

int test_neg(int a) {
    int r;
    r = 0;
    if (-a) r = 1;           // ANEG1 + NE10f
    return r;
}

// ============================================================================
// DBL1 + NE10f (SHL AX,1)
// ============================================================================

int test_dbl(int a) {
    int r;
    r = 0;
    if (a + a) r = 1;        // ADD12 on same var → DBL1 if optimized, else ADD12+NE10f
    return r;
}

// ============================================================================
// ASL12 + NE10f (shift AX by CL)
// ============================================================================

int test_shl(int a, int b) {
    int r;
    r = 0;
    if (a << b) r = 1;       // ASL12 + NE10f (no SETSFLG: SAL by CL=0 skips flag update)
    return r;
}

// ============================================================================
// ASR12 + NE10f (arithmetic right shift AX by CL)
// ============================================================================

int test_shr(int a, int b) {
    int r;
    r = 0;
    if (a >> b) r = 1;       // ASR12 + NE10f (no SETSFLG: SAR by CL=0 skips flag update)
    return r;
}

// ============================================================================
// Main: verify all patterns produce correct results
// ============================================================================

void main() {
    printf("=== SETSFLG/fset optimization tests ===\n");

    // AND
    check("and(3,5)!=0",  test_and(3, 5));     // 3 & 5 = 1 → truthy
    check("and(2,5)==0", !test_and(2, 5));     // 2 & 5 = 0 → falsy
    check("!and(2,5)",    test_and_eq(2, 5));  // !(2 & 5) → 1
    check("!and(3,5)",   !test_and_eq(3, 5)); // !(3 & 5) → 0

    // OR
    check("or(0,0)==0",  !test_or(0, 0));      // 0|0=0 falsy
    check("or(1,0)!=0",   test_or(1, 0));      // 1|0=1 truthy
    check("!or(0,0)",     test_or_eq(0, 0));   // !(0|0)=1
    check("!or(1,0)",    !test_or_eq(1, 0));

    // XOR
    check("xor(3,3)==0", !test_xor(3, 3));    // 3^3=0 falsy
    check("xor(3,1)!=0",  test_xor(3, 1));    // 3^1=2 truthy

    // ADD
    check("add(1,-1)==0", !test_add(1, -1));  // 1+(-1)=0 falsy
    check("add(1,2)!=0",   test_add(1, 2));   // 1+2=3 truthy

    // SUB
    check("sub(5,5)==0",  !test_sub(5, 5));   // 5-5=0 falsy
    check("sub(6,5)!=0",   test_sub(6, 5));   // 6-5=1 truthy

    // NEG
    check("neg(0)==0",  !test_neg(0));         // -0=0 falsy
    check("neg(1)!=0",   test_neg(1));         // -1!=0 truthy

    // DBL (a+a)
    check("dbl(0)==0",  !test_dbl(0));         // 0+0=0 falsy
    check("dbl(3)!=0",   test_dbl(3));         // 3+3=6 truthy

    // SHL (count >= 1 only; SAL with CL=0 does not update flags on 8086)
    check("shl(4,2)!=0",  test_shl(4, 2));    // 4<<2=16 truthy
    check("shl(0,4)==0", !test_shl(0, 4));    // 0<<4=0 falsy

    // SHR (signed)
    check("shr(4,1)!=0",  test_shr(4, 1));    // 4>>1=2 truthy
    check("shr(0,1)==0", !test_shr(0, 1));    // 0>>1=0 falsy

    printf("\nPassed: %d  Failed: %d\n", passed, failed);
    if (failed) getchar();
}
