// longs.c -- Test program for long int type support in Small-C 2.3.
//
// Purpose:
//   Verify that the compiler correctly handles the 32-bit 'long' integer
//   type, including storage allocation, sizeof, struct layout with long
//   members, initializers, and pointer-to-long semantics.
//
// Functionality covered:
//   - sizeof(long), sizeof(unsigned long), sizeof(long int)
//   - sizeof(long *) equals the machine pointer size (2 bytes on 8086)
//   - Global long and unsigned long scalar variable declarations
//   - Global 1D and 2D long arrays
//   - Global long variable with a constant initializer
//   - Global long array with a brace initializer
//   - Struct containing long members: layout without padding, sizeof
//   - Struct containing a long array member
//   - Global struct instance with long members
//   - Pointer-to-long: sizeof, declaration, assignment from address
//   - Local long variables and local long arrays
//   - Long constants and initializers (values exceeding 16 bits)

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
// Global declarations -- these test that the compiler accepts long types
// and allocates the correct amount of storage (verified via sizeof).
// ============================================================================

long gl;                      // global long variable (4 bytes)
unsigned long gul;            // global unsigned long (4 bytes)
long garr[3];                 // global long array (12 bytes)
long garr2d[2][3];            // global 2d long array (24 bytes)

// Global long with initializer (small constant, fits in 16 bits)
long ginit = 42;

// Global long array with initializer
long garrinit[4] = { 1, 2, 3, 4 };

// Struct containing long members
struct has_long {
    char c;
    long l;
    int i;
};

// Struct with long array member
struct has_long_arr {
    int count;
    long vals[2];
};

// Global struct instance
struct has_long gstruct;

// Pointer to long
long *glp;

// ============================================================================
// Test: sizeof
// ============================================================================

void test_sizeof() {
    printf("--- sizeof tests ---\n");

    check("sizeof(long) == 4", sizeof(long) == 4);
    check("sizeof(unsigned long) == 4", sizeof(unsigned long) == 4);
    check("sizeof(long int) == 4", sizeof(long int) == 4);
    check("sizeof(unsigned long int) == 4", sizeof(unsigned long int) == 4);

    // pointer to long is still 16-bit pointer
    check("sizeof(long *) == 2", sizeof(long *) == 2);

    // global long variable
    check("sizeof(gl) == 4", sizeof(gl) == 4);
    check("sizeof(gul) == 4", sizeof(gul) == 4);

    // global long arrays
    check("sizeof(garr) == 12", sizeof(garr) == 12);
    check("sizeof(garr2d) == 24", sizeof(garr2d) == 24);

    // pointer to long
    check("sizeof(glp) == 2", sizeof(glp) == 2);
}

// ============================================================================
// Test: struct layout with long members
// ============================================================================

void test_struct_sizeof() {
    printf("--- struct sizeof tests ---\n");

    // struct has_long: char(1) + long(4) + int(2) = 7 (no padding)
    check("sizeof(struct has_long) == 7",
        sizeof(struct has_long) == 7);

    check("sizeof(gstruct) == 7", sizeof(gstruct) == 7);

    // struct has_long_arr: int(2) + long[2](8) = 10
    check("sizeof(struct has_long_arr) == 10",
        sizeof(struct has_long_arr) == 10);
}

// ============================================================================
// Test: local long declarations
// ============================================================================

void test_local_decl() {
    long loc;
    long larr[2];
    unsigned long uloc;
    long *lp;

    printf("--- local declaration tests ---\n");

    check("sizeof(loc) == 4", sizeof(loc) == 4);
    check("sizeof(larr) == 8", sizeof(larr) == 8);
    check("sizeof(uloc) == 4", sizeof(uloc) == 4);
    check("sizeof(lp) == 2", sizeof(lp) == 2);
}

// ============================================================================
// Test: local struct with long members
// ============================================================================

void test_local_struct() {
    struct has_long ls;
    struct has_long_arr la;

    printf("--- local struct with long tests ---\n");

    check("sizeof(ls) == 7", sizeof(ls) == 7);
    check("sizeof(la) == 10", sizeof(la) == 10);
}

// ============================================================================
// Test: global long initialization (small values that fit in 16 bits)
// ============================================================================

void test_global_init() {
    int *p;

    printf("--- global long init tests ---\n");

    // Access global long as pair of words via pointer cast trick:
    // treat &ginit as int* and read low word
    p = &ginit;
    check("ginit low word == 42", *p == 42);
    p++;
    check("ginit high word == 0", *p == 0);

    // Check array init: garrinit[0] should be 1, etc.
    p = garrinit;
    check("garrinit[0] low == 1", *p == 1);
    p++;
    check("garrinit[0] high == 0", *p == 0);
    p++;
    check("garrinit[1] low == 2", *p == 2);
    p++;
    check("garrinit[1] high == 0", *p == 0);
    p++;
    check("garrinit[2] low == 3", *p == 3);
    p++;
    check("garrinit[2] high == 0", *p == 0);
    p++;
    check("garrinit[3] low == 4", *p == 4);
    p++;
    check("garrinit[3] high == 0", *p == 0);
}

// ============================================================================
// Test: that 'long' keyword works in different contexts
// ============================================================================

// Function that takes a long parameter (currently passed as 2 bytes on stack,
// will be 4 bytes once Phase 7 is done -- for now just tests parsing)
long ret_long() {
    // Function returning long -- tests that rettype accepts TYPE_LONG
    // Returns value in AX only for now (only low 16 bits)
    return 99;
}

int test_long_func() {
    int r;
    printf("--- long function return type test ---\n");

    // Call function declared as returning long
    r = ret_long();
    check("ret_long() low word == 99", r == 99);
}

// ============================================================================
// Phase 3 tests: Load/Store/Push/Pop for 32-bit values
// ============================================================================

// Globals for load/store tests
long gval;
long gval2;

