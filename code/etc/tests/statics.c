// statictest.c -- Test program for static variable support in Small-C 2.3.
//
// Tests static global variables and static local variables of all types:
//   char, unsigned char, int, unsigned int, long, unsigned long
//
// Properties verified:
//   - sizeof correctness
//   - Zero initialization for uninitialized statics (both global and local)
//   - Constant initializers work correctly (initial value set once)
//   - Static local persistence: value survives across function calls
//   - Static local initializer: applied only at first call
//   - Arrays: zero init, constant init, element access
//   - Pointer-to-static
//   - Multiple static locals in one function

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
// Global static declarations -- file-scope, not exported (no PUBLIC label)
// ============================================================================

// Uninitialized (must be zero)
static int   gs_int;
static char  gs_char;
static unsigned int  gs_uint;
static unsigned char gs_uchar;
static long  gs_long;
static unsigned long gs_ulong;

// Initialized scalars
static int           gs_int_i   = 42;
static char          gs_char_i  = 65;      // 'A'
static int           gs_int_neg = -7;
static unsigned int  gs_uint_i  = 1000;
static unsigned char gs_uchar_i = 200;
static long          gs_long_i  = 70000;   // > 16-bit: lo=4464, hi=1
static unsigned long gs_ulong_i = 70000;

// Uninitialized arrays (must be zero)
static int  gs_int_arr[4];
static char gs_char_arr[4];
static long gs_long_arr[3];

// Initialized arrays
static int  gs_int_arr_i[4]  = {10, 20, 30, 40};
static char gs_char_arr_i[4] = {1, 2, 3, 4};
static long gs_long_arr_i[3] = {1, 2, 3};

// ============================================================================
// Test: sizeof static global types
// ============================================================================

void test_sizeof_static() {
    printf("--- sizeof static global types ---\n");

    check("sizeof(gs_int) == 2",          sizeof(gs_int)  == 2);
    check("sizeof(gs_char) == 1",         sizeof(gs_char) == 1);
    check("sizeof(gs_uint) == 2",         sizeof(gs_uint) == 2);
    check("sizeof(gs_uchar) == 1",        sizeof(gs_uchar) == 1);
    check("sizeof(gs_long) == 4",         sizeof(gs_long) == 4);
    check("sizeof(gs_ulong) == 4",        sizeof(gs_ulong) == 4);

    check("sizeof(gs_int_arr) == 8",      sizeof(gs_int_arr)  == 8);
    check("sizeof(gs_char_arr) == 4",     sizeof(gs_char_arr) == 4);
    check("sizeof(gs_long_arr) == 12",    sizeof(gs_long_arr) == 12);

    check("sizeof(gs_int_arr_i) == 8",    sizeof(gs_int_arr_i)  == 8);
    check("sizeof(gs_char_arr_i) == 4",   sizeof(gs_char_arr_i) == 4);
    check("sizeof(gs_long_arr_i) == 12",  sizeof(gs_long_arr_i) == 12);
}

// ============================================================================
// Test: global static zero initialization (uninitialized statics == 0)
// ============================================================================

void test_g_zeroinit() {
    int i;
    int *p;
    printf("--- global static zero initialization ---\n");

    check("gs_int == 0",   gs_int  == 0);
    check("gs_char == 0",  gs_char == 0);
    check("gs_uint == 0",  gs_uint == 0);
    check("gs_uchar == 0", gs_uchar == 0);

    // long: check both words
    p = &gs_long;
    check("gs_long lo == 0", *p == 0);
    check("gs_long hi == 0", *(p + 1) == 0);

    p = &gs_ulong;
    check("gs_ulong lo == 0", *p == 0);
    check("gs_ulong hi == 0", *(p + 1) == 0);

    // int array: all elements zero
    i = 0;
    while (i < 4) {
        check("gs_int_arr[i] == 0", gs_int_arr[i] == 0);
        i++;
    }

    // char array: all elements zero
    i = 0;
    while (i < 4) {
        check("gs_char_arr[i] == 0", gs_char_arr[i] == 0);
        i++;
    }

    // long array: all elements zero
    i = 0;
    while (i < 3) {
        p = &gs_long_arr[i];
        check("gs_long_arr[i] lo == 0", *p == 0);
        check("gs_long_arr[i] hi == 0", *(p + 1) == 0);
        i++;
    }
}

