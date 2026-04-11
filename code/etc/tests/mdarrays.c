// mdarrays.c -- Test program for multi-dimensional array support.
//
// Purpose:
//   Verify that the compiler correctly handles multi-dimensional array
//   declarations, initializers, element access, and pointer decay for
//   2D and 3D arrays of both scalar and struct types.
//
// Functionality covered:
//   - 2D int array with nested brace initializer
//   - 2D char array initialized with string literals
//   - 2D int array with flat (non-nested) brace initializer
//   - Partial 2D array initializer (remaining elements zero-filled)
//   - 3D int array with nested brace initializer
//   - Uninitialized global 2D int array (implicitly zero)
//   - Global 2D struct arrays (struct point, struct cell)
//   - Local 2D and 3D array declarations and access
//   - Subscript expressions: correct row/column index calculation
//   - Function accepting a 1D row pointer (int row[], int cols)
//   - Function accepting a full 2D array parameter (int b[][3], int rows)
//   - sizeof for 2D and 3D arrays (total byte count)
//   - Local 2D long array (nested brace init) — regression for BPD store bug
//   - Local 2D long array (flat init)           — regression for BPD store bug

#include "../../smallc22/stdio.h"

int passed, failed;

// ============================================================================
// Struct definitions
// ============================================================================

struct point {
    int x, y;
};

struct cell {
    char tag;
    int  val;
};

// ============================================================================
// Global test arrays
// ============================================================================

// 2D int array with nested brace init
int mat3x3[3][3] = {
    {1, 2, 3},
    {4, 5, 6},
    {7, 8, 9}
};

// 2D char array with string init
char names[3][6] = {
    {"hello"},
    {"world"},
    {"test"}
};

// 2D int array with flat init
int flat2x3[2][3] = {1, 2, 3, 4, 5, 6};

// partial init (should zero-fill remainder)
int part2x3[2][3] = {
    {10, 20},
    {30}
};

// 3D int array
int cube[2][2][2] = {
    {{1, 2}, {3, 4}},
    {{5, 6}, {7, 8}}
};

// uninitialized global 2D array
int grid[4][4];

// global 2D struct arrays (uninitialized)
struct point pts[2][3];
struct cell cells[3][2];

// ============================================================================
// Helpers
// ============================================================================

void check(char *desc, int cond) {
    if (cond) {
        printf("  PASS: %s\n", desc);
        passed++;
    }
    else {
        printf("  FAIL: %s\n", desc);
        printf("  Press Enter...");
        getchar();
        failed++;
    }
}

// Function taking a 2D array parameter
int sumRow(int row[], int cols) {
    int s, c;
    s = 0;
    c = 0;
    while (c < cols) {
        s += row[c];
        ++c;
    }
    return s;
}

// Function taking a full 2D array parameter
int sum2D(int b[][3], int rows) {
    int r, c, s;
    s = 0;
    r = 0;
    while (r < rows) {
        c = 0;
        while (c < 3) {
            s += b[r][c];
            ++c;
        }
        ++r;
    }
    return s;
}

// ============================================================================
// Tests
// ============================================================================

void tGlb2DInt() {
    printf("\n--- Global 2D int (nested init) ---\n");
    check("mat[0][0]==1", mat3x3[0][0] == 1);
    check("mat[0][2]==3", mat3x3[0][2] == 3);
    check("mat[1][1]==5", mat3x3[1][1] == 5);
    check("mat[2][0]==7", mat3x3[2][0] == 7);
    check("mat[2][2]==9", mat3x3[2][2] == 9);
}

void tGlb2DChar() {
    printf("\n--- Global 2D char (string init) ---\n");
    check("names[0][0]=='h'", names[0][0] == 'h');
    check("names[1][0]=='w'", names[1][0] == 'w');
    check("names[2][0]=='t'", names[2][0] == 't');
    check("names[0][4]=='o'", names[0][4] == 'o');
}

