// knr.c -- Test program for K&R-style function argument declarations in Small-C.
//
// Purpose:
//   Verify that the compiler correctly handles K&R (pre-ANSI) style function
//   definitions, where argument types are declared after the parameter name
//   list and before the function body.
//
//   This exercises the three-phase argument processing in doArgsNonTyped:
//     Phase 1: argument names collected with placeholder types.
//     Phase 2: doArgTypes patches IDENT/TYPE/SIZE from the declaration lines.
//     Phase 3: fixArgOffs() recomputes true BP-relative offsets.
//
// Functionality covered:
//   - Single int argument
//   - Two ints on one declaration line (int a, b;)
//   - Three ints on one declaration line
//   - char argument (type narrower than int)
//   - int pointer argument
//   - long argument (exercises fixArgOffs -- 4-byte slot)
//   - Mixed long + int arguments (fixArgOffs must push int offset past 4-byte long)
//   - Array argument (decays to pointer via IDENT_POINTER path)
//   - Struct argument (hidden-pointer transformation: IDENT_VARIABLE -> IDENT_POINTER)
//   - Struct argument member access via the hidden pointer

#include <stdio.h>

int passed, failed;

// ============================================================================
// Struct used by struct-argument tests
// ============================================================================

struct pt {
    int x, y;
};

// ============================================================================
// K&R-style test functions
// ============================================================================

// Single int argument.
int identity(n)
int n;
{
    return n;
}

// Two ints declared on one line: int a, b;
int sub(a, b)
int a, b;
{
    return a - b;
}

// Three ints declared on one line.
int sum3(a, b, c)
int a, b, c;
{
    return a + b + c;
}

// char argument.
int charval(c)
char c;
{
    return c;
}

// Pointer argument.
int deref(p)
int *p;
{
    return *p;
}

// long argument -- fixArgOffs must allocate a 4-byte BP slot before returning.
long longid(n)
long n;
{
    return n;
}

// Mixed: long followed by int.  fixArgOffs must place 'small' past the 4-byte
// slot occupied by 'big', not just 2 bytes past it.
int longplus(big, small)
long big;
int small;
{
    return (int)big + small;
}

// Array argument: decays to pointer, so arr[0] is readable.
int arrfirst(arr)
int arr[];
{
    return arr[0];
}

// Struct argument: hidden-pointer transform (IDENT_VARIABLE -> IDENT_POINTER).
// The caller passes the address of the struct; the callee receives a pointer.
int getx(s)
struct pt s;
{
    return s.x;
}

int gety(s)
struct pt s;
{
    return s.y;
}

// ============================================================================
// Test harness
// ============================================================================

void check(desc, cond)
char *desc;
int cond;
{
    if (cond) {
        printf("  PASS: %s\n", desc);
        passed++;
    }
    else {
        printf("  FAIL: %s\n", desc);
        failed++;
        getchar();
    }
}

// ============================================================================
// main
// ============================================================================

main() {
    int arr[3];
    struct pt p;
    long lv;

    passed = 0;
    failed = 0;

    printf("=== K&R argument tests ===\n");

    // Single int
    check("identity(7) == 7",         identity(7) == 7);
    check("identity(-1) == -1",        identity(-1) == -1);
    check("identity(0) == 0",          identity(0) == 0);

    // Two ints on one declaration line
    check("sub(10,3) == 7",            sub(10, 3) == 7);
    check("sub(0,5) == -5",            sub(0, 5) == -5);

    // Three ints on one declaration line
    check("sum3(1,2,3) == 6",          sum3(1, 2, 3) == 6);
    check("sum3(-1,0,1) == 0",         sum3(-1, 0, 1) == 0);

    // char argument
    check("charval('A') == 'A'",       charval('A') == 'A');
    check("charval(0) == 0",           charval(0) == 0);

    // Pointer argument
    arr[0] = 42;
    check("deref(&arr[0]) == 42",      deref(&arr[0]) == 42);

    // long argument
    lv = 70000L;
    check("longid(70000L) == 70000L",  longid(70000L) == 70000L);
    lv = -1L;
    check("longid(-1L) == -1L",        longid(-1L) == -1L);

    // Mixed long + int (critical for fixArgOffs)
    check("longplus(100L,5) == 105",   longplus(100L, 5) == 105);
    check("longplus(-1L,1) == 0",      longplus(-1L, 1) == 0);

    // Array argument (decays to pointer)
    arr[0] = 99;
    check("arrfirst(arr) == 99",       arrfirst(arr) == 99);

    // Struct argument (hidden pointer)
    p.x = 3;
    p.y = 7;
    check("getx(p) == 3",             getx(p) == 3);
    check("gety(p) == 7",             gety(p) == 7);

    // Summary
    printf("\n=== Results: %d passed, %d failed ===\n", passed, failed);
    if (failed)
        printf("*** FAILURES DETECTED ***\n");
    else
        printf("All tests passed.\n");
}
