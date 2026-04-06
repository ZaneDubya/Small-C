// proto.c -- Runtime tests for C90 function prototype support.
//
// What is tested:
//   1. Forward prototype (same file): call before definition; prototype
//      suppresses "implicit declaration" warning.
//   2. Forward prototype with pointer return type.
//   3. f(void) explicit empty parameter list.
//   4. Variadic prototype (int f(int a, ...)) -- call with fixed arg only,
//      and with extra args; neither should warn.
//   5. Function defined before its call: param types are recorded even
//      without a preceding prototype, so subsequent calls are checked.
//   6. Re-declaration of the same prototype is silently ignored (no crash,
//      no double EXTRN).
//
// Warning-only scenarios (verified by inspecting compiler stderr, not at
// runtime):
//   W1. Calling a function with no prior declaration produces
//       "implicit declaration of function".
//   W2. Calling a prototyped function with the wrong number of fixed
//       arguments produces "wrong number of arguments".
//   W3. Calling a variadic function with fewer than nfixed arguments
//       produces "too few arguments for variadic function".
//
// Build:
//   cc proto
//   asm proto /p
//   ylink proto.obj,<clib.lib> -e=proto.exe

#include "../../smallc22/stdio.h"

// ============================================================================
// Forward prototypes (functions defined later in this file)
// ============================================================================
int add(int a, int b);
int negate(int n);
char *tagname(int tag);
int identity(void);
int summer(int base, ...);

// A second declaration of add() is legal C90 and must be silently ignored.
int add(int a, int b);

// ============================================================================
// Helpers
// ============================================================================
int passed, failed;

void check(char *desc, int val, int expected) {
    if (val == expected) {
        printf("  PASS: %s\n", desc);
        passed++;
    }
    else {
        printf("  FAIL: %s  (got %d, expected %d)\n", desc, val, expected);
        failed++;
    }
}

void checkptr(char *desc, char *val, char *expected) {
    if (strcmp(val, expected) == 0) {
        printf("  PASS: %s\n", desc);
        passed++;
    }
    else {
        printf("  FAIL: %s  (got \"%s\", expected \"%s\")\n",
               desc, val, expected);
        failed++;
    }
}

// ============================================================================
// A function defined BEFORE it is called -- no prototype needed, but the
// compiler must record its param types so calls below are checked.
// ============================================================================
int mul(int a, int b) {
    return a * b;
}

// ============================================================================
// main
// ============================================================================
void main() {
    passed = 0;
    failed = 0;
    printf("=== Function Prototype Tests ===\n\n");

    // --- Test 1: forward proto, call before definition ---
    check("forward proto: add(3, 4) == 7",       add(3, 4),    7);
    check("forward proto: add(0, 0) == 0",        add(0, 0),    0);
    check("forward proto: add(-1, 1) == 0",       add(-1, 1),   0);
    check("forward proto: negate(5) == -5",       negate(5),   -5);
    check("forward proto: negate(-3) == 3",       negate(-3),   3);

    // --- Test 2: forward proto with pointer return ---
    checkptr("forward proto ptr return: tagname(0)", tagname(0), "zero");
    checkptr("forward proto ptr return: tagname(1)", tagname(1), "one");
    checkptr("forward proto ptr return: tagname(2)", tagname(2), "other");

    // --- Test 3: f(void) explicit empty param list ---
    check("f(void): identity() == 42", identity(), 42);

    // --- Test 4: variadic prototype ---
    // Call with exactly the fixed argument -- should not warn.
    check("variadic: summer(10) == 10",   summer(10),    10);
    // Call with extra arguments -- valid for variadic; should not warn.
    check("variadic: summer(5, 1) == 5",  summer(5, 1),  5);
    check("variadic: summer(7, 1, 2) == 7", summer(7, 1, 2), 7);

    // --- Test 5: function defined before call, no prior prototype ---
    // mul() was defined above; its param types were recorded at definition
    // time, so these calls are type-checked.
    check("def-before-call: mul(3, 4) == 12",  mul(3, 4),  12);
    check("def-before-call: mul(0, 5) == 0",   mul(0, 5),   0);
    check("def-before-call: mul(-2, 3) == -6", mul(-2, 3), -6);

    // -----------------------------------------------------------------------
    printf("\nResults: %d passed, %d failed\n", passed, failed);
}

// ============================================================================
// Definitions of the forward-prototyped functions
// ============================================================================
int add(int a, int b) {
    return a + b;
}

int negate(int n) {
    return -n;
}

char *tagname(int tag) {
    if (tag == 0) return "zero";
    if (tag == 1) return "one";
    return "other";
}

// f(void): the compiler must parse this as a zero-parameter definition and
// must not produce any "parameter cannot have void type" error.
int identity(void) {
    return 42;
}

// Variadic: only the fixed argument 'base' is used by the body.
// The '...' causes protoVariadic=1 so calls with extra args are not warned.
int summer(int base, ...) {
    return base;
}
