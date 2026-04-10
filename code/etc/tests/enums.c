// enums.c -- Test program for enum support in Small-C 2.3.
//
// Purpose:
//   Verify that the compiler correctly handles C-style enumeration types,
//   including enumerator constant values, enum-typed variables, and use of
//   enum constants in expressions, function calls, and switch statements.
//
// Functionality covered:
//   - Simple enum with auto-increment from 0 (RED, GREEN, BLUE)
//   - Enum with explicit integer values (NORTH=1, SOUTH=2, EAST=4, WEST=8)
//   - Enum starting at non-zero and then auto-incrementing (MON=1 .. SUN=7)
//   - Enum with negative values (NEG=-1, ZERO=0, POS=1)
//   - Mixed explicit and auto-incremented enumerators (A=10, B, C=20, D)
//   - Enum where an explicit value resets the counter mid-list
//   - Duplicate enumerator values (C90 allows this)
//   - Global variables declared with an enum type tag
//   - Global int initialized from an enumerator constant
//   - Local enum variables
//   - Enumerators used in arithmetic and comparison expressions
//   - Enum in switch/case statements
//   - Enum as function parameter and return type

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
// Global enum declarations
// ============================================================================

// Simple enum without explicit values (auto-incrementing from 0)
enum color { RED, GREEN, BLUE };

// Enum with explicit values
enum direction { NORTH = 1, SOUTH = 2, EAST = 4, WEST = 8 };

// Enum starting at non-zero
enum weekday { MON = 1, TUE, WED, THU, FRI, SAT, SUN };

// Enum with negative values
enum sign { NEG = -1, ZERO = 0, POS = 1 };

// Enum with explicit then auto
enum mixed { A = 10, B, C = 20, D };

// Enum where an explicit value resets the counter
enum resets { X = 5, Y, Z = 0, W };

// Enum with duplicate values (C90 allows this)
enum dups { ALPHA = 1, BETA = 1, GAMMA = 2 };

// Enum used as global variable base type
enum color gcolor;
enum direction gdir;

// Global int initialised from an enumerator
int gval_from_enum = GREEN;

// ============================================================================
// Test: enumerator constant values
// ============================================================================

void values_simple() {
    printf("--- simple enum values (start from 0) ---\n");
    check("RED   == 0", RED   == 0);
    check("GREEN == 1", GREEN == 1);
    check("BLUE  == 2", BLUE  == 2);
}

void values_explicit() {
    printf("--- explicit enum values ---\n");
    check("NORTH == 1", NORTH == 1);
    check("SOUTH == 2", SOUTH == 2);
    check("EAST  == 4", EAST  == 4);
    check("WEST  == 8", WEST  == 8);
}

void values_nonzero_start() {
    printf("--- auto-increment from non-zero start ---\n");
    check("MON == 1", MON == 1);
    check("TUE == 2", TUE == 2);
    check("WED == 3", WED == 3);
    check("THU == 4", THU == 4);
    check("FRI == 5", FRI == 5);
    check("SAT == 6", SAT == 6);
    check("SUN == 7", SUN == 7);
}

void values_negative() {
    printf("--- negative enum values ---\n");
    check("NEG  == -1", NEG  == -1);
    check("ZERO ==  0", ZERO ==  0);
    check("POS  ==  1", POS  ==  1);
}

void values_mixed() {
    printf("--- mixed explicit/auto values ---\n");
    check("A == 10", A == 10);
    check("B == 11", B == 11);
    check("C == 20", C == 20);
    check("D == 21", D == 21);
}

void values_resets() {
    printf("--- explicit value resets counter ---\n");
    check("X == 5", X == 5);
    check("Y == 6", Y == 6);
    check("Z == 0", Z == 0);
    check("W == 1", W == 1);
}

void values_dups() {
    printf("--- duplicate values allowed ---\n");
    check("ALPHA == 1", ALPHA == 1);
    check("BETA  == 1", BETA  == 1);
    check("GAMMA == 2", GAMMA == 2);
}