void test_ld_st_global() {
    int *p;
    printf("--- Phase 3: global load/store ---\n");

    // Write via pointer cast, read back via long
    p = &gval;
    *p = 1234;       // low word
    *(p + 1) = 5;    // high word  => 5*65536 + 1234 = 328914

    // Read low and high words back via pointer to verify store worked
    p = &gval;
    check("gval low word == 1234", *p == 1234);
    check("gval high word == 5", *(p + 1) == 5);

    // Simple long assignment (16-bit constant fits in low word)
    gval2 = 42;
    p = &gval2;
    check("gval2 low word == 42", *p == 42);
    check("gval2 high word == 0", *(p + 1) == 0);
}

void test_ld_st_local() {
    long loc;
    int *p;
    printf("--- Phase 3: local load/store ---\n");

    loc = 100;
    p = &loc;
    check("local long low == 100", *p == 100);
    check("local long high == 0", *(p + 1) == 0);
}

// ============================================================================
// Phase 4 tests: Expressions, Promotion, Operators
// ============================================================================

long gadd1, gadd2, gresult;

void test_long_add() {
    int *p;
    printf("--- Phase 4: long addition ---\n");

    // long + long (small values)
    gadd1 = 10;
    gadd2 = 20;
    gresult = gadd1 + gadd2;
    p = &gresult;
    check("10L + 20L low == 30", *p == 30);
    check("10L + 20L high == 0", *(p + 1) == 0);
}

void test_long_sub() {
    int *p;
    printf("--- Phase 4: long subtraction ---\n");

    gadd1 = 50;
    gadd2 = 20;
    gresult = gadd1 - gadd2;
    p = &gresult;
    check("50L - 20L low == 30", *p == 30);
    check("50L - 20L high == 0", *(p + 1) == 0);
}

void test_long_bitwise() {
    int *p;
    printf("--- Phase 4: long bitwise ops ---\n");

    gadd1 = 255;   // 0x00FF
    gadd2 = 15;    // 0x000F
    gresult = gadd1 & gadd2;
    p = &gresult;
    check("255L & 15L == 15", *p == 15);

    gresult = gadd1 | gadd2;
    p = &gresult;
    check("255L | 15L == 255", *p == 255);

    gresult = gadd1 ^ gadd2;
    p = &gresult;
    check("255L ^ 15L == 240", *p == 240);
}

void test_long_unary() {
    long val;
    int *p;
    int r;
    printf("--- Phase 4: long unary ops ---\n");

    val = 5;
    val = -val;
    p = &val;
    // -5 in 32-bit two's complement: lo=0xFFFB, hi=0xFFFF
    check("-5L low == -5", *p == -5);
    check("-5L high == -1", *(p + 1) == -1);

    printf("  DBG: before val=0\n");
    val = 0;
    printf("  DBG: after val=0\n");
    r = !val;
    printf("  DBG: after !val", r);
    check("!0L == 1", r == 1);
    printf("  DBG: after !val, r=%d\n", r);
    val = 1;
    r = !val;
    printf("  DBG: after !val, r=%d\n", r);
    check("!1L == 0", r == 0);
    printf("  DBG: after !val, r=%d\n", r);
}

void test_long_comparison() {
    long a, b;
    int r;
    printf("--- Phase 4: long comparisons ---\n");

    a = 10;
    printf("  DBG: a=10 done\n");
    b = 20;
    printf("  DBG: b=20 done\n");
    r = (a == b);
    printf("  DBG: r=(a==b) done, r=%d\n", r);
    check("10L == 20L is false", r == 0);
    r = (a == a);
    printf("  DBG: r=(a==a) done, r=%d\n", r);
    check("10L == 10L is true", r == 1);
    r = (a < b);
    printf("  DBG: r=(a<b) done, r=%d\n", r);
    check("10L < 20L is true", r == 1);
    r = (b < a);
    check("20L < 10L is false", r == 0);
    r = (a != b);
    check("10L != 20L is true", r == 1);
}

void test_int_long_promotion() {
    long result;
    int x;
    int *p;
    printf("--- Phase 4: int-to-long promotion ---\n");

    x = 100;
    result = x + gadd1;  // gadd1 was set to 50 in sub test
    gadd1 = 50;          // reset
    result = x + gadd1;
    p = &result;
    check("int(100) + long(50) low == 150", *p == 150);
    check("int(100) + long(50) high == 0", *(p + 1) == 0);
}

void test_long_assign_widen() {
    long lv;
    int iv;
    int *p;
    printf("--- Phase 4: assignment widening ---\n");

    iv = 42;
    lv = iv;   // int assigned to long -> widen
    p = &lv;
    check("long = int(42) low == 42", *p == 42);
    check("long = int(42) high == 0", *(p + 1) == 0);

    // Negative int -> long (sign extension)
    iv = -1;
    lv = iv;
    p = &lv;
    check("long = int(-1) low == -1", *p == -1);
    check("long = int(-1) high == -1", *(p + 1) == -1);
}

void test_compound_assign() {
    long val;
    int *p;
    printf("--- Phase 4: compound assignment ---\n");

    val = 10;
    p = &val;
    printf("  DBG: val=10 lo=%d hi=%d\n", *p, *(p + 1));
    val += 5;
    p = &val;
    printf("  DBG: val+=5 lo=%d hi=%d\n", *p, *(p + 1));
    check("10L += 5 low == 15", *p == 15);
    check("10L += 5 high == 0", *(p + 1) == 0);

    val = 20;
    val -= 3;
    p = &val;
    check("20L -= 3 low == 17", *p == 17);
}

void test_long_truthiness() {
    long zero, nonzero;
    int r;
    printf("--- Phase 4: long truthiness ---\n");

    zero = 0;
    nonzero = 1;

    r = 0;
    if (nonzero)
        r = 1;
    check("if(1L) is true", r == 1);

    r = 1;
    if (zero)
        r = 0;
    check("if(0L) is false", r == 1);
}

// ============================================================================
// Phase 5 tests: Pointer Arithmetic & Arrays
// ============================================================================

long garr5[4];

void test_long_array_subscript() {
    long arr[3];
    int *p;
    printf("--- Phase 5: long array subscript ---\n");

    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;

    p = &arr[0];
    check("arr[0] low == 10", *p == 10);
    p = &arr[1];
    check("arr[1] low == 20", *p == 20);
    p = &arr[2];
    check("arr[2] low == 30", *p == 30);
}