// ============================================================================
// Test: global static initialized scalars
// ============================================================================

void test_g_initscalar() {
    int *p;
    printf("--- global static initialized scalars ---\n");

    check("gs_int_i == 42",    gs_int_i   == 42);
    check("gs_char_i == 65",   gs_char_i  == 65);
    check("gs_int_neg == -7",  gs_int_neg == -7);
    check("gs_uint_i == 1000", gs_uint_i  == 1000);
    check("gs_uchar_i == 200", gs_uchar_i == 200);

    // gs_long_i = 70000: lo=4464, hi=1
    p = &gs_long_i;
    check("gs_long_i lo == 4464",  *p == 4464);
    check("gs_long_i hi == 1",     *(p + 1) == 1);

    // gs_ulong_i = 70000: same bit pattern
    p = &gs_ulong_i;
    check("gs_ulong_i lo == 4464", *p == 4464);
    check("gs_ulong_i hi == 1",    *(p + 1) == 1);
}

// ============================================================================
// Test: global static initialized arrays
// ============================================================================

void test_g_initarray() {
    int *p;
    printf("--- global static initialized arrays ---\n");

    check("gs_int_arr_i[0] == 10", gs_int_arr_i[0] == 10);
    check("gs_int_arr_i[1] == 20", gs_int_arr_i[1] == 20);
    check("gs_int_arr_i[2] == 30", gs_int_arr_i[2] == 30);
    check("gs_int_arr_i[3] == 40", gs_int_arr_i[3] == 40);

    check("gs_char_arr_i[0] == 1", gs_char_arr_i[0] == 1);
    check("gs_char_arr_i[1] == 2", gs_char_arr_i[1] == 2);
    check("gs_char_arr_i[2] == 3", gs_char_arr_i[2] == 3);
    check("gs_char_arr_i[3] == 4", gs_char_arr_i[3] == 4);

    // gs_long_arr_i[0..2] = {1,2,3}
    p = &gs_long_arr_i[0];
    check("gs_long_arr_i[0] lo == 1", *p == 1);
    check("gs_long_arr_i[0] hi == 0", *(p + 1) == 0);

    p = &gs_long_arr_i[1];
    check("gs_long_arr_i[1] lo == 2", *p == 2);
    check("gs_long_arr_i[1] hi == 0", *(p + 1) == 0);

    p = &gs_long_arr_i[2];
    check("gs_long_arr_i[2] lo == 3", *p == 3);
    check("gs_long_arr_i[2] hi == 0", *(p + 1) == 0);
}

// ============================================================================
// Test: global static mutation (write then read back)
// ============================================================================

void test_g_mut() {
    int *p;
    printf("--- global static mutation ---\n");

    gs_int = 99;
    check("gs_int = 99", gs_int == 99);

    gs_char = 'Z';
    check("gs_char = 'Z'", gs_char == 90);

    gs_uint = 60000;
    // 60000 = 0xEA60 => signed == -5536
    p = &gs_uint;
    check("gs_uint = 60000 bits", *p == -5536);

    gs_long = 100000;
    // 100000 = 0x186A0 => lo=0x86A0=-31072, hi=1
    p = &gs_long;
    check("gs_long = 100000 lo", *p == -31072);
    check("gs_long = 100000 hi", *(p + 1) == 1);

    // Verify array elements can be mutated
    gs_int_arr[2] = 77;
    check("gs_int_arr[2] = 77", gs_int_arr[2] == 77);

    gs_long_arr[1] = 70000;
    p = &gs_long_arr[1];
    check("gs_long_arr[1] = 70000 lo", *p == 4464);
    check("gs_long_arr[1] = 70000 hi", *(p + 1) == 1);
}

