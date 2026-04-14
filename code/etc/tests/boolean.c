// boolean.c -- Test program for || / && boxing elimination pass (boxing_elim_pass).
//
// Purpose:
//   Verify that boxing_elim_pass() in flushfunc() correctly eliminates the
//   0/1 materialization tail emitted by skim() for || and && expressions
//   when the result is consumed by a conditional branch.
//   All tests confirm correct runtime behaviour after the transformation.
//
// Functionality covered:
//   - if (a || b)              Pattern A basic
//   - if (a && b)              Pattern B basic
//   - if (a || b || c)         Pattern A chained (3 terms)
//   - if (a && b && c)         Pattern B chained (3 terms)
//   - if (a || b && c)         mixed: inner && (B) nested inside outer || (A)
//   - while (a || b)           Pattern A in loop condition
//   - for (;a&&b;)             Pattern B in for-expr2
//   - (a || b) ? 1 : 2         Pattern A in ternary condition (dropout consumer)
//   - if (a==b || c>d)         comparisons as sub-terms + boxing elimination
//   - r = a || b               value context: NO transformation (not NE10f consumer)
//   - r = a && b               value context: NO transformation

#include "../../smallc22/stdio.h"

int passed, failed;

void check(char *desc, int cond) {
    if (cond) {
        printf("  PASS: %s\n", desc);
        passed++;
    } else {
        printf("  FAIL: %s\n", desc);
        failed++;
    }
}

// ============================================================================
// Pattern A: if (a || b)  -- basic two-term OR
// ============================================================================

int test_or_if(int a, int b) {
    if (a || b) return 1;
    return 0;
}

// ============================================================================
// Pattern B: if (a && b)  -- basic two-term AND
// ============================================================================

int test_and_if(int a, int b) {
    if (a && b) return 1;
    return 0;
}

// ============================================================================
// Pattern A chained: if (a || b || c)
// ============================================================================

int test_or3(int a, int b, int c) {
    if (a || b || c) return 1;
    return 0;
}

// ============================================================================
// Pattern B chained: if (a && b && c)
// ============================================================================

int test_and3(int a, int b, int c) {
    if (a && b && c) return 1;
    return 0;
}

// ============================================================================
// Mixed: if (a || b && c)   inner && (B) nested inside outer || (A)
// ============================================================================

int test_mixed(int a, int b, int c) {
    if (a || b && c) return 1;
    return 0;
}

// ============================================================================
// Pattern A in while loop: while (a || b)
// ============================================================================

int test_or_while(int a, int b) {
    int count;
    count = 0;
    while (a || b) {
        count++;
        a = 0;
        b = 0;
    }
    return count;
}

// ============================================================================
// Pattern B in for loop: for (;a&&b;)
// ============================================================================

int test_and_for(int a, int b) {
    int count;
    count = 0;
    for (; a && b;) {
        count++;
        a = 0;
    }
    return count;
}

// ============================================================================
// Pattern A in ternary: (a || b) ? 1 : 2
// ============================================================================

int test_or_ternary(int a, int b) {
    return (a || b) ? 1 : 2;
}

// ============================================================================
// Comparisons as sub-terms: if (a==b || c>d)
// ============================================================================

int test_cmp_or(int a, int b, int c, int d) {
    if (a == b || c > d) return 1;
    return 0;
}

// ============================================================================
// Value context: r = a || b  (no boxing elimination expected; must still work)
// ============================================================================

int test_or_value(int a, int b) {
    return a || b;
}

int test_and_value(int a, int b) {
    return a && b;
}

// ============================================================================
// Main
// ============================================================================

void main() {
    int r;
    printf("=== Boolean boxing elimination tests ===\n");

    // -- Pattern A: if (a || b) --
    check("or_if(0,0)==0",  !test_or_if(0, 0));
    check("or_if(1,0)==1",   test_or_if(1, 0));
    check("or_if(0,1)==1",   test_or_if(0, 1));
    check("or_if(1,1)==1",   test_or_if(1, 1));

    // -- Pattern B: if (a && b) --
    check("and_if(0,0)==0", !test_and_if(0, 0));
    check("and_if(1,0)==0", !test_and_if(1, 0));
    check("and_if(0,1)==0", !test_and_if(0, 1));
    check("and_if(1,1)==1",  test_and_if(1, 1));

    // -- Pattern A chained 3 terms --
    check("or3(0,0,0)==0", !test_or3(0, 0, 0));
    check("or3(0,0,1)==1",  test_or3(0, 0, 1));
    check("or3(0,1,0)==1",  test_or3(0, 1, 0));
    check("or3(1,0,0)==1",  test_or3(1, 0, 0));

    // -- Pattern B chained 3 terms --
    check("and3(1,1,1)==1",  test_and3(1, 1, 1));
    check("and3(1,1,0)==0", !test_and3(1, 1, 0));
    check("and3(1,0,1)==0", !test_and3(1, 0, 1));
    check("and3(0,1,1)==0", !test_and3(0, 1, 1));

    // -- Mixed: a || b && c --
    check("mixed(1,0,0)==1",  test_mixed(1, 0, 0));  // a true -> true
    check("mixed(0,1,1)==1",  test_mixed(0, 1, 1));  // b&&c true -> true
    check("mixed(0,1,0)==0", !test_mixed(0, 1, 0));  // b&&c false, a false -> false
    check("mixed(0,0,1)==0", !test_mixed(0, 0, 1));  // b&&c false, a false -> false

    // -- while (a || b) --
    check("or_while(1,0)==1",  test_or_while(1, 0) == 1);
    check("or_while(0,1)==1",  test_or_while(0, 1) == 1);
    check("or_while(0,0)==0",  test_or_while(0, 0) == 0);

    // -- for (;a&&b;) --
    check("and_for(1,1)==1",  test_and_for(1, 1) == 1);
    check("and_for(1,0)==0",  test_and_for(1, 0) == 0);
    check("and_for(0,1)==0",  test_and_for(0, 1) == 0);

    // -- ternary (a || b) ? 1 : 2 --
    check("or_tern(0,0)==2",  test_or_ternary(0, 0) == 2);
    check("or_tern(1,0)==1",  test_or_ternary(1, 0) == 1);
    check("or_tern(0,1)==1",  test_or_ternary(0, 1) == 1);

    // -- comparisons as sub-terms --
    check("cmp_or(1,1,0,5)==1",  test_cmp_or(1, 1, 0, 5));   // a==b true
    check("cmp_or(1,2,5,0)==1",  test_cmp_or(1, 2, 5, 0));   // c>d true
    check("cmp_or(1,2,0,5)==0", !test_cmp_or(1, 2, 0, 5));   // both false

    // -- value context: result must still be correct 0/1 --
    r = test_or_value(0, 0);  check("or_val(0,0)==0",  r == 0);
    r = test_or_value(1, 0);  check("or_val(1,0)!=0",  r != 0);
    r = test_or_value(0, 1);  check("or_val(0,1)!=0",  r != 0);
    r = test_and_value(0, 1); check("and_val(0,1)==0", r == 0);
    r = test_and_value(1, 1); check("and_val(1,1)!=0", r != 0);

    printf("\n%d passed, %d failed\n", passed, failed);
}
