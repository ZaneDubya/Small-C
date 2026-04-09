// typedef.c -- Test program for typedef support in Small-C.

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

void checkval(char *desc, int actual, int expected) {
    if (actual == expected) {
        printf("  PASS: %s\n", desc);
        passed++;
    }
    else {
        printf("  FAIL: %s (got %d, expected %d)\n", desc, actual, expected);
        failed++;
        getchar();
    }
}

void checkuval(char *desc, unsigned int actual, unsigned int expected) {
    if (actual == expected) {
        printf("  PASS: %s\n", desc);
        passed++;
    }
    else {
        printf("  FAIL: %s (got %u, expected %u)\n", desc, actual, expected);
        failed++;
        getchar();
    }
}

// ============================================================================
// Global typedef declarations
// ============================================================================

typedef int     myint;
typedef char    mychar;
typedef long         mylong;
typedef long int     mylongint;
typedef unsigned long myulong;
typedef unsigned long int myulongint;
typedef unsigned int myuint;
typedef char    *mystr;

struct pt {
    int x;
    int y;
};
typedef struct pt Point;
typedef Point   *PtPtr;

// Global variables using typedef'd types
myint   gi;
mychar  gc;
mylong    gl;
mylongint gli;
myulong   gul;
myulongint guli;
myuint  gu;
mystr   gs;
Point   gpt;
PtPtr   gpp;

// ============================================================================
// sizeof tests
// ============================================================================

void test_sizeof() {
    printf("--- sizeof tests ---\n");

    checkval("sizeof(myint)",  sizeof(myint),  2);
    checkval("sizeof(mychar)", sizeof(mychar), 1);
    checkval("sizeof(mylong)",    sizeof(mylong),    4);
    checkval("sizeof(mylongint)", sizeof(mylongint), 4);
    checkval("sizeof(myulong)",   sizeof(myulong),   4);
    checkval("sizeof(myulngint)", sizeof(myulongint),4);
    checkval("sizeof(myuint)",    sizeof(myuint),    2);
    checkval("sizeof(mystr)",  sizeof(mystr),  2);
    checkval("sizeof(Point)",  sizeof(Point),  4);
    checkval("sizeof(PtPtr)",  sizeof(PtPtr),  2);

    checkval("sizeof(gi)",  sizeof(gi),  2);
    checkval("sizeof(gc)",  sizeof(gc),  1);
    checkval("sizeof(gl)",   sizeof(gl),  4);
    checkval("sizeof(gli)",  sizeof(gli), 4);
    checkval("sizeof(gul)",  sizeof(gul), 4);
    checkval("sizeof(guli)", sizeof(guli),4);
    checkval("sizeof(gu)",   sizeof(gu),  2);
    checkval("sizeof(gpt)", sizeof(gpt), 4);
}

// ============================================================================
// myint: arithmetic and assignment
// ============================================================================

void test_myint() {
    myint a, b, c;
    printf("--- myint tests ---\n");

    a = 10;
    b = 3;
    c = a + b;
    check("myint add", c == 13);

    c = a - b;
    check("myint sub", c == 7);

    c = a * b;
    check("myint mul", c == 30);

    c = a / b;
    check("myint div", c == 3);

    c = a % b;
    check("myint mod", c == 1);

    gi = 42;
    check("myint global assign", gi == 42);
}

// ============================================================================
// mychar: assignment and arithmetic
// ============================================================================

void test_mychar() {
    mychar a, b;
    printf("--- mychar tests ---\n");

    a = 'A';
    check("mychar assign 'A'", a == 65);

    b = a + 1;
    check("mychar add 1 == 'B'", b == 66);

    gc = 'z';
    check("mychar global assign", gc == 122);
}

// ============================================================================
// mylong: arithmetic
// ============================================================================

void test_mylong() {
    mylong a, b, c;
    mylongint d, e, f;
    printf("--- mylong tests ---\n");

    a = 100000;
    b = 200000;
    c = a + b;
    check("mylong add", c == 300000);

    c = b - a;
    check("mylong sub", c == 100000);

    a = 1000;
    b = 1000;
    c = a * b;
    check("mylong mul", c == 1000000);

    d = 100000;
    e = 200000;
    f = d + e;
    check("mylongint add", f == 300000);

    f = e - d;
    check("mylongint sub", f == 100000);

    gl = 99999;
    check("mylong global assign", gl == 99999);

    gli = 99999;
    check("mylongint global assign", gli == 99999);
}