// ============================================================================
// Test: sizeof
// ============================================================================

void test_sizeof() {
    printf("--- sizeof tests ---\n");
    check("sizeof(enum color) == 2",   sizeof(enum color) == 2);
    check("sizeof(enum direction) == 2", sizeof(enum direction) == 2);
    check("sizeof(RED) == 2",          sizeof(RED) == 2);
    check("sizeof(GREEN) == 2",        sizeof(GREEN) == 2);
    check("sizeof(gcolor) == 2",       sizeof(gcolor) == 2);
    check("sizeof(gdir) == 2",         sizeof(gdir) == 2);
}

// ============================================================================
// Test: enum as int (backed by int — same size, assignable to int)
// ============================================================================

void enum_as_int() {
    int i;
    printf("--- enum as int ---\n");

    i = RED;
    check("int i = RED;  i == 0", i == 0);
    i = BLUE;
    check("int i = BLUE; i == 2", i == 2);
    i = NORTH;
    check("int i = NORTH; i == 1", i == 1);
    i = WEST;
    check("int i = WEST; i == 8", i == 8);
}

// ============================================================================
// Test: enum variable declaration and assignment
// ============================================================================

void enum_var() {
    enum color c;
    enum direction d;
    printf("--- enum variable decl and assign ---\n");

    c = RED;
    check("c = RED;   c == 0", c == 0);
    c = GREEN;
    check("c = GREEN; c == 1", c == 1);
    c = BLUE;
    check("c = BLUE;  c == 2", c == 2);

    d = EAST;
    check("d = EAST;  d == 4", d == 4);
    d = WEST;
    check("d = WEST;  d == 8", d == 8);
}

// ============================================================================
// Test: global enum variable
// ============================================================================

void global_enum_var() {
    printf("--- global enum variable ---\n");

    gcolor = RED;
    check("gcolor = RED;   gcolor == 0", gcolor == 0);
    gcolor = BLUE;
    check("gcolor = BLUE;  gcolor == 2", gcolor == 2);

    gdir = SOUTH;
    check("gdir = SOUTH;   gdir == 2", gdir == 2);
    gdir = NORTH;
    check("gdir = NORTH;   gdir == 1", gdir == 1);

    check("gval_from_enum == GREEN (1)", gval_from_enum == 1);
}

// ============================================================================
// Test: enumerators as constant expressions (array size, initializer)
// ============================================================================

// Array whose size is an enumerator
int palette[BLUE + 1];               // BLUE==2, so size==3
int compass[WEST + 1];               // WEST==8, so size==9

void const_array_size() {
    printf("--- enumerator as array size ---\n");
    check("sizeof(palette) == 3*2", sizeof(palette) == 6);
    check("sizeof(compass) == 9*2", sizeof(compass) == 18);
}

// ============================================================================
// Test: enumerator in expressions
// ============================================================================

void enum_expr() {
    int r;
    printf("--- enumerator in expressions ---\n");

    r = RED + BLUE;
    check("RED + BLUE == 2", r == 2);

    r = GREEN * 5;
    check("GREEN * 5 == 5", r == 5);

    r = WEST - NORTH;
    check("WEST - NORTH == 7", r == 7);

    r = EAST | NORTH;
    check("EAST | NORTH == 5", r == 5);

    r = (NORTH + SOUTH + EAST + WEST);
    check("NORTH+SOUTH+EAST+WEST == 15", r == 15);

    r = WEST & EAST;
    check("WEST & EAST == 0", r == 0);

    r = GREEN == RED;
    check("GREEN == RED is false (0)", r == 0);

    r = RED == ZERO;
    check("RED == ZERO (both 0) is true", r == 1);
}

// ============================================================================
// Test: cast (enum tag)expr treated as (int)
// ============================================================================