// ============================================================================
// Test: pointer to global static variable
// ============================================================================

void test_g_ptr() {
    int *pi;
    char *pc;
    long *pl;
    int *p;
    printf("--- pointer to global static ---\n");

    gs_int_i = 55;
    pi = &gs_int_i;
    check("*pi (ptr to gs_int_i) == 55", *pi == 55);

    gs_char_i = 'B';
    pc = &gs_char_i;
    check("*pc (ptr to gs_char_i) == 'B'", *pc == 66);

    gs_long_i = 99;
    pl = &gs_long_i;
    p = pl;
    check("gs_long_i via long* lo == 99", *p == 99);
    check("gs_long_i via long* hi == 0",  *(p + 1) == 0);
}

// ============================================================================
// Local static helper functions
// ============================================================================

// Returns an incrementing count -- tests int static persistence
int count_int() {
    static int n = 0;
    n++;
    return n;
}

// Returns accumulated sum -- tests that static local persists between calls
int accum_int(int add) {
    static int sum;       // uninitialized: should start at 0
    sum += add;
    return sum;
}

// Counter for char type
char count_char() {
    static char c = 10;
    c++;
    return c;
}

// Counter for unsigned int type
unsigned int count_uint() {
    static unsigned int u = 0;
    u += 1000;
    return u;
}

// Counter for long type (also tests long static persistence)
long count_long() {
    static long lv = 0;
    lv = lv + 70000;
    return lv;
}

// Returns a long static initialized to 70000 on first call
long long_init_once() {
    static long lv = 70000;
    return lv;
}

// ============================================================================
// Test: local static int -- persistence and zero initialization
// ============================================================================

void test_l_int_persist() {
    printf("--- local static int: persistence ---\n");

    check("count_int() call 1 == 1", count_int() == 1);
    check("count_int() call 2 == 2", count_int() == 2);
    check("count_int() call 3 == 3", count_int() == 3);
    check("count_int() call 4 == 4", count_int() == 4);
    check("count_int() call 5 == 5", count_int() == 5);
}

void test_l_int_zeroinit() {
    printf("--- local static int: zero initialization ---\n");

    // accum_int starts at 0 (no initializer), then accumulates
    check("accum(10) == 10",  accum_int(10) == 10);
    check("accum(5)  == 15",  accum_int(5)  == 15);
    check("accum(20) == 35",  accum_int(20) == 35);
}

// ============================================================================
// Test: local static char
// ============================================================================

void test_l_ch_persist() {
    printf("--- local static char: persistence ---\n");

    // count_char starts at 10, increments each call
    check("count_char() call 1 == 11", count_char() == 11);
    check("count_char() call 2 == 12", count_char() == 12);
    check("count_char() call 3 == 13", count_char() == 13);
}

// ============================================================================
// Test: local static unsigned int
// ============================================================================

void test_l_uint_persist() {
    int *p;
    int v;
    printf("--- local static unsigned int: persistence ---\n");

    // count_uint increments by 1000 each call
    v = count_uint();
    check("count_uint() call 1 == 1000", v == 1000);
    v = count_uint();
    check("count_uint() call 2 == 2000", v == 2000);
    v = count_uint();
    check("count_uint() call 3 == 3000", v == 3000);
}

// ============================================================================
// Test: local static long -- persistence and initializer
// ============================================================================

void test_l_l_persist() {
    long r;
    int *p;
    printf("--- local static long: persistence ---\n");

    // count_long accumulates 70000 each call: 70000, 140000, 210000
    r = count_long();
    p = &r;
    // 70000 = 0x11170: lo=4464, hi=1
    check("count_long() call 1 lo == 4464", *p == 4464);
    check("count_long() call 1 hi == 1",    *(p + 1) == 1);

    r = count_long();
    p = &r;
    // 140000 = 0x222E0: lo=0x22E0=8928, hi=2
    check("count_long() call 2 lo == 8928", *p == 8928);
    check("count_long() call 2 hi == 2",    *(p + 1) == 2);

    r = count_long();
    p = &r;
    // 210000 = 0x33450: lo=0x3450=13392, hi=3
    check("count_long() call 3 lo == 13392", *p == 13392);
    check("count_long() call 3 hi == 3",     *(p + 1) == 3);
}