void test_long_ptr_inc() {
    long *lp;
    int before, after;
    printf("--- Phase 5: long pointer increment ---\n");

    lp = garr5;
    before = lp;
    lp++;
    after = lp;
    check("long* ++ increments by 4", (after - before) == 4);

    lp--;
    after = lp;
    check("long* -- decrements by 4", after == before);
}

void test_long_val_inc() {
    long val;
    int *p;
    printf("--- Phase 5: long value increment ---\n");

    val = 10;
    val++;
    p = &val;
    check("10L++ low == 11", *p == 11);
    check("10L++ high == 0", *(p + 1) == 0);

    val--;
    p = &val;
    check("11L-- low == 10", *p == 10);
}

void test_prefix_inc_long() {
    long val;
    int *p;
    printf("--- Phase 5: prefix inc/dec long ---\n");

    val = 5;
    ++val;
    p = &val;
    check("++5L low == 6", *p == 6);

    --val;
    p = &val;
    check("--6L low == 5", *p == 5);
}

// ============================================================================
// Phase 6 tests: Constant Parsing, Suffixes, Folding
// ============================================================================

void test_large_constants() {
    long val;
    int *p;
    printf("--- Phase 6: large constants ---\n");

    // 70000 = 0x11170 => lo=0x1170=4464, hi=0x0001=1
    val = 70000;
    p = &val;
    check("70000 low == 4464", *p == 4464);
    check("70000 high == 1", *(p + 1) == 1);

    // 100000 = 0x186A0 => lo=0x86A0=-31072(signed), hi=0x0001=1
    val = 100000;
    p = &val;
    check("100000 low == -31072", *p == -31072);
    check("100000 high == 1", *(p + 1) == 1);

    // Negative large constant: -70000
    val = -70000;
    p = &val;
    check("-70000 low == -4464", *p == -4464);
    check("-70000 high == -2", *(p + 1) == -2);
}

void test_suffix_parsing() {
    long val;
    int *p;
    printf("--- Phase 6: suffix parsing ---\n");

    // L suffix forces long type even for small values
    val = 100L;
    p = &val;
    check("100L low == 100", *p == 100);
    check("100L high == 0", *(p + 1) == 0);

    // 0L
    val = 0L;
    p = &val;
    check("0L low == 0", *p == 0);
    check("0L high == 0", *(p + 1) == 0);

    // UL suffix
    val = 50000UL;
    p = &val;
    check("50000UL low == -15536", *p == -15536);
    check("50000UL high == 0", *(p + 1) == 0);

    // U suffix: 50000U should be unsigned int (fits 16-bit unsigned)
    // but 70000U should be unsigned long
    val = 70000U;
    p = &val;
    check("70000U low == 4464", *p == 4464);
    check("70000U high == 1", *(p + 1) == 1);
}

void test_hex_octal_large() {
    long val;
    int *p;
    printf("--- Phase 6: hex/octal large constants ---\n");

    // 0x10000 = 65536 => lo=0, hi=1
    val = 0x10000;
    p = &val;
    check("0x10000 low == 0", *p == 0);
    check("0x10000 high == 1", *(p + 1) == 1);

    // 0x1FFFF = 131071 => lo=0xFFFF=-1, hi=1
    val = 0x1FFFF;
    p = &val;
    check("0x1FFFF low == -1", *p == -1);
    check("0x1FFFF high == 1", *(p + 1) == 1);

    // 0200000 (octal) = 65536 => lo=0, hi=1
    val = 0200000;
    p = &val;
    check("0200000 low == 0", *p == 0);
    check("0200000 high == 1", *(p + 1) == 1);

    // 0xFFFF should stay as unsigned int (16-bit), not long
    // When assigned to long, it should zero-extend
    val = 0xFFFF;
    p = &val;
    check("0xFFFF low == -1", *p == -1);
    // hi depends on whether 0xFFFF is uint (zero-extend) or int (sign-extend)
    // Per C89, hex 0xFFFF fits in unsigned int, so zero-extends
    check("0xFFFF high == 0", *(p + 1) == 0);
}

void test_const_folding() {
    long val;
    int *p;
    printf("--- Phase 6: constant folding ---\n");

    // Addition of two long constants
    val = 70000 + 1;
    p = &val;
    check("70000+1 low == 4465", *p == 4465);
    check("70000+1 high == 1", *(p + 1) == 1);

    // Subtraction
    val = 70000 - 1;
    p = &val;
    check("70000-1 low == 4463", *p == 4463);
    check("70000-1 high == 1", *(p + 1) == 1);

    // Bitwise AND
    val = 0x1FFFF & 0xFFFF;
    p = &val;
    check("0x1FFFF & 0xFFFF low == -1", *p == -1);
    check("0x1FFFF & 0xFFFF high == 0", *(p + 1) == 0);

    // Bitwise OR
    val = 0x10000 | 0xFF;
    p = &val;
    check("0x10000|0xFF low == 255", *p == 255);
    check("0x10000|0xFF high == 1", *(p + 1) == 1);

    // Shift left
    val = 1L << 16;
    p = &val;
    check("1L<<16 low == 0", *p == 0);
    check("1L<<16 high == 1", *(p + 1) == 1);

    // Equality of constants
    val = (70000 == 70000);
    check("70000==70000 is 1", val == 1);

    val = (70000 == 70001);
    check("70000==70001 is 0", val == 0);
}

void test_const_unary() {
    long val;
    int *p;
    int r;
    printf("--- Phase 6: unary constant folding ---\n");

    // Negate large constant
    val = -70000;
    p = &val;
    check("-70000 low == -4464", *p == -4464);
    check("-70000 high == -2", *(p + 1) == -2);

    // Bitwise complement
    val = ~0x10000;
    p = &val;
    check("~0x10000 low == -1", *p == -1);
    check("~0x10000 high == -2", *(p + 1) == -2);

    // Logical negate
    r = !70000;
    check("!70000 == 0", r == 0);

    r = !0L;
    check("!0L == 1", r == 1);
}