void enum_cast() {
    int x, r;
    printf("--- cast to enum type ---\n");

    x = 1;
    r = (enum color)x;
    check("(enum color)1 == 1", r == 1);

    x = 8;
    r = (enum color)x;
    check("(enum color)8 == 8", r == 8);

    x = -1;
    r = (enum sign)x;
    check("(enum sign)-1 == -1", r == -1);
}

// ============================================================================
// Test: switch on enum
// ============================================================================

int decode_color(int c) {
    switch (c) {
        case RED:   return 100;
        case GREEN: return 200;
        case BLUE:  return 300;
        default:    return 0;
    }
}

int decode_dir(int d) {
    switch (d) {
        case NORTH: return 1;
        case SOUTH: return 2;
        case EAST:  return 4;
        case WEST:  return 8;
        default:    return -1;
    }
}

void switch_enum() {
    printf("--- switch on enum constants ---\n");
    check("decode_color(RED)   == 100", decode_color(RED)   == 100);
    check("decode_color(GREEN) == 200", decode_color(GREEN) == 200);
    check("decode_color(BLUE)  == 300", decode_color(BLUE)  == 300);
    check("decode_color(99)    == 0",   decode_color(99)    == 0);

    check("decode_dir(NORTH) == 1", decode_dir(NORTH) == 1);
    check("decode_dir(SOUTH) == 2", decode_dir(SOUTH) == 2);
    check("decode_dir(EAST)  == 4", decode_dir(EAST)  == 4);
    check("decode_dir(WEST)  == 8", decode_dir(WEST)  == 8);
    check("decode_dir(0)     ==-1", decode_dir(0)     == -1);
}

// ============================================================================
// Test: enum in if / while / for
// ============================================================================

void enum_control_flow() {
    int c, sum;
    printf("--- enum in control flow ---\n");

    // if on enumerator
    if (RED == 0)
        check("if(RED==0) true", 1);
    else
        check("if(RED==0) true", 0);

    if (GREEN)
        check("if(GREEN) is true", 1);
    else
        check("if(GREEN) is true", 0);

    // while counting with enumerator bound
    c = RED;
    sum = 0;
    while (c <= BLUE) {
        sum = sum + c;
        c++;
    }
    check("sum 0..2 while loop == 3", sum == 3);

    // for loop up to FRI
    sum = 0;
    for (c = MON; c <= FRI; c++)
        sum = sum + c;
    check("sum MON..FRI (1+2+3+4+5) == 15", sum == 15);
}

// ============================================================================
// Test: enum in struct members
// ============================================================================

struct color_record {
    enum color c;
    int value;
};

void struct_with_enum() {
    struct color_record rec;
    printf("--- struct with enum field ---\n");

    rec.c = GREEN;
    rec.value = 42;
    check("rec.c == GREEN (1)", rec.c == 1);
    check("rec.value == 42",    rec.value == 42);

    rec.c = BLUE;
    check("rec.c = BLUE (2)", rec.c == 2);

    check("sizeof(struct color_record)==4",
        sizeof(struct color_record) == 4);
}

// ============================================================================
// Test: function taking/returning enum (as int)
// ============================================================================

int next_color(int c) {
    if (c == RED)   return GREEN;
    if (c == GREEN) return BLUE;
    return RED;
}

int is_primary(int d) {
    return (d == NORTH || d == SOUTH || d == EAST || d == WEST);
}

int enum_func() {
    printf("--- functions with enum args/returns ---\n");
    check("next_color(RED)  == GREEN", next_color(RED)  == GREEN);
    check("next_color(GREEN)== BLUE",  next_color(GREEN) == BLUE);
    check("next_color(BLUE) == RED",   next_color(BLUE)  == RED);

    check("is_primary(NORTH)==1", is_primary(NORTH) == 1);
    check("is_primary(EAST)==1",  is_primary(EAST)  == 1);
    check("is_primary(RED)==0",   is_primary(RED)   == 0);
}

// ============================================================================
// Test: cross-referencing enumerators in same definition
// ============================================================================