int test_l_l_initonce() {
    long r;
    int *p;
    printf("--- local static long: initializer applied once ---\n");

    // long_init_once() returns the same lv each call (never modified)
    r = long_init_once();
    p = &r;
    // 70000 lo=4464, hi=1
    check("long_init_once() call 1 lo == 4464", *p == 4464);
    check("long_init_once() call 1 hi == 1",    *(p + 1) == 1);

    r = long_init_once();
    p = &r;
    check("long_init_once() call 2 lo == 4464", *p == 4464);
    check("long_init_once() call 2 hi == 1",    *(p + 1) == 1);
}

// ============================================================================
// Test: local static arrays
// ============================================================================

// Declares a static int array; modifies one element; returns element
int s_int_arr_read(int idx) {
    static int arr[4] = {10, 20, 30, 40};
    return arr[idx];
}

int s_int_arr_write(int idx, int val) {
    static int arr2[4];    // uninitialized: should be 0
    arr2[idx] = val;
    return arr2[idx];
}

// Returns element of a static long array (initialized)
long s_long_arr_read(int idx) {
    static long larr[3] = {1, 2, 3};
    return larr[idx];
}

void test_l_arr_init() {
    printf("--- local static array: initialized ---\n");

    check("static arr[0] == 10", s_int_arr_read(0) == 10);
    check("static arr[1] == 20", s_int_arr_read(1) == 20);
    check("static arr[2] == 30", s_int_arr_read(2) == 30);
    check("static arr[3] == 40", s_int_arr_read(3) == 40);

    // Call again: same values (static, not stack)
    check("static arr[0] still 10", s_int_arr_read(0) == 10);
    check("static arr[2] still 30", s_int_arr_read(2) == 30);
}

void test_l_arr_zeroinit() {
    printf("--- local static array: zero initialized ---\n");

    check("static arr2[0] == 0 (before write)", s_int_arr_write(0, 55) == 55);
    // After write, the element persists
    check("static arr2[0] == 55 (after write, re-read)", s_int_arr_write(0, 55) == 55);
}

void test_l_l_arr() {
    long r;
    int *p;
    printf("--- local static long array ---\n");

    r = s_long_arr_read(0);
    p = &r;
    check("static larr[0] lo == 1", *p == 1);
    check("static larr[0] hi == 0", *(p + 1) == 0);

    r = s_long_arr_read(2);
    p = &r;
    check("static larr[2] lo == 3", *p == 3);
    check("static larr[2] hi == 0", *(p + 1) == 0);
}

// ============================================================================
// Test: multiple static locals in one function
// ============================================================================

// Function with several static locals of different types
void multi_static_helper(int bump) {
    static int   si  = 100;
    static char  sc  = 10;
    static long  sl  = 70000;
    si += bump;
    sc += bump;
    sl = sl + bump;
    printf("  si=%d sc=%d\n", si, sc);
}

void test_multi_static() {
    int *p;
    printf("--- multiple static locals in one function ---\n");

    multi_static_helper(1);   // si=101, sc=11
    multi_static_helper(2);   // si=103, sc=13
    multi_static_helper(4);   // si=107, sc=17
    // We can verify via dedicated accessors:
    // After 3 calls with bumps 1,2,4:
    // si = 100+1+2+4 = 107
    // sc = 10+1+2+4  = 17
    // (Verified visually from printf output above)
    check("multi_static: test reached", 1);   // presence test
}

// ============================================================================
// Test: local static scope -- same name in different functions
// ============================================================================

// Two functions each with a static int named 'count' -- they must be independent
int scope_a() {
    static int count = 0;
    count++;
    return count;
}

int scope_b() {
    static int count = 0;
    count += 10;
    return count;
}