// ============================================================================
// Phase 7 tests: Function Calls & Returns with long
// ============================================================================

// --- Phase 7a: Long function arguments ---

// Helpers for test11_addr_arg: take int* like btell(fd, offset[])
void fill_long(int *p, int lo, int hi) { p[0] = lo; p[1] = hi; }
int get_lo(p) int *p; { return p[0]; }
int get_hi(p) int *p; { return p[1]; }

long add_longs(long a, long b) { return a + b; }

long add_int_lng(int a, long b) { return a + b; }

long first_arg(long a, int b) { return a; }

long second_arg(int a, long b) { return b; }

long three_args(long a, int b, long c) { return a + b + c; }

void test7_args() {
    long r;
    int *p;
    printf("--- Phase 7a: long function arguments ---\n");

    r = add_longs(70000, 30000L);
    p = &r;
    check("add_longs(70000,30000) lo", *p == -31072);
    check("add_longs(70000,30000) hi", *(p+1) == 1);

    r = add_int_lng(100, 70000);
    p = &r;
    check("add_int_lng(100,70000) lo", *p == 4564);
    check("add_int_lng(100,70000) hi", *(p+1) == 1);

    r = first_arg(70000, 5);
    p = &r;
    check("first_arg(70000,5) lo", *p == 4464);
    check("first_arg(70000,5) hi", *(p+1) == 1);

    r = second_arg(5, 70000);
    p = &r;
    check("second_arg(5,70000) lo", *p == 4464);
    check("second_arg(5,70000) hi", *(p+1) == 1);

    r = three_args(70000, 5, 30000L);
    p = &r;
    check("three_args lo", *p == -31067);
    check("three_args hi", *(p+1) == 1);
}

// --- Phase 7b: Long return values ---

long ret_const() { return 70000; }

long ret_widen(int x) { return x; }

long ret_expr(long a) { return a + 1; }

long ret_neg() { return -1; }

int test7_return() {
    long r;
    int *p;
    printf("--- Phase 7b: long return values ---\n");

    r = ret_const();
    p = &r;
    check("ret_const() lo == 4464", *p == 4464);
    check("ret_const() hi == 1", *(p+1) == 1);

    r = ret_widen(-1);
    p = &r;
    check("ret_widen(-1) lo == -1", *p == -1);
    check("ret_widen(-1) hi == -1", *(p+1) == -1);

    r = ret_expr(99999);
    p = &r;
    check("ret_expr(99999) lo", *p == -31072);
    check("ret_expr(99999) hi", *(p+1) == 1);

    r = ret_neg();
    p = &r;
    check("ret_neg() lo == -1", *p == -1);
    check("ret_neg() hi == -1", *(p+1) == -1);
}

// --- Phase 7c: Long result in expressions ---

long double_it(long x) { return x + x; }

void test7_retexpr() {
    long r;
    int *p;
    printf("--- Phase 7c: long result in expressions ---\n");

    r = double_it(50000) + 1;
    p = &r;
    check("double_it(50000)+1 lo", *p == -31071);
    check("double_it(50000)+1 hi", *(p+1) == 1);
}

// ============================================================================
// Phase 8 tests: Control Flow & Switch
// ============================================================================

// --- Phase 8a: Switch on long values ---

long sw_long(long x) {
    switch (x) {
        case 0:      return 1;
        case 70000:  return 2;
        case 100000: return 3;
        case -1:     return 4;
        default:     return 0;
    }
}

void test8_switch() {
    printf("--- Phase 8a: switch on long values ---\n");

    check("sw_long(0L)==1", sw_long(0L) == 1);
    check("sw_long(70000)==2", sw_long(70000) == 2);
    check("sw_long(100000)==3", sw_long(100000) == 3);
    check("sw_long(-1L)==4", sw_long(-1L) == 4);
    check("sw_long(42L)==0", sw_long(42L) == 0);
}

// --- Phase 8b: Long in control flow (regression) ---

int long_if(long x) { if (x) return 1; return 0; }

int long_while() {
    long i;
    int c;
    i = 3; c = 0;
    while (i) { c++; i--; }
    return c;
}

int long_for() {
    long i;
    int c;
    c = 0;
    for (i = 0; i < 3; i++) c++;
    return c;
}

int long_do() {
    long i;
    int c;
    i = 3; c = 0;
    do { c++; i--; } while (i);
    return c;
}

void test8_ctrlflow() {
    printf("--- Phase 8b: long in control flow ---\n");

    check("long_if(0L)==0", long_if(0L) == 0);
    check("long_if(70000)==1", long_if(70000) == 1);
    check("long_while()==3", long_while() == 3);
    check("long_for()==3", long_for() == 3);
    check("long_do()==3", long_do() == 3);
}

// --- Phase 8c: 16-bit switch regression ---

int sw_int(int x) {
    switch (x) {
        case 1:  return 10;
        case 2:  return 20;
        default: return 0;
    }
}

void test8_intswitch() {
    printf("--- Phase 8c: 16-bit switch regression ---\n");

    check("sw_int(1)==10", sw_int(1) == 10);
    check("sw_int(2)==20", sw_int(2) == 20);
    check("sw_int(3)==0", sw_int(3) == 0);
}

// ============================================================================
// Phase 9 tests: Cast Expressions
// ============================================================================

void test9_basic() {
    long l;
    int i;
    char c;
    unsigned long ul;
    unsigned u;
    int *p;

    printf("--- Phase 9a: basic cast expressions ---\n");

    // int to long (signed widening)
    i = 100;
    l = (long)i;
    p = &l;
    check("(long)100 lo", *p == 100);
    check("(long)100 hi", *(p+1) == 0);

    // negative int to long (sign extension)
    i = -1;
    l = (long)i;
    p = &l;
    check("(long)-1 lo", *p == -1);
    check("(long)-1 hi", *(p+1) == -1);

    // unsigned int to long (zero extension)
    u = 0xFFFF;
    l = (long)u;
    p = &l;
    check("(long)0xFFFF lo", *p == -1);
    check("(long)0xFFFF hi", *(p+1) == 0);

    // long to int (truncation)
    l = 70000;
    i = (int)l;
    check("(int)70000L==4464", i == 4464);

    // long to char (truncation to byte)
    l = 0x1234;
    c = (char)l;
    check("(char)0x1234L", c == 0x34);

    // int to char
    i = 0x4142;
    c = (char)i;
    check("(char)0x4142", c == 0x42);
}

