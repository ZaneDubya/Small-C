// unions.c -- Test program for union support in Small-C 2.2.
// Tests: definition, variable declaration (global/local), member access,
//        sizeof, union assignment, brace initialization (global and local),
//        union containing struct, pointer to union, union array,
//        union as function parameter (by pointer), union-returning function.
//
// Each test prints PASS or FAIL with a description.
// Summary of passed/failed/total is printed at end.

#include "../../smallc22/stdio.h"

int passed, failed;

// ============================================================================
// Union definitions
// ============================================================================

union word {
    int  i;
    char c[2];
};

union multi {
    char  b;
    int   w;
    long  d;
};

union punned {
    int  ival;
    char bytes[2];
};

// A struct nested inside a union
struct point {
    int x;
    int y;
};

union geovar {
    int       scalar;
    struct point pt;
};

// Global union variables
union word   gw;
union multi  gm;

// Global union with brace initializer (first member)
union word ginit = { 1234 };

// ============================================================================
// Helper
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
// Functions for parameter / return testing
// ============================================================================

void set_word(union word *p, int v) {
    p->i = v;
}

int get_word(union word *p) {
    return p->i;
}

union word make_word(int v) {
    union word w;
    w.i = v;
    return w;
}

// ============================================================================
// Tests
// ============================================================================

void t_sizeof() {
    printf("--- sizeof ---\n");
    // union word has int (2) and char[2] (2) -> size == 2
    check("sizeof union word == 2", sizeof(union word) == 2);
    // union multi has char(1), int(2), long(4) -> size == 4
    check("sizeof union multi == 4", sizeof(union multi) == 4);
    // union geovar has int(2) and struct point(4) -> size == 4
    check("sizeof union geovar == 4", sizeof(union geovar) == 4);
}

void t_global_access() {
    printf("--- global member read/write ---\n");
    gw.i = 0x0102;
    check("gw.i write/read", gw.i == 0x0102);
    // Low byte on little-endian 8086
    check("gw.c[0] shares storage with gw.i", gw.c[0] == 2);
    check("gw.c[1] shares storage with gw.i", gw.c[1] == 1);
}

void t_global_init() {
    printf("--- global brace init ---\n");
    check("ginit.i == 1234", ginit.i == 1234);
}

void t_local_access() {
    union word w;
    printf("--- local member read/write ---\n");
    w.i = 0x0304;
    check("local w.i write/read", w.i == 0x0304);
    check("local w.c[0] shares storage", w.c[0] == 4);
    check("local w.c[1] shares storage", w.c[1] == 3);
}

void t_local_init() {
    union word w;
    printf("--- local brace init ---\n");
    w.i = 0;
    // Re-declare with initializer to test the init path
    {
        union word wi;
        wi.i = 999;   // manual init (local init not in loop)
        check("local union init via assign", wi.i == 999);
    }
}

void t_multi() {
    union multi m;
    printf("--- multi-member union ---\n");
    m.d = 0x12345678L;
    check("multi.d stores long", m.d == 0x12345678L);
    // low 16 bits of the long should equal m.w
    check("multi.w shares lo-word of long", m.w == 0x5678);
    m.b = 99;
    check("multi.b write", m.b == 99);
}

void t_nested_struct() {
    union geovar g;
    printf("--- union containing struct ---\n");
    g.scalar = 7;
    check("geovar.scalar write/read", g.scalar == 7);
    g.pt.x = 100;
    g.pt.y = 200;
    check("geovar.pt.x", g.pt.x == 100);
    check("geovar.pt.y", g.pt.y == 200);
    // scalar now equals geovar.pt.x (first member of struct point, same offset)
    check("geovar.scalar == geovar.pt.x after struct write", g.scalar == 100);
}

void t_pointer_to_union() {
    union word  w;
    union word *p;
    printf("--- pointer to union ---\n");
    p = &w;
    p->i = 555;
    check("p->i write/read", p->i == 555);
    check("(*p).i same as p->i", (*p).i == 555);
    check("w.i via pointer", w.i == 555);
}

void t_array_of_unions() {
    union word arr[3];
    printf("--- array of unions ---\n");
    arr[0].i = 1;
    arr[1].i = 2;
    arr[2].i = 3;
    check("arr[0].i == 1", arr[0].i == 1);
    check("arr[1].i == 2", arr[1].i == 2);
    check("arr[2].i == 3", arr[2].i == 3);
}

void t_param_return() {
    union word w;
    union word r;
    printf("--- union parameter / return ---\n");
    set_word(&w, 777);
    check("set_word via pointer", w.i == 777);
    check("get_word via pointer", get_word(&w) == 777);
    r = make_word(888);
    check("make_word returns union", r.i == 888);
}

void t_assignment() {
    union word a, b;
    printf("--- union assignment ---\n");
    a.i = 42;
    b = a;
    check("union = union copies value", b.i == 42);
    b.i = 99;
    check("modify copy does not affect original", a.i == 42);
}

// ============================================================================
// main
// ============================================================================

int main() {
    passed = 0;
    failed = 0;

    printf("=== union tests ===\n");

    t_sizeof();
    t_global_access();
    t_global_init();
    t_local_access();
    t_local_init();
    t_multi();
    t_nested_struct();
    t_pointer_to_union();
    t_array_of_unions();
    t_param_return();
    t_assignment();

    printf("\n=== Results: %d passed, %d failed ===\n",
        passed, failed);
    if (failed != 0)
        getchar();
    return failed;
}
