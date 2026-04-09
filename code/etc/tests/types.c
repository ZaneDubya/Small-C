// typing.c -- Test program for type qualifier support in Small-C 2.3.
// Tests: signed, short, const, volatile, register qualifiers.
//
// NOTE on const-violation tests: const assignment/increment errors are
// caught at *compile* time, so they cannot be tested in a running program.
// To verify them manually, uncomment the blocks marked "COMPILE ERROR TEST"
// one at a time and confirm the compiler emits "cannot assign to const
// variable" or "cannot modify const variable".

#include "../../smallc22/stdio.h"

int passed, failed;

// ============================================================================
// Test helper
// ============================================================================

void check(char *desc, int cond) {
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
// Global declarations -- these test that the compiler accepts the new
// qualifiers and produces the correct types (verified via sizeof, value tests,
// and pointer arithmetic).
// ============================================================================

// signed: same storage as plain int/char/long
signed int    gsi;
signed char   gsc;
signed long   gsl;

// signed with optional 'int' keyword
signed        gsi2;

// short: same as int on 8086 (16-bit word)
short         gs;
unsigned short gus;
short int     gsi3;

// const globals with initializers
const int     gci  = 100;
const char    gcc  = 'X';
const long    gcl  = 99999L;
const unsigned int  gcui = 65000;

// volatile and register are accepted, no storage effect
volatile int  gvi;
volatile long gvl;

// struct members are unaffected
struct qtest {
    signed int  si;
    short       sh;
    unsigned short ush;
    int         normal;
};

// ============================================================================
// Test: sizeof for signed and short
// ============================================================================

void test_sizeof() {
    printf("--- sizeof: signed and short ---\n");

    check("sizeof(signed) == 2",        sizeof(signed) == 2);
    check("sizeof(signed int) == 2",    sizeof(signed int) == 2);
    check("sizeof(signed char) == 1",   sizeof(signed char) == 1);
    check("sizeof(signed long) == 4",   sizeof(signed long) == 4);

    check("sizeof(short) == 2",         sizeof(short) == 2);
    check("sizeof(short int) == 2",     sizeof(short int) == 2);
    check("sizeof(unsigned short) == 2",sizeof(unsigned short) == 2);

    check("sizeof(const int) == 2",     sizeof(const int) == 2);
    check("sizeof(volatile int) == 2",  sizeof(volatile int) == 2);
}

// ============================================================================
// Test: signed arithmetic (same semantics as plain int)
// ============================================================================

void signed_arith() {
    signed int a, b, c;
    signed char sc;
    signed long sl, sl2;

    printf("--- signed arithmetic ---\n");

    a = -5;
    b = 3;
    c = a + b;
    check("signed int: -5 + 3 == -2", c == -2);

    c = a * b;
    check("signed int: -5 * 3 == -15", c == -15);

    c = a / b;
    check("signed int: -6 / 3 == -2", (-6 / 3) == -2);

    sc = -100;
    check("signed char retains negative value", sc == -100);

    // signed long arithmetic
    sl  = -1000000L;
    sl2 = sl + 1L;
    check("signed long: -1000000 + 1 == -999999", sl2 == -999999L);
}

// ============================================================================
// Test: signed vs unsigned comparison behaviour
// ============================================================================

void signed_vs_unsigned() {
    signed int   si;
    unsigned int ui;

    printf("--- signed vs unsigned comparison ---\n");

    si = -1;
    ui = 65535;

    // As signed: -1 < 0 is true
    check("signed: -1 < 0 is true",  si < 0);

    // As unsigned: 65535 > 0 is true
    check("unsigned: 65535 > 0 is true", ui > 0);

    // Cast tests
    check("(signed)-1 < 0",       (signed int)(-1) < 0);
    check("(signed char)-1 < 0",  (signed char)(-1) < 0);
}

// ============================================================================
// Test: short == int on 8086
// ============================================================================

void test_short() {
    short s;
    unsigned short us;
    short int si;

    printf("--- short type (== int on 8086) ---\n");

    s = 32767;
    check("short max == 32767", s == 32767);

    s = -1;
    check("short -1 < 0 (signed)", s < 0);

    us = 65535;
    check("unsigned short max == 65535", us == 65535);
    check("unsigned short: 65535 > 0", us > 0);

    si = -500;
    check("short int: -500 == -500", si == -500);

    // sizeof
    check("sizeof(s) == 2",  sizeof(s) == 2);
    check("sizeof(us) == 2", sizeof(us) == 2);

    // sizeof a pointer-to-short is 2 (all pointers are 16-bit on 8086)
    check("sizeof(short *) == 2", sizeof(short *) == 2);
}

// ============================================================================
// Test: const qualifier -- read-only value, correct at runtime
// ============================================================================

void test_const() {
    const int   ci  = 42;
    const char  cc  = 'A';
    const long  cl  = 100000L;

    printf("--- const qualifier ---\n");

    check("const int local == 42",      ci == 42);
    check("const char local == 'A'",    cc == 'A');
    check("const long local == 100000", cl == 100000L);

    check("global const int == 100",    gci == 100);
    check("global const char == 'X'",   gcc == 'X');
    check("global const long == 99999", gcl == 99999L);
    check("global const uint == 65000", gcui == 65000);

    // const values can appear in expressions
    check("const int in expression: ci * 2 == 84", ci * 2 == 84);
    check("global const in expression: gci + 1 == 101", gci + 1 == 101);

    // const int address can be taken (read-only in spirit, no MMU enforcement)
    {
        int *p;
        p = &ci;
        check("can take address of const int", *p == 42);
    }
}

// ============================================================================
// Test: volatile -- accepted, same runtime behaviour as int
// ============================================================================

void test_volatile() {
    volatile int  vi;
    volatile long vl;
    volatile char vc;

    printf("--- volatile qualifier ---\n");

    vi = 55;
    check("volatile int read/write", vi == 55);

    vl = -123456L;
    check("volatile long read/write", vl == -123456L);

    vc = 'Z';
    check("volatile char read/write", vc == 'Z');

    gvi = 1234;
    check("global volatile int read/write", gvi == 1234);
}

// ============================================================================
// Test: register -- treated as auto, same runtime behaviour
// ============================================================================

void test_register() {
    register int  ri;
    register char rc;

    printf("--- register qualifier ---\n");

    ri = 77;
    check("register int read/write", ri == 77);

    rc = 'R';
    check("register char read/write", rc == 'R');

    // address-of a register variable: allowed (no MMU restriction on 8086)
    {
        int *p;
        p = &ri;
        check("can take address of register int", *p == 77);
    }
}

// ============================================================================
// Test: cast expressions with new type combinations
// ============================================================================

void test_casts() {
    int i;
    long l;

    printf("--- casts: signed, short, const ---\n");

    i = -1;
    check("(signed)(-1) == -1",       (signed)i == -1);
    check("(signed int)(-1) < 0",     (signed int)i < 0);
    check("(signed char)(-1) < 0",    (signed char)i < 0);
    check("(short)(-1) == -1",        (short)i == -1);
    check("(unsigned short)(65535) > 0", (unsigned short)(65535) > 0);

    // const qualifier in cast: value is unchanged
    check("(const int)(42) == 42",    (const int)(42) == 42);

    // volatile qualifier in cast: value is unchanged
    check("(volatile int)(7) == 7",   (volatile int)(7) == 7);

    // signed long
    l = -1L;
    check("(signed long)(-1) < 0",    (signed long)l < 0L);
}

// ============================================================================
// Test: struct with signed / short members
// ============================================================================

void test_struct() {
    struct qtest q;

    printf("--- struct with signed/short members ---\n");

    q.si  = -999;
    q.sh  = -400;
    q.ush = 60000;
    q.normal = 7;

    check("struct signed int member", q.si == -999);
    check("struct short member",      q.sh == -400);
    check("struct unsigned short",    q.ush == 60000);
    check("struct normal int",        q.normal == 7);
}

// ============================================================================
// Test: multi-qualifier combinations
// ============================================================================

void multi_qual() {
    const volatile int  cvi  = 10;
    const signed int    csi  = -3;
    const unsigned int  cuii = 7;
    const short         csh  = 5;
    const unsigned short cush = 300;
    const signed long   csl  = -500000L;

    printf("--- multi-qualifier combinations ---\n");

    check("const volatile int == 10",       cvi  == 10);
    check("const signed int == -3",         csi  == -3);
    check("const unsigned int == 7",        cuii == 7);
    check("const short == 5",               csh  == 5);
    check("const unsigned short == 300",    cush == 300);
    check("const signed long == -500000",   csl  == -500000L);
}

// ============================================================================
// Test: function parameters with qualifiers
// ============================================================================

int sum_test_const(const int a, const int b) {
    return a + b;
}

int negate_signed(signed int x) {
    return -x;
}

int short_add(short x, short y) {
    return x + y;
}

void func_params() {
    printf("--- qualified function parameters ---\n");

    check("const params: sum_test_const(3,4) == 7",   sum_test_const(3, 4) == 7);
    check("signed param: negate_signed(-5) == 5", negate_signed(-5) == 5);
    check("short params: short_add(100,-30) == 70", short_add(100, -30) == 70);
}

// ============================================================================
// COMPILE ERROR TESTS (manual, do not uncomment in normal test runs)
// ============================================================================
//
// -- const assignment --
// const_assign_error() {
//     const int x = 5;
//     x = 6;              /* ERROR: cannot assign to const variable */
// }
//
// -- const += --
// const_compound_error() {
//     const int x = 5;
//     x += 1;             /* ERROR: cannot assign to const variable */
// }
//
// -- const pre-increment --
// const_preinc_error() {
//     const int x = 5;
//     ++x;                /* ERROR: cannot modify const variable */
// }
//
// -- const post-increment --
// const_postinc_error() {
//     const int x = 5;
//     x++;                /* ERROR: cannot modify const variable */
// }
//
// -- const function parameter --
// const_param_assign_error(const int x) {
//     x = 99;             /* ERROR: cannot assign to const variable */
// }
//
// -- const global --
// const_global_assign_error() {
//     gci = 0;            /* ERROR: cannot assign to const variable */
// }

// ============================================================================
// Main
// ============================================================================

int main() {
    passed = 0;
    failed = 0;

    printf("=== typing: qualifier support tests ===\n\n");

    test_sizeof();
    signed_arith();
    signed_vs_unsigned();
    test_short();
    test_const();
    test_volatile();
    test_register();
    test_casts();
    test_struct();
    multi_qual();
    func_params();

    printf("\n=== Results: %d passed, %d failed ===\n", passed, failed);
    if (failed) getchar();
    return failed;
}