void test9_unsign() {
    long l;
    int i;
    unsigned u;
    unsigned long ul;
    int *p;

    printf("--- Phase 9b: unsigned cast variants ---\n");

    // int to unsigned long: signed source is sign-extended
    i = -1;
    ul = (unsigned long)i;
    p = &ul;
    check("(ulong)-1 lo", *p == -1);
    check("(ulong)-1 hi", *(p+1) == -1);

    // unsigned int to unsigned long: zero-extended
    u = 0x8000;
    ul = (unsigned long)u;
    p = &ul;
    check("(ulong)0x8000 lo", *p == -32768);
    check("(ulong)0x8000 hi", *(p+1) == 0);

    // long to unsigned int
    l = -1;
    i = (unsigned)l;
    check("(uint)-1L==-1", i == -1);

    // unsigned long to int
    ul = 70000;
    i = (int)ul;
    check("(int)70000UL==4464", i == 4464);
}

void test9_const() {
    long l;
    int i;
    int *p;

    printf("--- Phase 9c: cast constant folding ---\n");

    // Cast constant int to long
    l = (long)42;
    p = &l;
    check("(long)42 lo", *p == 42);
    check("(long)42 hi", *(p+1) == 0);

    // Cast negative constant to long
    l = (long)-1;
    p = &l;
    check("(long)-1c lo", *p == -1);
    check("(long)-1c hi", *(p+1) == -1);

    // Cast long constant to int (truncate)
    i = (int)100000;
    check("(int)100000", i == -31072);

    // Cast to char
    i = (char)0x41;
    check("(char)0x41==65", i == 65);

    // Cast long to long (no-op)
    l = (long)70000;
    p = &l;
    check("(long)70000 lo", *p == 4464);
    check("(long)70000 hi", *(p+1) == 1);
}

void test9_expr() {
    long l;
    int i;
    int *p;

    printf("--- Phase 9d: cast in expressions ---\n");

    // Cast result used in arithmetic
    i = 100;
    l = (long)i + 70000;
    p = &l;
    check("(long)100+70k lo", *p == 4564);
    check("(long)100+70k hi", *(p+1) == 1);

    // Cast inside comparison
    i = 5;
    check("(long)5==5L", (long)i == 5L);

    // Cast in function argument
    l = (long)200 + (long)300;
    p = &l;
    check("(long)200+300 lo", *p == 500);
    check("(long)200+300 hi", *(p+1) == 0);
}

void test9_edge() {
    long l;
    int *p;

    printf("--- Phase 9e: cast edge cases ---\n");

    // Cast 0 to long
    l = (long)0;
    p = &l;
    check("(long)0 lo", *p == 0);
    check("(long)0 hi", *(p+1) == 0);

    // Cast max signed int to long
    l = (long)32767;
    p = &l;
    check("(long)32767 lo", *p == 32767);
    check("(long)32767 hi", *(p+1) == 0);

    // Cast min signed int to long (sign extension)
    l = (long)(-32768);
    p = &l;
    check("(long)-32768 lo", *p == -32768);
    check("(long)-32768 hi", *(p+1) == -1);

    // Nested parens with cast
    l = (long)(100 + 200);
    p = &l;
    check("(long)(300) lo", *p == 300);
    check("(long)(300) hi", *(p+1) == 0);
}

void test9_regr() {
    int a, b;
    long l;

    printf("--- Phase 9f: paren regression ---\n");

    // Parenthesized expressions must still work
    a = (1 + 2) * 3;
    check("(1+2)*3==9", a == 9);

    b = (a);
    check("(a)==9", b == 9);

    // Comma expression in parens
    a = (1, 2, 3);
    check("(1,2,3)==3", a == 3);

    // Existing long operations still work
    l = 70000;
    check("70000 still works", l == 70000);
}

void test9_uchar() {
    char c;
    unsigned char uc;
    int i;
    long l;
    int *p;

    printf("--- Phase 9g: unsigned char casts ---\n");

    // int to unsigned char
    i = 0x4142;
    uc = (unsigned char)i;
    check("(uchar)0x4142", uc == 0x42);

    // long to unsigned char
    l = 0x1234;
    uc = (unsigned char)l;
    check("(uchar)0x1234L", uc == 0x34);

    // negative int to unsigned char
    i = -1;
    uc = (unsigned char)i;
    check("(uchar)-1==255", uc == 255);

    // unsigned char to long (zero extend)
    uc = 200;
    l = (long)uc;
    p = &l;
    check("(long)uc200 lo", *p == 200);
    check("(long)uc200 hi", *(p+1) == 0);

    // unsigned char to unsigned long
    uc = 255;
    l = (unsigned long)uc;
    p = &l;
    check("(ulong)uc255 lo", *p == 255);
    check("(ulong)uc255 hi", *(p+1) == 0);
}

void test9_char() {
    char c;
    int i;
    long l;
    int *p;

    printf("--- Phase 9h: char cast paths ---\n");

    // char to long (sign extend negative)
    c = -1;
    l = (long)c;
    p = &l;
    check("(long)c-1 lo", *p == -1);
    check("(long)c-1 hi", *(p+1) == -1);

    // char to long (positive)
    c = 65;
    l = (long)c;
    p = &l;
    check("(long)c65 lo", *p == 65);
    check("(long)c65 hi", *(p+1) == 0);

    // char to int (explicit cast)
    c = 0x42;
    i = (int)c;
    check("(int)c==66", i == 66);

    // char to unsigned int
    c = -1;
    i = (unsigned)c;
    check("(uint)c-1", i == -1);

    // char to unsigned long
    c = -1;
    l = (unsigned long)c;
    p = &l;
    check("(ulong)c-1 lo", *p == -1);
    check("(ulong)c-1 hi", *(p+1) == -1);
}