void tGlbFlat() {
    printf("\n--- Global 2D int (flat init) ---\n");
    check("flat[0][0]==1", flat2x3[0][0] == 1);
    check("flat[0][2]==3", flat2x3[0][2] == 3);
    check("flat[1][0]==4", flat2x3[1][0] == 4);
    check("flat[1][2]==6", flat2x3[1][2] == 6);
}

void tGlbPartial() {
    printf("\n--- Global 2D int (partial init) ---\n");
    check("part[0][0]==10", part2x3[0][0] == 10);
    check("part[0][1]==20", part2x3[0][1] == 20);
    check("part[0][2]==0",  part2x3[0][2] == 0);
    check("part[1][0]==30", part2x3[1][0] == 30);
    check("part[1][1]==0",  part2x3[1][1] == 0);
    check("part[1][2]==0",  part2x3[1][2] == 0);
}

void tGlb3D() {
    printf("\n--- Global 3D int ---\n");
    check("cube[0][0][0]==1", cube[0][0][0] == 1);
    check("cube[0][0][1]==2", cube[0][0][1] == 2);
    check("cube[0][1][0]==3", cube[0][1][0] == 3);
    check("cube[1][0][0]==5", cube[1][0][0] == 5);
    check("cube[1][1][1]==8", cube[1][1][1] == 8);
}

void tLocal2D() {
    int loc[2][3];
    printf("\n--- Local 2D int (uninitialized write/read) ---\n");
    loc[0][0] = 100;
    loc[0][1] = 200;
    loc[0][2] = 300;
    loc[1][0] = 400;
    loc[1][1] = 500;
    loc[1][2] = 600;
    check("loc[0][0]==100", loc[0][0] == 100);
    check("loc[0][2]==300", loc[0][2] == 300);
    check("loc[1][0]==400", loc[1][0] == 400);
    check("loc[1][2]==600", loc[1][2] == 600);
}

void tLocalInit() {
    int li[2][2] = {{10, 20}, {30, 40}};
    printf("\n--- Local 2D int (brace init) ---\n");
    check("li[0][0]==10", li[0][0] == 10);
    check("li[0][1]==20", li[0][1] == 20);
    check("li[1][0]==30", li[1][0] == 30);
    check("li[1][1]==40", li[1][1] == 40);
}

void tGridZero() {
    int r, c, ok;
    printf("\n--- Global uninitialized 2D (should be 0) ---\n");
    ok = 1;
    r = 0;
    while (r < 4) {
        c = 0;
        while (c < 4) {
            if (grid[r][c] != 0) ok = 0;
            ++c;
        }
        ++r;
    }
    check("all grid[r][c]==0", ok);
}

void tRowParam() {
    printf("\n--- Row passed to function ---\n");
    check("sumRow(mat[0],3)==6",  sumRow(mat3x3[0], 3) == 6);
    check("sumRow(mat[1],3)==15", sumRow(mat3x3[1], 3) == 15);
    check("sumRow(mat[2],3)==24", sumRow(mat3x3[2], 3) == 24);
}

void tSubExpr() {
    int i, j;
    printf("\n--- Subscript with expressions ---\n");
    i = 1;
    j = 2;
    check("mat[i][j]==6", mat3x3[i][j] == 6);
    check("mat[i+1][j-2]==7", mat3x3[i + 1][j - 2] == 7);
}

void tGlb2DStruc() {
    int r, c;
    printf("\n--- Global 2D struct point (assign + read) ---\n");
    // fill pts[2][3] with r*10+c pattern
    r = 0;
    while (r < 2) {
        c = 0;
        while (c < 3) {
            pts[r][c].x = r * 10 + c;
            pts[r][c].y = r * 100 + c * 10;
            ++c;
        }
        ++r;
    }
    check("pts[0][0].x==0",   pts[0][0].x == 0);
    check("pts[0][0].y==0",   pts[0][0].y == 0);
    check("pts[0][2].x==2",   pts[0][2].x == 2);
    check("pts[0][2].y==20",  pts[0][2].y == 20);
    check("pts[1][0].x==10",  pts[1][0].x == 10);
    check("pts[1][0].y==100", pts[1][0].y == 100);
    check("pts[1][2].x==12",  pts[1][2].x == 12);
    check("pts[1][2].y==120", pts[1][2].y == 120);
}