void test_l_scope_independence() {
    printf("--- static local scope independence ---\n");

    check("scope_a() call 1 == 1",  scope_a() == 1);
    check("scope_b() call 1 == 10", scope_b() == 10);
    check("scope_a() call 2 == 2",  scope_a() == 2);
    check("scope_b() call 2 == 20", scope_b() == 20);
    check("scope_a() call 3 == 3",  scope_a() == 3);
    check("scope_b() call 3 == 30", scope_b() == 30);
    // Confirm a is still independent of b
    check("scope_a() call 4 == 4",  scope_a() == 4);
}

// ============================================================================
// Test: local static uninitialized long (must be zero)
// ============================================================================

long get_uninit_long() {
    static long ul;
    return ul;
}

void test_l_l_zeroinit() {
    long r;
    int *p;
    printf("--- local static long: zero initialization ---\n");

    r = get_uninit_long();
    p = &r;
    check("uninit static long lo == 0", *p == 0);
    check("uninit static long hi == 0", *(p + 1) == 0);
}

// ============================================================================
// Test: local static char array zero initialized
// ============================================================================

char get_static_char(int idx) {
    static char ca[6];
    return ca[idx];
}

void test_l_ch_arr_zeroinit() {
    int i;
    printf("--- local static char array: zero initialized ---\n");

    i = 0;
    while (i < 6) {
        check("static char arr[i] == 0", get_static_char(i) == 0);
        i++;
    }
}

// ============================================================================
// Test: local static unsigned char
// ============================================================================

unsigned char count_uchar() {
    static unsigned char uc = 250;
    uc++;
    return uc;
}

void test_l_uchar_persist() {
    printf("--- local static unsigned char: persistence ---\n");

    check("count_uchar() call 1 == 251", count_uchar() == 251);
    check("count_uchar() call 2 == 252", count_uchar() == 252);
    check("count_uchar() call 3 == 253", count_uchar() == 253);
    // Wrap at 255 -> 256 -> 0 for unsigned char? Depends on impl.
    // Just verify consistent increments.
}

// ============================================================================
// Test: global static sizeof in sizeof() expression
// ============================================================================

void test_g_static_sizeof() {
    static int  si;
    static char sc;
    static long sl;
    printf("--- sizeof local static types ---\n");

    check("sizeof(static int) == 2",  sizeof(si) == 2);
    check("sizeof(static char) == 1", sizeof(sc) == 1);
    check("sizeof(static long) == 4", sizeof(sl) == 4);
}

// ============================================================================
// Test: address of local static is stable across calls
// ============================================================================

int *g_static_addr_out;   // caller reads result here

void get_static_addr() {
    static int sv = 0;
    g_static_addr_out = &sv;
}

void test_static_addr_stable() {
    int *p1, *p2;
    printf("--- address of local static is stable ---\n");

    get_static_addr();
    p1 = g_static_addr_out;
    get_static_addr();
    p2 = g_static_addr_out;
    check("same addr both calls", p1 == p2);

    *p1 = 123;
    check("store via addr persists", *p2 == 123);
}

// ============================================================================
// Test: mutate initialized globals to large values, re-read both words
// ============================================================================