void test9_syntax() {
    long l;
    int i;
    int *p;

    printf("--- Phase 9i: cast syntax variants ---\n");

    // (long int) syntax
    i = 100;
    l = (long int)i;
    p = &l;
    check("(long int)100 lo", *p == 100);
    check("(long int)100 hi", *(p+1) == 0);

    // (unsigned long int) syntax
    i = 50;
    l = (unsigned long int)i;
    p = &l;
    check("(ulong int)50 lo", *p == 50);
    check("(ulong int)50 hi", *(p+1) == 0);

    // same-type cast: int to int (no-op)
    i = 42;
    i = (int)i;
    check("(int)int==42", i == 42);

    // same-type cast: long to long (runtime, not const)
    l = 70000;
    l = (long)l;
    p = &l;
    check("(long)long lo", *p == 4464);
    check("(long)long hi", *(p+1) == 1);

    // (unsigned) shorthand for (unsigned int)
    l = 70000;
    i = (unsigned)l;
    check("(unsigned)70k", i == 4464);
}

// ============================================================================
// Phase 10-12 tests: Edge cases, mul/div/mod, shifts, unsigned cmp, etc.
// ============================================================================

// --- test10_muldv: long multiply, divide, modulo ---

void test10_muldv() {
    long a, b, r;
    int *p;
    printf("--- Phase 10: long mul/div/mod ---\n");

    // 70000 * 2 = 140000 => lo=8928, hi=2
    a = 70000;
    b = 2;
    r = a * b;
    p = &r;
    check("70000*2 lo==8928", *p == 8928);
    check("70000*2 hi==2", *(p+1) == 2);

    // 140000 / 2 = 70000 => lo=4464, hi=1
    r = 140000;
    r = r / b;
    p = &r;
    check("140000/2 lo==4464", *p == 4464);
    check("140000/2 hi==1", *(p+1) == 1);

    // 70001 % 10 = 1
    a = 70001;
    b = 10;
    r = a % b;
    p = &r;
    check("70001%10 lo==1", *p == 1);
    check("70001%10 hi==0", *(p+1) == 0);

    // Negative: -70000 / 2 = -35000
    // -35000 in 32-bit: 0xFFFF7748, lo=0x7748=30536, hi=0xFFFF=-1
    a = -70000;
    b = 2;
    r = a / b;
    p = &r;
    check("-70000/2 lo", *p == 30536);
    check("-70000/2 hi==-1", *(p+1) == -1);

    // Signed modulo: -7 % 3 = -1
    a = -7;
    b = 3;
    r = a % b;
    p = &r;
    check("-7L%3L lo==-1", *p == -1);
    check("-7L%3L hi==-1", *(p+1) == -1);
}

// --- test10_shift: long shift operations ---

void test10_shift() {
    long a, r;
    int *p;
    printf("--- Phase 10: long shifts ---\n");

    // 1L << 16 = 65536 => lo=0, hi=1
    a = 1;
    r = a << 16;
    p = &r;
    check("1L<<16 lo==0", *p == 0);
    check("1L<<16 hi==1", *(p+1) == 1);

    // 65536 >> 16 = 1 => lo=1, hi=0
    a = 65536;
    r = a >> 16;
    p = &r;
    check("65536>>16 lo==1", *p == 1);
    check("65536>>16 hi==0", *(p+1) == 0);

    // Arithmetic right shift of negative: -65536 >> 8
    // -65536 = 0xFFFF0000, >> 8 = 0xFFFFFF00
    // lo=0xFF00=-256, hi=0xFFFF=-1
    a = -65536;
    r = a >> 8;
    p = &r;
    check("-65536>>8 lo", *p == -256);
    check("-65536>>8 hi==-1", *(p+1) == -1);

    // Shift by 0 is identity
    a = 70000;
    r = a << 0;
    p = &r;
    check("70000<<0 lo", *p == 4464);
    check("70000<<0 hi==1", *(p+1) == 1);
}

// --- test10_ulcmp: unsigned long comparisons ---

void test10_ulcmp() {
    long a, b;
    int r;
    printf("--- Phase 10: unsigned long cmp ---\n");

    // Compare large unsigned values
    // 0x80000000 as unsigned > 1
    // In 32-bit: 0x80000000 = lo=0, hi=0x8000=-32768
    a = 1;
    b = 0;
    // Use raw memory to set b = 0x80000000
    {
        int *p;
        p = &b;
        *p = 0;
        *(p+1) = 0x8000;
    }
    // Signed: b < a (because 0x80000000 is negative)
    r = (b < a);
    check("0x80000000 < 1 signed", r == 1);

    // 70000 > 30000 unsigned
    a = 70000;
    b = 30000;
    r = (a > b);
    check("70000 > 30000", r == 1);

    // 0 <= anything
    a = 0;
    b = 70000;
    r = (a <= b);
    check("0 <= 70000", r == 1);
}

// --- test10_mdarr: multi-dimensional long array ---

void test10_mdarr() {
    long arr[2][3];
    int *p;
    printf("--- Phase 10: multi-dim long arr ---\n");

    arr[0][0] = 10;
    arr[0][1] = 20;
    arr[0][2] = 30;
    arr[1][0] = 70000;
    arr[1][1] = 80000;
    arr[1][2] = 90000;

    check("arr[0][0]==10", arr[0][0] == 10);
    check("arr[0][2]==30", arr[0][2] == 30);

    p = &arr[1][0];
    check("arr[1][0] lo==4464", *p == 4464);
    check("arr[1][0] hi==1", *(p+1) == 1);

    p = &arr[1][2];
    check("arr[1][2] lo==24464", *p == 24464);
    check("arr[1][2] hi==1", *(p+1) == 1);
}

// --- test10_postf: postfix inc/dec old value ---

void test10_postf() {
    long val, old;
    int *p;
    printf("--- Phase 10: postfix old value ---\n");

    val = 10;
    old = val++;
    p = &old;
    check("postfix++ old lo==10", *p == 10);
    check("postfix++ old hi==0", *(p+1) == 0);
    p = &val;
    check("postfix++ new lo==11", *p == 11);

    val = 20;
    old = val--;
    p = &old;
    check("postfix-- old lo==20", *p == 20);
    check("postfix-- old hi==0", *(p+1) == 0);
    p = &val;
    check("postfix-- new lo==19", *p == 19);
}