void tGlb2DCell() {
    printf("\n--- Global 2D struct cell (char+int) ---\n");
    cells[0][0].tag = 'A';
    cells[0][0].val = 100;
    cells[2][1].tag = 'Z';
    cells[2][1].val = 999;
    check("cells[0][0].tag=='A'", cells[0][0].tag == 'A');
    check("cells[0][0].val==100", cells[0][0].val == 100);
    check("cells[2][1].tag=='Z'", cells[2][1].tag == 'Z');
    check("cells[2][1].val==999", cells[2][1].val == 999);
}

void tLcl2DStruc() {
    struct point lp[2][2];
    printf("\n--- Local 2D struct point ---\n");
    lp[0][0].x = 1;  lp[0][0].y = 2;
    lp[0][1].x = 3;  lp[0][1].y = 4;
    lp[1][0].x = 5;  lp[1][0].y = 6;
    lp[1][1].x = 7;  lp[1][1].y = 8;
    check("lp[0][0].x==1", lp[0][0].x == 1);
    check("lp[0][1].y==4", lp[0][1].y == 4);
    check("lp[1][0].x==5", lp[1][0].x == 5);
    check("lp[1][1].y==8", lp[1][1].y == 8);
}

void tStrucExpr() {
    int i, j;
    printf("\n--- 2D struct with expression subscripts ---\n");
    // pts was filled in tGlb2DStruc
    i = 1;
    j = 1;
    check("pts[i][j].x==11",  pts[i][j].x == 11);
    check("pts[i][j].y==110", pts[i][j].y == 110);
    check("pts[i-1][j+1].x==2", pts[i - 1][j + 1].x == 2);
}

void tLclFlat() {
    int lf[2][3] = {10, 20, 30, 40, 50, 60};
    printf("\n--- Local 2D int (flat init) ---\n");
    check("lf[0][0]==10", lf[0][0] == 10);
    check("lf[0][2]==30", lf[0][2] == 30);
    check("lf[1][0]==40", lf[1][0] == 40);
    check("lf[1][2]==60", lf[1][2] == 60);
}

void tLclCharStr() {
    char n[2][6] = {{"hello"}, {"world"}};
    printf("\n--- Local 2D char (string init) ---\n");
    check("n[0][0]=='h'", n[0][0] == 'h');
    check("n[0][4]=='o'", n[0][4] == 'o');
    check("n[1][0]=='w'", n[1][0] == 'w');
    check("n[1][4]=='d'", n[1][4] == 'd');
    check("n[0][5]==0",   n[0][5] == 0);
}

void tParam2D() {
    printf("\n--- 2D array function parameter ---\n");
    check("sum2D(mat,3)==45", sum2D(mat3x3, 3) == 45);
}

// Regression tests for BPD (long/dword) element stores in local 2D arrays.
// Before the genStore fix, initLocMDSub open-coded PUTwp1/PUTbp1 for emit and
// zero-fill, silently using the byte-store path for 4-byte (long) elements.

// Print hi/lo words of a long and whether it equals the expected value.
// On 8086, long is stored little-endian: low word at &v, high word at &v+2.
void checkLong(char *desc, long got, long exp) {
    int *p;
    p = &got;
    printf("  %s: got lo=%d hi=%d", desc, *p, *(p+1));
    p = &exp;
    printf(" exp lo=%d hi=%d", *p, *(p+1));
    if (got == exp) {
        printf(" PASS\n");
        passed++;
    }
    else {
        printf(" FAIL\n");
        getchar();
        failed++;
    }
}