// ============================================================================
// myulong / myulongint: unsigned long arithmetic
// ============================================================================

void test_myulong() {
    myulong a, b, c;
    myulongint d;
    printf("--- myulong tests ---\n");

    a = 100000;
    b = 200000;
    c = a + b;
    check("myulong add", c == 300000);

    c = b - a;
    check("myulong sub", c == 100000);

    a = 1000;
    b = 1000;
    c = a * b;
    check("myulong mul", c == 1000000);

    gul = 999999;
    check("myulong global assign", gul == 999999);

    d = 123456;
    check("myulongint local", d == 123456);

    guli = 654321;
    check("myulongint global", guli == 654321);
}

// ============================================================================
// myuint: unsigned arithmetic
// ============================================================================

void test_myuint() {
    myuint a, b;
    printf("--- myuint tests ---\n");

    a = 50000;
    b = 10000;
    printf("  a=%u b=%u a+b=%u a-b=%u\n", a, b, a + b, a - b);
    checkuval("myuint add", a + b, 60000);
    checkuval("myuint sub", a - b, 40000);

    gu = 65535;
    printf("  gu=%u\n", gu);
    checkuval("myuint global max", gu, 65535);
}

// ============================================================================
// mystr: pointer typedef
// ============================================================================

void test_mystr() {
    mystr s;
    printf("--- mystr tests ---\n");

    s = "hello";
    check("mystr assign", s[0] == 'h');
    check("mystr index 1", s[1] == 'e');

    gs = "world";
    check("mystr global assign", gs[0] == 'w');
}

// ============================================================================
// Point (typedef struct): member access and sizeof
// ============================================================================

void test_point() {
    Point p;
    printf("--- Point tests ---\n");

    p.x = 3;
    p.y = 4;
    check("Point member x", p.x == 3);
    check("Point member y", p.y == 4);

    gpt.x = 10;
    gpt.y = 20;
    check("Point global x", gpt.x == 10);
    check("Point global y", gpt.y == 20);
}

// ============================================================================
// PtPtr (typedef pointer-to-struct)
// ============================================================================

void test_ptptr() {
    Point p;
    PtPtr pp;
    printf("--- PtPtr tests ---\n");

    p.x = 7;
    p.y = 9;
    pp = &p;
    check("PtPtr -> x", pp->x == 7);
    check("PtPtr -> y", pp->y == 9);

    pp->x = 100;
    check("PtPtr write x", p.x == 100);

    gpp = &gpt;
    gpt.x = 55;
    check("PtPtr global ->x", gpp->x == 55);
}

// ============================================================================
// Local typedef usage (typedef type used inside a function body)
// ============================================================================

void test_localuse() {
    myint      li;
    mylong     ll;
    mylongint  lli;
    myulong    lul;
    myulongint luli;
    mychar     lc;
    Point      lp;
    printf("--- local use tests ---\n");

    li = -5;
    check("local myint neg", li == -5);

    ll = 1000000;
    check("local mylong big", ll == 1000000);

    lli = 1000000;
    check("local mylongint big", lli == 1000000);

    lul = 1000000;
    check("local myulong big", lul == 1000000);

    luli = 1000000;
    check("local myulongint big", luli == 1000000);

    lc = 'X';
    check("local mychar", lc == 88);

    lp.x = 1;
    lp.y = 2;
    check("local Point x", lp.x == 1);
    check("local Point y", lp.y == 2);
}

// ============================================================================
// main
// ============================================================================

void main() {
    passed = 0;
    failed = 0;

    test_sizeof();
    test_myint();
    test_mychar();
    test_mylong();
    test_myulong();
    test_myuint();
    test_mystr();
    test_point();
    test_ptptr();
    test_localuse();

    printf("\n");
    printf("passed: %d\n", passed);
    printf("failed: %d\n", failed);
}