// --- test10_ptdif: pointer-to-long difference ---

void test10_ptdif() {
    long arr[4];
    long *p1, *p2;
    int diff;
    printf("--- Phase 10: long ptr diff ---\n");

    p1 = &arr[0];
    p2 = &arr[3];
    diff = p2 - p1;
    check("&arr[3]-&arr[0]==3", diff == 3);

    p2 = &arr[1];
    diff = p2 - p1;
    check("&arr[1]-&arr[0]==1", diff == 1);
}

// --- test10_strct: struct long member access ---

void test10_strct() {
    struct has_long s;
    int *p;
    printf("--- Phase 10: struct long member ---\n");

    s.c = 'A';
    s.l = 70000;
    s.i = 99;

    check("s.c == 'A'", s.c == 65);
    check("s.i == 99", s.i == 99);

    p = &s.l;
    check("s.l lo==4464", *p == 4464);
    check("s.l hi==1", *(p+1) == 1);

    // Modify and re-read
    s.l = -1;
    p = &s.l;
    check("s.l=-1 lo==-1", *p == -1);
    check("s.l=-1 hi==-1", *(p+1) == -1);
    // Verify other members not clobbered
    check("s.c still 'A'", s.c == 65);
    check("s.i still 99", s.i == 99);
}

// --- test10_chlng: char + long mixed arithmetic ---

void test10_chlng() {
    char c;
    long lv, r;
    int *p;
    printf("--- Phase 10: char+long mixed ---\n");

    c = 5;
    lv = 70000;
    r = lv + c;
    p = &r;
    // 70000 + 5 = 70005 => lo=4469, hi=1
    check("70000+char(5) lo", *p == 4469);
    check("70000+char(5) hi", *(p+1) == 1);

    // Subtraction
    r = lv - c;
    p = &r;
    // 70000 - 5 = 69995 => lo=4459, hi=1
    check("70000-char(5) lo", *p == 4459);
    check("70000-char(5) hi", *(p+1) == 1);

    // Negative char
    c = -1;
    r = lv + c;
    p = &r;
    // 70000 + (-1) = 69999 => lo=4463, hi=1
    check("70000+char(-1) lo", *p == 4463);
    check("70000+char(-1) hi", *(p+1) == 1);
}

// --- test10_logic: long in && || ternary ---

void test10_logic() {
    long a;
    int r;
    printf("--- Phase 10: long && || ternary ---\n");

    a = 70000;
    r = (a && 1);
    check("70000 && 1 == 1", r == 1);

    a = 0;
    r = (a && 1);
    check("0L && 1 == 0", r == 0);

    a = 0;
    r = (a || 1);
    check("0L || 1 == 1", r == 1);

    a = 0;
    r = (a || 0);
    check("0L || 0 == 0", r == 0);

    a = 70000;
    r = (a || 0);
    check("70000 || 0 == 1", r == 1);

    // Ternary
    a = 70000;
    r = a ? 42 : 99;
    check("70000 ? 42 : 99", r == 42);

    a = 0;
    r = a ? 42 : 99;
    check("0L ? 42 : 99", r == 99);
}

// --- test10_dec40: decimal 40000 auto-promotes ---

void test10_dec40() {
    long val;
    int *p;
    printf("--- Phase 10: decimal 40000 ---\n");

    // 40000 exceeds 16-bit signed int (32767), should auto-promote
    val = 40000;
    p = &val;
    // 40000 = 0x9C40 => lo=0x9C40=-25536, hi=0
    check("40000 lo==-25536", *p == -25536);
    check("40000 hi==0", *(p+1) == 0);

    // Verify arithmetic with it
    val = 40000 + 30000;
    p = &val;
    // 70000 => lo=4464, hi=1
    check("40000+30000 lo", *p == 4464);
    check("40000+30000 hi==1", *(p+1) == 1);
}

// ============================================================================
// Phase 11: &(long_local) passed directly as a function argument
// This is the btell(fd, &offset) pattern that previously triggered the
// PUSHd1 bug: cc3.c left TYP_OBJ = TYPE_ULONG after the & operator,
// causing isLongVal() to emit PUSH DX + PUSH AX instead of just PUSH AX.
// ============================================================================

void test11_addr_arg() {
    unsigned long lv;
    long sl;
    int *p;

    printf("--- Phase 11: &long as function argument ---\n");

    // fill_long writes through int* — same pattern as btell(fd, &offset)
    fill_long(&lv, 1234, 5678);
    p = &lv;
    check("fill &ulong lo", *p == 1234);
    check("fill &ulong hi", *(p+1) == 5678);

    // read back via get_lo / get_hi
    lv = 70000;
    check("get_lo(&ulong)", get_lo(&lv) == 4464);
    check("get_hi(&ulong)", get_hi(&lv) == 1);

    // same with signed long
    fill_long(&sl, -1, -1);
    p = &sl;
    check("fill &long lo==-1", *p == -1);
    check("fill &long hi==-1", *(p+1) == -1);

    sl = -70000;
    check("get_lo(&long neg)", get_lo(&sl) == -4464);
    check("get_hi(&long neg)", get_hi(&sl) == -2);
}

// ============================================================================
// Phase 12: int-to-unsigned-long widening (sign-extension trap)
// When an int value >= 0x8000 is cast directly to unsigned long, it
// sign-extends to 0xFFFFxxxx. The fix is to cast through uint first:
//   (unsigned long)(uint)int_val   -- zero-extends (correct)
//   (unsigned long)int_val         -- sign-extends (wrong)
// ============================================================================

// Helper: store a value into an int array slot, widen via uint (correct)
unsigned long widen_uint(int arr[], int idx) {
    unsigned int u;
    u = arr[idx];
    return (unsigned long)u;
}

// Helper: widen directly from int (exposes sign-extension if value >= 0x8000)
unsigned long widen_int(int arr[], int idx) {
    return (unsigned long)arr[idx];
}