// Nested-brace form: alternates values with zero and non-zero hi words.
// 65537L = 0x00010001: lo=1, hi=1.  131075L = 0x00020003: lo=3, hi=2.
void tLclLongNested() {
    long ln[2][2] = {{1L, 65537L}, {131075L, 4L}};
    int *p;
    printf("\n--- Local 2D long (nested brace init) ---\n");
    p = ln;
    printf("  raw words: %d %d %d %d %d %d %d %d\n",
        *p, *(p+1), *(p+2), *(p+3),
        *(p+4), *(p+5), *(p+6), *(p+7));
    checkLong("ln[0][0]", ln[0][0], 1L);        // lo=1, hi=0
    checkLong("ln[0][1]", ln[0][1], 65537L);    // lo=1, hi=1
    checkLong("ln[1][0]", ln[1][0], 131075L);   // lo=3, hi=2
    checkLong("ln[1][1]", ln[1][1], 4L);        // lo=4, hi=0
    // explicit hi-word checks via int pointer
    p = &ln[0][1];
    check("ln[0][1] lo==1", *p == 1);
    check("ln[0][1] hi==1", *(p+1) == 1);
    p = &ln[1][0];
    check("ln[1][0] lo==3", *p == 3);
    check("ln[1][0] hi==2", *(p+1) == 2);
}

// Flat form: alternates lo-only and hi+lo values in row-major order.
// 65546L = 65536+10: lo=10, hi=1.  131102L = 2*65536+30: lo=30, hi=2.
void tLclLongFlat() {
    long lf[2][2] = {10L, 65546L, 131102L, 40L};
    int *p;
    printf("\n--- Local 2D long (flat init) ---\n");
    p = lf;
    printf("  raw words: %d %d %d %d %d %d %d %d\n",
        *p, *(p+1), *(p+2), *(p+3),
        *(p+4), *(p+5), *(p+6), *(p+7));
    checkLong("lf[0][0]", lf[0][0], 10L);       // lo=10, hi=0
    checkLong("lf[0][1]", lf[0][1], 65546L);    // lo=10, hi=1
    checkLong("lf[1][0]", lf[1][0], 131102L);   // lo=30, hi=2
    checkLong("lf[1][1]", lf[1][1], 40L);       // lo=40, hi=0
    // explicit hi-word checks via int pointer
    p = &lf[0][1];
    check("lf[0][1] lo==10", *p == 10);
    check("lf[0][1] hi==1",  *(p+1) == 1);
    p = &lf[1][0];
    check("lf[1][0] lo==30", *p == 30);
    check("lf[1][0] hi==2",  *(p+1) == 2);
}

// Increment of a 2D long element that overflows the lo word into the hi word,
// and decrement that borrows back from the hi word into the lo word.
// 65535L++ should yield lo=0, hi=1; 65536L-- should yield lo=-1 (0xFFFF), hi=0.
void tLclLongBoundary() {
    long a[1][2];
    int *p;
    printf("\n--- 2D long elem inc/dec word boundary ---\n");
    // increment overflow: lo wraps 0xFFFF -> 0x0000, carry into hi
    a[0][0] = 65535L;
    a[0][0]++;
    p = &a[0][0];
    check("65535++ lo==0", *p == 0);
    check("65535++ hi==1", *(p+1) == 1);
    // decrement underflow: hi borrows back, lo goes from 0x0000 to 0xFFFF
    a[0][1] = 65536L;
    a[0][1]--;
    p = &a[0][1];
    check("65536-- lo==-1", *p == -1);  // 0xFFFF == -1 as signed 16-bit
    check("65536-- hi==0",  *(p+1) == 0);
}

// ============================================================================
// Main
// ============================================================================

void main() {
    passed = 0;
    failed = 0;
    printf("Multi-Dimensional Array Tests\n");
    printf("==============================\n");
    tGlb2DInt();
    tGlb2DChar();
    tGlbFlat();
    tGlbPartial();
    tGlb3D();
    tLocal2D();
    tLocalInit();
    tGridZero();
    tRowParam();
    tSubExpr();
    tGlb2DStruc();
    tGlb2DCell();
    tLcl2DStruc();
    tStrucExpr();
    tLclFlat();
    tLclCharStr();
    tParam2D();
    tLclLongNested();
    tLclLongFlat();
    tLclLongBoundary();
    printf("\n==============================\n");
    printf("Results: %d passed, %d failed\n",
        passed, failed);
    if (failed)
        printf("*** SOME TESTS FAILED ***\n");
    else
        printf("All tests passed!\n");
    if (failed) getchar();
}