enum cross { CA = 3, CB = CA + 1, CC = CB * 2 };

void cross_ref() {
    printf("--- cross-reference within enum definition ---\n");
    check("CA == 3",  CA == 3);
    check("CB == 4",  CB == 4);
    check("CC == 8",  CC == 8);
}

// ============================================================================
// Test: enumerator constant in array initializer dimension
// ============================================================================

enum limits { MAXN = 5, MAXM = 3 };

int grid[MAXN][MAXM];   // 5x3 array of int

void array_dim() {
    int i, j;
    printf("--- enumerator as array dimension ---\n");
    check("sizeof(grid) == MAXN*MAXM*2", sizeof(grid) == 30);

    // Write and read back via enumerated bounds
    for (i = 0; i < MAXN; i++)
        for (j = 0; j < MAXM; j++)
            grid[i][j] = i * MAXM + j;

    check("grid[0][0] == 0",  grid[0][0] == 0);
    check("grid[1][0] == 3",  grid[1][0] == 3);
    check("grid[4][2] == 14", grid[4][2] == 14);
}

// ============================================================================
// Test: local enum declaration (tag must be recognised via global tag table)
// ============================================================================

void local_enum_var() {
    enum color lc;
    enum direction ld;
    int x;
    printf("--- local enum variable ---\n");

    lc = GREEN;
    check("local enum color = GREEN (1)", lc == 1);

    ld = WEST;
    check("local enum direction = WEST (8)", ld == 8);

    // Local int holding result of enum arithmetic
    x = lc + (int)ld;
    check("GREEN + WEST == 9", x == 9);
}

// ============================================================================
// Test: constant folding in IsConstExpr contexts
// ============================================================================

// The compiler evaluates these at compile time for the .DATA initializer.
int gi_red   = RED;
int gi_green = GREEN;
int gi_blue  = BLUE;
int gi_north = NORTH;
int gi_west  = WEST;
int gi_neg   = NEG;

void global_init() {
    printf("--- global init from enumerators ---\n");
    check("gi_red   == 0", gi_red   == 0);
    check("gi_green == 1", gi_green == 1);
    check("gi_blue  == 2", gi_blue  == 2);
    check("gi_north == 1", gi_north == 1);
    check("gi_west  == 8", gi_west  == 8);
    check("gi_neg   ==-1", gi_neg   == -1);
}

// ============================================================================
// Test: conditional compilation -- enumerators as #if-style comparisons
// ============================================================================

void enum_relational() {
    int r;
    printf("--- enumerator relational / logical ---\n");

    r = (RED < GREEN);
    check("RED < GREEN is true",  r == 1);
    r = (BLUE > RED);
    check("BLUE > RED is true",   r == 1);
    r = (GREEN <= GREEN);
    check("GREEN <= GREEN true",  r == 1);
    r = (GREEN >= BLUE);
    check("GREEN >= BLUE false",  r == 0);
    r = (RED != BLUE);
    check("RED != BLUE true",     r == 1);

    r = (RED == 0 && GREEN == 1);
    check("RED==0 && GREEN==1",   r == 1);
    r = (RED == 1 || GREEN == 1);
    check("RED==1 || GREEN==1",   r == 1);
}

// ============================================================================
// main
// ============================================================================

int main() {
    passed = 0;
    failed = 0;

    printf("=== testenum: enum support tests ===\n");

    values_simple();
    values_explicit();
    values_nonzero_start();
    values_negative();
    values_mixed();
    values_resets();
    values_dups();
    test_sizeof();
    enum_as_int();
    enum_var();
    global_enum_var();
    const_array_size();
    enum_expr();
    enum_cast();
    switch_enum();
    enum_control_flow();
    struct_with_enum();
    enum_func();
    cross_ref();
    array_dim();
    local_enum_var();
    global_init();
    enum_relational();

    printf("\n=== Results: %d passed, %d failed ===\n", passed, failed);
    if (failed) getchar();
    return failed;
}