void test12_widen() {
    int tbl[4];
    unsigned long r;

    printf("--- Phase 12: int->ulong widening sign-extension ---\n");

    // Small value: both paths identical
    tbl[0] = 0x1234;
    r = widen_uint(tbl, 0);
    check("widen_uint small", r == 0x1234);
    r = widen_int(tbl, 0);
    check("widen_int small", r == 0x1234);

    // Value just below 0x8000: still safe
    tbl[0] = 0x7FFF;
    r = widen_uint(tbl, 0);
    check("widen_uint 0x7FFF", r == 0x7FFF);

    // Value >= 0x8000: sign-extension trap
    // 0xBC46 is a real value from the ylink bug (CC4.OBJ code segment origin)
    tbl[0] = 0xBC46;
    r = widen_uint(tbl, 0);
    check("widen_uint 0xBC46 lo", (int)(r & 0xFFFF) == 0xBC46);
    check("widen_uint 0xBC46 hi", (int)(r >> 16) == 0);

    // Direct int->ulong cast sign-extends: hi word becomes 0xFFFF
    r = widen_int(tbl, 0);
    check("widen_int 0xBC46 hi==0xFFFF", (int)(r >> 16) == 0xFFFF);

    // Another boundary: 0x8000 itself
    tbl[0] = 0x8000;
    r = widen_uint(tbl, 0);
    check("widen_uint 0x8000 hi==0", (int)(r >> 16) == 0);
    r = widen_int(tbl, 0);
    check("widen_int 0x8000 hi==0xFFFF", (int)(r >> 16) == 0xFFFF);

    // The ylink fix: EXE_HDR_LEN(0x40) + 0xBC46, cast through uint
    tbl[0] = 0xBC46;
    { unsigned int u; u = tbl[0];
      r = (unsigned long)0x40 + (unsigned long)u; }
    check("ylink fix codeBase hi==0", (int)(r >> 16) == 0);
    check("ylink fix codeBase lo==0xBC86", (int)(r & 0xFFFF) == 0xBC86);
}

// ============================================================================
// Phase 13: Pointer comparison regression for isLongVal() fix.
// Before the fix, `lp == la` (long* vs long[]) emitted EQd12 (32-bit equality)
// because primary() sets TYP_OBJ=TYPE_LONG on an array name even though AX
// holds a 16-bit address. The fix guards the TYP_OBJ check with TYP_ADR==0.
// Also covers prefix ++/-- on long* and unsigned long* (Phase 5 only had
// postfix).
// ============================================================================
void test_lptrcmp() {
    long          la[2];
    unsigned long ula[2];
    long          *lp;
    unsigned long *ulp;
    int            b, a, r;

    printf("--- lptr cmp ---\n");

    // lp == la: pointer to array start must equal array name (16-bit EQ)
    lp = la;
    r  = (lp == la);
    check("lp==la true",   r == 1);

    // advance lp: now lp != la
    lp = la + 1;
    r  = (lp == la);
    check("lp!=la false",  r == 0);

    r  = (lp != la);
    check("lp!=la true",   r == 1);

    lp = la;
    r  = (lp != la);
    check("lp!=la eq0",    r == 0);

    // unsigned long*: same isLongVal() code path
    ulp = ula;
    r   = (ulp == ula);
    check("ulp==ula true", r == 1);

    ulp = ula + 1;
    r   = (ulp == ula);
    check("ulp!=ula fls",  r == 0);

    // prefix ++ on long*: step = 4
    lp = la;
    b  = lp;
    ++lp;
    a  = lp;
    check("++lp step=4",   (a - b) == 4);

    // prefix -- back to start: verify via lp == la (regression)
    --lp;
    r = (lp == la);
    check("--lp ==la",     r == 1);

    // prefix ++ on unsigned long*: step = 4
    ulp = ula;
    b   = ulp;
    ++ulp;
    a   = ulp;
    check("++ulp step=4",  (a - b) == 4);

    --ulp;
    r = (ulp == ula);
    check("--ulp ==ula",   r == 1);
}

// ============================================================================
// Main
// ============================================================================

void main() {
    passed = 0;
    failed = 0;

    printf("=== Testing Small-C Support for Long Int ===\n\n");

    // Phase 1-2 tests
    test_sizeof();
    test_struct_sizeof();
    test_local_decl();
    test_local_struct();
    test_global_init();
    test_long_func();

    // Phase 3 tests
    test_ld_st_global();
    test_ld_st_local();

    // Phase 4 tests
    test_long_add();
    test_long_sub();
    test_long_bitwise();
    test_long_unary();
    test_long_comparison();
    test_int_long_promotion();
    test_long_assign_widen();
    test_compound_assign();
    test_long_truthiness();

    // Phase 5 tests
    test_long_array_subscript();
    test_long_ptr_inc();
    test_long_val_inc();
    test_prefix_inc_long();

    // Phase 6 tests
    test_large_constants();
    test_suffix_parsing();
    test_hex_octal_large();
    test_const_folding();
    test_const_unary();

    // Phase 7 tests
    test7_args();
    test7_return();
    test7_retexpr();

    // Phase 8 tests
    test8_switch();
    test8_ctrlflow();
    test8_intswitch();

    // Phase 9 tests
    test9_basic();
    test9_unsign();
    test9_const();
    test9_expr();
    test9_edge();
    test9_regr();
    test9_uchar();
    test9_char();
    test9_syntax();

    // Phase 10 tests
    test10_muldv();
    test10_shift();
    test10_ulcmp();
    test10_mdarr();
    test10_postf();
    test10_ptdif();
    test10_strct();
    test10_chlng();
    test10_logic();
    test10_dec40();

    // Phase 11 tests
    test11_addr_arg();

    // Phase 12 tests
    test12_widen();

    // Phase 13 tests
    test_lptrcmp();

    // Are ya winning, son?
    printf("\n=== Results: %d passed, %d failed, %d total ===\n",
        passed, failed, passed + failed);

    if (failed)
        printf("*** FAILURES DETECTED ***\n");
    else
        printf("All tests passed.\n");
    if (failed) getchar();
}