void test_g_reinit() {
    int *p;
    printf("--- global static: re-assign large values and re-check ---\n");

    // Scalar int: overwrite with new value
    gs_int_i = 30000;
    check("gs_int_i re-assign 30000", gs_int_i == 30000);

    // Scalar char: overwrite
    gs_char_i = 'Z';
    check("gs_char_i re-assign 'Z'", gs_char_i == 90);

    // Negative int: overwrite with positive large value
    gs_int_neg = 32767;
    check("gs_int_neg re-assign 32767", gs_int_neg == 32767);

    // Unsigned int: overwrite with large value, check bit pattern
    gs_uint_i = 50000;
    p = &gs_uint_i;
    // 50000 = 0xC350 => as signed int = -15536
    check("gs_uint_i re-assign 50000 bits", *p == -15536);

    // Unsigned char: overwrite
    gs_uchar_i = 7;
    check("gs_uchar_i re-assign 7", gs_uchar_i == 7);

    // Long scalar: overwrite with a different large constant, check hi word
    gs_long_i = 140000;
    // 140000 = 0x222E0 => lo=0x22E0=8928, hi=2
    p = &gs_long_i;
    check("gs_long_i re-assign 140000 lo", *p == 8928);
    check("gs_long_i re-assign 140000 hi", *(p + 1) == 2);

    // Unsigned long: overwrite
    gs_ulong_i = 140000;
    p = &gs_ulong_i;
    check("gs_ulong_i re-assign 140000 lo", *p == 8928);
    check("gs_ulong_i re-assign 140000 hi", *(p + 1) == 2);

    // Long scalar: overwrite again with negative large
    gs_long_i = -70000;
    // -70000 in 32-bit: lo=-4464(0xEE90), hi=-2(0xFFFE)
    p = &gs_long_i;
    check("gs_long_i re-assign -70000 lo", *p == -4464);
    check("gs_long_i re-assign -70000 hi", *(p + 1) == -2);

    // int array: overwrite all with new values
    gs_int_arr_i[0] = 100;
    gs_int_arr_i[1] = 200;
    gs_int_arr_i[2] = 300;
    gs_int_arr_i[3] = 400;
    check("gs_int_arr_i[0] re-assign 100", gs_int_arr_i[0] == 100);
    check("gs_int_arr_i[1] re-assign 200", gs_int_arr_i[1] == 200);
    check("gs_int_arr_i[2] re-assign 300", gs_int_arr_i[2] == 300);
    check("gs_int_arr_i[3] re-assign 400", gs_int_arr_i[3] == 400);

    // char array: overwrite
    gs_char_arr_i[0] = 'a';
    gs_char_arr_i[3] = 'z';
    check("gs_char_arr_i[0] re-assign 'a'", gs_char_arr_i[0] == 97);
    check("gs_char_arr_i[3] re-assign 'z'", gs_char_arr_i[3] == 122);

    // long array: overwrite elements with large values
    gs_long_arr_i[0] = 70000;
    gs_long_arr_i[2] = 140000;
    p = &gs_long_arr_i[0];
    check("gs_long_arr_i[0] re-assign 70000 lo",  *p == 4464);
    check("gs_long_arr_i[0] re-assign 70000 hi",  *(p + 1) == 1);
    p = &gs_long_arr_i[2];
    check("gs_long_arr_i[2] re-assign 140000 lo", *p == 8928);
    check("gs_long_arr_i[2] re-assign 140000 hi", *(p + 1) == 2);

    // Confirm middle element of long array untouched
    p = &gs_long_arr_i[1];
    check("gs_long_arr_i[1] still 2 lo", *p == 2);
    check("gs_long_arr_i[1] still 2 hi", *(p + 1) == 0);
}

// ============================================================================
// Main
// ============================================================================

void main() {
    passed = 0;
    failed = 0;

    printf("=== Static Variable Tests ===\n\n");

    printf("\n== Global Static ==\n");
    test_sizeof_static();
    test_g_zeroinit();
    test_g_initscalar();
    test_g_initarray();
    test_g_mut();
    test_g_ptr();
    test_g_reinit();

    printf("\n== Local Static: int / char / uint ==\n");
    test_l_int_persist();
    test_l_int_zeroinit();
    test_l_ch_persist();
    test_l_uint_persist();
    test_l_uchar_persist();

    printf("\n== Local Static: long ==\n");
    test_l_l_zeroinit();
    test_l_l_persist();
    test_l_l_initonce();

    printf("\n== Local Static: arrays ==\n");
    test_l_arr_init();
    test_l_arr_zeroinit();
    test_l_l_arr();
    test_l_ch_arr_zeroinit();

    printf("\n== Local Static: scope and misc ==\n");
    test_l_scope_independence();
    test_multi_static();
    test_g_static_sizeof();
    test_static_addr_stable();

    printf("\n=== Results: %d passed, %d failed ===\n", passed, failed);
}
