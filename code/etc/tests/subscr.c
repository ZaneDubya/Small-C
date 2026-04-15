// subscr.c -- Test subscript E1[E2] as lvalue: all base forms, all element
//             types, compound ops, increment/decrement, and 2D arrays.
//
// Purpose:
//   Verify that subscript expressions work as lvalues (assignable) when
//   the base expression is not a simple named symbol, and that element-size
//   scaling is correct for every supported element type.  Also verifies
//   correct lvalue behaviour for global storage, compound assignment,
//   prefix/postfix increment, and multi-dimensional arrays.
//
// Functionality covered:
//   - Named array subscript: arr[0], arr[n]  (regression baseline)
//   - Pointer variable subscript: p[n] = v   (char pointer)
//   - Macro-constant index: arr[SZ - 2] = v  (original failing case)
//   - Cast-expression subscript: ((char *)intarr)[n] = v (write-through)
//   - Double-pointer chain: pp[0][1] = v     (depth-2 subscript)
//   - Arithmetic-base subscript: (p+1)[0] = v
//   - All constant-index variants: p[0], p[1], p[SZ]
//   - Function-return subscript read: getptr()[0]
//   - int / unsigned array subscript (BPW x2 element scale)
//   - long / unsigned long array subscript (BPD x4 element scale)
//   - unsigned char array subscript (x1, unsigned fetch)
//   - Cast to (int *) subscript (BPW scale on byte buffer)
//   - Cast to (long *) subscript (BPD scale on byte buffer)
//   - Struct array subscript (element scale = sizeof struct)
//   - char array subscript lvalue: local, global, constant index, variable index
//   - int array subscript lvalue: local, global, constant index, variable index
//   - long array subscript lvalue: local, global (catches 32-bit store errors)
//   - char pointer subscript lvalue (ptr[i] = ch form)
//   - int pointer subscript lvalue
//   - long pointer subscript lvalue
//   - #define constant as subscript index (LAST, MID constants)
//   - Expression as subscript index (i+1, i*2)
//   - Compound assignment through subscript (+=, -=, *=)
//   - Prefix ++ and -- through subscript
//   - Postfix ++ and -- through subscript
//   - 2D int array: arr[row][col] = val (row-major lvalue)
//   - 2D long array: same, checks 32-bit element stores in 2D context
//   - Global pointer subscript lvalue (pointer defined globally,
//     assigned to a local array, then written via ptr[idx])

#include <stdio.h>

int passed, failed;

#define SZ   8
#define LAST 4
#define MID  2
#define ZERO 0

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
// Global arrays and pointers
// ============================================================================

char  gc[5];
int   gi[5];
long  gl[5];

char  *gcp;
int   *gip;
long  *glp;

// ============================================================================
// Phase 1: Named array -- regression baseline
// Classic arr[constant] and arr[variable] as lvalue.
// ============================================================================
void test_named() {
    char buf[8];
    int  n;

    printf("--- named array ---\n");

    buf[0] = 10;
    check("named[0]=10",   buf[0] == 10);

    buf[3] = 42;
    check("named[3]=42",   buf[3] == 42);

    n = 5;
    buf[n] = 99;
    check("named[n]=99",   buf[n] == 99);

    // index 0 after non-zero writes must be unaffected
    check("named[0] still 10", buf[0] == 10);
}

// ============================================================================
// Phase 2: Pointer variable subscript
// char *p; p[n] = v;  -- p is a named pointer, not an array name.
// ============================================================================
void test_ptrvar() {
    char buf[4];
    char *p;
    int  n;

    printf("--- ptr variable ---\n");

    p = buf;
    p[0] = 'A';
    check("pvar[0]='A'",   p[0] == 'A');

    p[2] = 'Z';
    check("pvar[2]='Z'",   p[2] == 'Z');

    n = 1;
    p[n] = 'B';
    check("pvar[n]='B'",   p[n] == 'B');

    // reads via buf[] must agree
    check("buf[0]='A'",    buf[0] == 'A');
    check("buf[1]='B'",    buf[1] == 'B');
    check("buf[2]='Z'",    buf[2] == 'Z');
}

// ============================================================================
// Phase 3: Macro-constant index  (the original failing case)
// arr[SZ - 2] = v;  where SZ is a #define -- the index is folded at
// compile time to a constant, which triggered the old down2 dead-code bug.
// ============================================================================
void test_define() {
    char buf[SZ];
    int  i;

    printf("--- macro index ---\n");

    // SZ - 2 == 6
    buf[SZ - 2] = 77;
    check("buf[SZ-2]=77",  buf[SZ - 2] == 77);

    // SZ - 1 == 7
    buf[SZ - 1] = 88;
    check("buf[SZ-1]=88",  buf[SZ - 1] == 88);

    // plain SZ-constant index: index 0
    buf[0] = 11;
    check("buf[0]=11",     buf[0] == 11);

    // neighbours must be unaffected
    check("buf[5] zero",   buf[5] == 0 || buf[5] != 77);  /* flexible: just no crash */
    check("SZ-2 still 77", buf[SZ - 2] == 77);
}

// ============================================================================
// Phase 4: Cast-expression subscript as lvalue
// ((char *)intarr)[n] = v;  -- base is a cast expression, not a named array.
// ============================================================================
void test_cast() {
    int  ia[4];
    char *cp;
    int  n;

    printf("--- cast subscript ---\n");

    ia[0] = 0;
    ia[1] = 0;
    ia[2] = 0;
    ia[3] = 0;

    // Write byte 0 of ia[0] via cast subscript
    ((char *)ia)[0] = 5;
    cp = ia;
    check("cast[0]=5",     cp[0] == 5);

    // Write byte 1 of ia[0]
    ((char *)ia)[1] = 7;
    check("cast[1]=7",     cp[1] == 7);

    // Write via variable index
    n = 2;
    ((char *)ia)[n] = 9;
    check("cast[n]=9",     cp[2] == 9);

    // Read back through original int array: byte 0 of ia[0] == 5, byte 1 == 7
    // low byte of ia[0] is cp[0], high byte is cp[1]
    // ia[0] should be (7 << 8) | 5 == 1797
    check("ia[0]=1797",    ia[0] == 1797);
}

// ============================================================================
// Phase 5: Double-pointer chain subscript
// char *ptrs[2]; ptrs[0] = buf; ptrs[0][1] = v;
// ============================================================================
void test_depth2() {
    char buf0[4];
    char buf1[4];
    char *ptrs[2];

    printf("--- depth-2 ptr ---\n");

    buf0[0] = 0;  buf0[1] = 0;  buf0[2] = 0;  buf0[3] = 0;
    buf1[0] = 0;  buf1[1] = 0;  buf1[2] = 0;  buf1[3] = 0;

    ptrs[0] = buf0;
    ptrs[1] = buf1;

    // subscript of subscript  (implements pp[0][1])
    ptrs[0][1] = 55;
    printf("  [diag] buf0[0]=%d buf0[1]=%d buf0[2]=%d\n",
           buf0[0], buf0[1], buf0[2]);
    check("pp[0][1]=55",   buf0[1] == 55);

    ptrs[1][0] = 66;
    check("pp[1][0]=66",   buf1[0] == 66);

    ptrs[0][0] = 11;
    check("pp[0][0]=11",   buf0[0] == 11);

    // reads through the pointer array must agree
    check("ptrs[0][1]",    ptrs[0][1] == 55);
    check("ptrs[1][0]",    ptrs[1][0] == 66);
}

// ============================================================================
// Phase 6: Arithmetic-expression base subscript
// (p + offset)[n] = v;  -- base is a pointer arithmetic expression.
// ============================================================================
void test_arith() {
    char buf[6];
    char *p;
    int  i;

    printf("--- arith base ---\n");

    for (i = 0; i < 6; i++) buf[i] = 0;

    p = buf;

    // (p + 1)[0] writes buf[1]
    (p + 1)[0] = 33;
    check("(p+1)[0]=33",   buf[1] == 33);

    // (p + 2)[1] writes buf[3]
    (p + 2)[1] = 44;
    check("(p+2)[1]=44",   buf[3] == 44);

    // (p + 0)[0] writes buf[0]
    (p + 0)[0] = 22;
    check("(p+0)[0]=22",   buf[0] == 22);

    check("buf[0]=22",     buf[0] == 22);
    check("buf[1]=33",     buf[1] == 33);
    check("buf[3]=44",     buf[3] == 44);
}

// ============================================================================
// Phase 7: Constant-index variants
// p[0], p[1], p[2], p[SZ-1] -- various compile-time constant forms.
// ============================================================================
void test_constidx() {
    char buf[SZ];
    char *p;

    printf("--- const index ---\n");

    p = buf;
    p[0] = 1;
    p[1] = 2;
    p[2] = 3;
    p[SZ - 1] = 127;

    check("p[0]=1",        p[0] == 1);
    check("p[1]=2",        p[1] == 2);
    check("p[2]=3",        p[2] == 3);
    check("p[SZ-1]=127",   p[SZ - 1] == 127);

    // overwrite p[1] with a macro-derived constant
    p[SZ - SZ + 1] = 99;
    check("p[SZ-SZ+1]=99", p[1] == 99);
}

// ============================================================================
// Phase 8: Function-return subscript read
// getptr()[0] -- base is a function return value (rvalue pointer).
// Read-only here since the function returns a pointer to a local-static buffer.
// ============================================================================
char gretbuf[4];

char *getptr() {
    gretbuf[0] = 10;
    gretbuf[1] = 20;
    gretbuf[2] = 30;
    gretbuf[3] = 40;
    return gretbuf;
}

void test_retval() {
    int v;

    printf("--- func return subscript ---\n");

    v = getptr()[0];
    check("fnret[0]=10",   v == 10);

    v = getptr()[2];
    check("fnret[2]=30",   v == 30);

    v = getptr()[3];
    check("fnret[3]=40",   v == 40);
}

// ============================================================================
// Phase 9: int array subscript (BPW x2 element scale)
// int arr[N]; arr[n] = v;  -- reads/writes must land on correct 2-byte slot.
// ============================================================================
void test_int() {
    int  ia[4];
    int  *ip;
    int  n;

    printf("--- int array ---\n");

    ia[0] = 1000;
    ia[1] = 2000;
    ia[2] = 3000;
    ia[3] = 4000;

    check("int[0]=1000",   ia[0] == 1000);
    check("int[1]=2000",   ia[1] == 2000);
    check("int[2]=3000",   ia[2] == 3000);
    check("int[3]=4000",   ia[3] == 4000);

    ip = ia;
    ip[2] = 9999;
    check("intp[2]=9999",  ia[2] == 9999);

    n = 1;
    ip[n] = 7777;
    check("intp[n]=7777",  ia[1] == 7777);

    // neighbours must be unaffected
    check("int[0] still",  ia[0] == 1000);
    check("int[3] still",  ia[3] == 4000);
}

// ============================================================================
// Phase 10: unsigned array subscript (BPW x2, unsigned values)
// ============================================================================
void test_uint() {
    unsigned ua[4];
    unsigned *up;
    int      n;

    printf("--- unsigned array ---\n");

    ua[0] = 60000;
    ua[1] = 61000;
    ua[2] = 62000;
    ua[3] = 63000;

    check("uint[0]=60000",  ua[0] == 60000);
    check("uint[1]=61000",  ua[1] == 61000);

    up = ua;
    n = 2;
    up[n] = 50000;
    check("uintp[2]=50000", ua[2] == 50000);

    up[3] = 40000;
    check("uintp[3]=40000", ua[3] == 40000);

    check("uint[0] still",  ua[0] == 60000);
}

// ============================================================================
// Phase 11: long array subscript (BPD x4 element scale)
// ============================================================================
void test_long() {
    long  la[4];
    long  *lp;
    long  v;
    int   n;

    printf("--- long array ---\n");

    la[0] = 100000;
    la[1] = 200000;
    la[2] = 300000;
    la[3] = 400000;

    check("lng[0]=100000",  la[0] == 100000);
    check("lng[1]=200000",  la[1] == 200000);
    check("lng[2]=300000",  la[2] == 300000);
    check("lng[3]=400000",  la[3] == 400000);

    lp = la;
    lp[1] = 999999;
    check("lngp[1]=999999", la[1] == 999999);

    n = 3;
    lp[n] = 111111;
    check("lngp[n]=111111", la[3] == 111111);

    check("lng[0] still",   la[0] == 100000);
    check("lng[2] still",   la[2] == 300000);
}

// ============================================================================
// Phase 12: unsigned long array subscript (BPD x4)
// ============================================================================
void test_ulong() {
    unsigned long ula[3];
    unsigned long *ulp;
    int           n;

    printf("--- ulong array ---\n");

    ula[0] = 70000;
    ula[1] = 80000;
    ula[2] = 90000;

    check("ulng[0]=70000",  ula[0] == 70000);
    check("ulng[1]=80000",  ula[1] == 80000);

    ulp = ula;
    n = 2;
    ulp[n] = 12345;
    check("ulngp[2]=12345", ula[2] == 12345);

    ulp[0] = 99999;
    check("ulngp[0]=99999", ula[0] == 99999);
    check("ulng[1] still",  ula[1] == 80000);
}

// ============================================================================
// Phase 13: unsigned char array subscript (x1, unsigned fetch)
// Reads must zero-extend (values 128..255 must stay positive when assigned
// to an int, distinguishing from signed char which would sign-extend).
// ============================================================================
void test_uchar() {
    unsigned char uca[4];
    unsigned char *up;
    int v;

    printf("--- uchar array ---\n");

    uca[0] = 200;
    uca[1] = 255;
    uca[2] = 128;
    uca[3] = 0;

    v = uca[0];  check("uchar[0]=200",  v == 200);
    v = uca[1];  check("uchar[1]=255",  v == 255);
    v = uca[2];  check("uchar[2]=128",  v == 128);

    up = uca;
    up[3] = 77;
    v = up[3];   check("ucharp[3]=77",  v == 77);
    check("uchar[0] still", uca[0] == 200);
}

// ============================================================================
// Phase 14: Cast to (int *) subscript -- BPW scale on a byte buffer.
// ((int *)charbuf)[n] must advance by 2 bytes per index slot.
// ============================================================================
void test_castp_int() {
    char cb[8];
    int  *ip;
    int  n;
    int  i;

    printf("--- cast (int*) subscript ---\n");

    for (i = 0; i < 8; i++) cb[i] = 0;

    // Write 0x0102 to bytes [0..1] via (int *) cast subscript
    ((int *)cb)[0] = 0x0102;
    ip = cb;
    check("(int*)cb[0]=0x0102", ip[0] == 0x0102);

    // Write 0x0304 to bytes [2..3]
    ((int *)cb)[1] = 0x0304;
    check("(int*)cb[1]=0x0304", ip[1] == 0x0304);

    // Variable index
    n = 2;
    ((int *)cb)[n] = 0x0506;
    check("(int*)cb[2]=0x0506", ip[2] == 0x0506);

    // Byte-level verification: slot 0 must be {0x02, 0x01} (little-endian)
    check("byte[0]=2",          cb[0] == 2);
    check("byte[1]=1",          cb[1] == 1);
    check("byte[2]=4",          cb[2] == 4);
    check("byte[3]=3",          cb[3] == 3);
}

// ============================================================================
// Phase 15: Cast to (long *) subscript -- BPD x4 scale on a byte buffer.
// ((long *)charbuf)[n] must advance by 4 bytes per index slot.
// ============================================================================
void test_castp_long() {
    char cb[12];
    long v;
    char *cp;
    int  i;

    printf("--- cast (long*) subscript ---\n");

    for (i = 0; i < 12; i++) cb[i] = 0;

    ((long *)cb)[0] = 70000;   /* 0x00011170 */
    ((long *)cb)[1] = 80000;   /* 0x00013880 */
    ((long *)cb)[2] = 90000;   /* 0x00015F90 */

    // Read back via long pointer and compare
    v = ((long *)cb)[0];
    check("(lng*)cb[0]=70000",  v == 70000);

    v = ((long *)cb)[1];
    check("(lng*)cb[1]=80000",  v == 80000);

    v = ((long *)cb)[2];
    check("(lng*)cb[2]=90000",  v == 90000);

    // Byte-level: 70000 = 0x00011170; little-endian -> [0x70, 0x11, 0x01, 0x00]
    cp = cb;
    check("lng byte0=0x70",     cp[0] == 0x70);
    check("lng byte1=0x11",     cp[1] == 0x11);
    check("lng byte2=0x01",     cp[2] == 0x01);
    check("lng byte3=0x00",     cp[3] == 0x00);
}

// ============================================================================
// Phase 16: Struct array subscript (element scale = sizeof struct)
// struct { int x; int y; } sa[3]; sa[n].x = v;
// Each struct is 4 bytes; indexing must advance by 4.
// ============================================================================
struct Point { int x; int y; };

void test_struct() {
    struct Point sa[3];
    struct Point *sp;
    int n;

    printf("--- struct array ---\n");

    sa[0].x = 10;  sa[0].y = 11;
    sa[1].x = 20;  sa[1].y = 21;
    sa[2].x = 30;  sa[2].y = 31;

    check("sa[0].x=10",    sa[0].x == 10);
    check("sa[0].y=11",    sa[0].y == 11);
    check("sa[1].x=20",    sa[1].x == 20);
    check("sa[1].y=21",    sa[1].y == 21);
    check("sa[2].x=30",    sa[2].x == 30);
    check("sa[2].y=31",    sa[2].y == 31);

    // Write via pointer variable subscript
    sp = sa;
    sp[1].x = 99;
    check("sp[1].x=99",    sa[1].x == 99);

    n = 2;
    sp[n].y = 77;
    check("sp[n].y=77",    sa[2].y == 77);

    // Neighbours must be unaffected
    check("sa[0].x still", sa[0].x == 10);
    check("sa[0].y still", sa[0].y == 11);
    check("sa[2].x still", sa[2].x == 30);
}

// ============================================================================
// Phase 17 (merged): char array lvalue -- local and global
// ============================================================================

void test_chrarr() {
    char loc[5];
    int  i;

    printf("--- char array lvalue ---\n");

    // local: variable index
    i = 0;
    while (i < 5) { loc[i] = 0; i++; }
    loc[0] = 'A';
    loc[1] = 'B';
    loc[2] = 'C';
    loc[3] = 'D';
    loc[4] = 'E';
    check("loc[0]='A'",   loc[0] == 'A');
    check("loc[4]='E'",   loc[4] == 'E');

    // local: #define constant index
    loc[LAST] = 'Z';
    check("loc[LAST]='Z'", loc[LAST] == 'Z');
    loc[MID]  = 'M';
    check("loc[MID]='M'",  loc[MID]  == 'M');
    loc[ZERO] = 'X';
    check("loc[ZERO]='X'", loc[ZERO] == 'X');

    // local: expression index
    i = 1;
    loc[i + 1] = 'Q';
    check("loc[i+1]='Q'", loc[2] == 'Q');

    // global: same checks
    gc[0] = 'a';
    gc[4] = 'e';
    check("gc[0]='a'",    gc[0] == 'a');
    check("gc[4]='e'",    gc[4] == 'e');
    gc[LAST] = 'z';
    check("gc[LAST]='z'", gc[LAST] == 'z');
    gc[MID]  = 'm';
    check("gc[MID]='m'",  gc[MID]  == 'm');
}

// ============================================================================
// Phase 18 (merged): int array lvalue -- local and global
// ============================================================================

void test_intarr() {
    int  loc[5];
    int  i;

    printf("--- int array lvalue ---\n");

    i = 0;
    while (i < 5) { loc[i] = 0; i++; }

    loc[0] = 10;
    loc[1] = 20;
    loc[2] = 30;
    loc[3] = 40;
    loc[4] = 50;
    check("loc[0]=10",    loc[0] == 10);
    check("loc[4]=50",    loc[4] == 50);

    // #define constant index
    loc[LAST] = 999;
    check("loc[LAST]=999", loc[LAST] == 999);
    loc[MID]  = 500;
    check("loc[MID]=500",  loc[MID]  == 500);
    loc[ZERO] = -1;
    check("loc[ZERO]=-1",  loc[ZERO] == -1);

    // expression index
    i = 2;
    loc[i * 2] = 88;
    check("loc[i*2]=88", loc[4] == 88);

    // global
    gi[0] = 100;
    gi[4] = 400;
    check("gi[0]=100",   gi[0] == 100);
    check("gi[4]=400",   gi[4] == 400);
    gi[LAST] = 777;
    check("gi[LAST]=777", gi[LAST] == 777);
    gi[MID]  = 222;
    check("gi[MID]=222",  gi[MID]  == 222);
}

// ============================================================================
// Phase 19 (merged): long array lvalue -- local and global
// Bug risk: 32-bit store may only write 16 bits when subscript is constant.
// ============================================================================

void test_longarr() {
    long loc[5];
    int  i;

    printf("--- long array lvalue ---\n");

    i = 0;
    while (i < 5) { loc[i] = 0L; i++; }

    loc[0] = 100000L;
    loc[4] = 200000L;
    check("loc[0]=100000", loc[0] == 100000L);
    check("loc[4]=200000", loc[4] == 200000L);

    // #define constant index -- catches 16-bit truncation of 32-bit value
    loc[LAST] = 999999L;
    check("loc[LAST]=999999", loc[LAST] == 999999L);
    loc[MID]  = -32769L;
    check("loc[MID]=-32769",  loc[MID]  == -32769L);
    loc[ZERO] = 65536L;
    check("loc[ZERO]=65536",  loc[ZERO] == 65536L);

    // expression index
    i = 1;
    loc[i + 1] = 131072L;
    check("loc[i+1]=131072", loc[2] == 131072L);

    // global
    gl[0] = 1000000L;
    gl[4] = 2000000L;
    check("gl[0]=1000000", gl[0] == 1000000L);
    check("gl[4]=2000000", gl[4] == 2000000L);
    gl[LAST] = 888888L;
    check("gl[LAST]=888888", gl[LAST] == 888888L);
    gl[MID]  = -65537L;
    check("gl[MID]=-65537",  gl[MID]  == -65537L);
}

// ============================================================================
// Phase 20 (merged): char pointer subscript lvalue
// Bug risk: ptr[CONST] = val may error "lvalue cannot be assigned" when
// ptr is a pointer (not array) and the index is a preprocessor constant.
// ============================================================================

void test_chrptr() {
    char  buf[5];
    char  *p;
    int   i;

    printf("--- char ptr subscript lvalue ---\n");

    i = 0;
    while (i < 5) { buf[i] = 0; i++; }
    p = buf;

    // variable index
    p[0] = 'a';
    p[3] = 'd';
    check("p[0]='a'",    p[0] == 'a' && buf[0] == 'a');
    check("p[3]='d'",    p[3] == 'd' && buf[3] == 'd');

    // #define constant index (the bug case)
    p[LAST] = 'z';
    check("p[LAST]='z'", p[LAST] == 'z' && buf[LAST] == 'z');
    p[MID]  = 'm';
    check("p[MID]='m'",  p[MID]  == 'm' && buf[MID]  == 'm');
    p[ZERO] = 'x';
    check("p[ZERO]='x'", p[ZERO] == 'x' && buf[ZERO] == 'x');

    // expression index
    i = 1;
    p[i + 2] = 'Q';
    check("p[i+2]='Q'", buf[3] == 'Q');

    // global pointer
    gcp = buf;
    gcp[0] = 'G';
    check("gcp[0]='G'", buf[0] == 'G');
    gcp[LAST] = 'L';
    check("gcp[LAST]='L'", buf[LAST] == 'L');
}

// ============================================================================
// Phase 21 (merged): int pointer subscript lvalue
// ============================================================================

void test_intptr() {
    int  buf[5];
    int  *p;
    int  i;

    printf("--- int ptr subscript lvalue ---\n");

    i = 0;
    while (i < 5) { buf[i] = 0; i++; }
    p = buf;

    p[0] = 11;
    p[3] = 33;
    check("p[0]=11",    p[0] == 11 && buf[0] == 11);
    check("p[3]=33",    p[3] == 33 && buf[3] == 33);

    p[LAST] = 55;
    check("p[LAST]=55", p[LAST] == 55 && buf[LAST] == 55);
    p[MID]  = 22;
    check("p[MID]=22",  p[MID]  == 22 && buf[MID]  == 22);
    p[ZERO] = -7;
    check("p[ZERO]=-7", p[ZERO] == -7 && buf[ZERO] == -7);

    i = 2;
    p[i + 1] = 99;
    check("p[i+1]=99", buf[3] == 99);

    // global pointer to local buffer
    gip = buf;
    gip[0] = 101;
    check("gip[0]=101", buf[0] == 101);
    gip[LAST] = 505;
    check("gip[LAST]=505", buf[LAST] == 505);
}

// ============================================================================
// Phase 22 (merged): long pointer subscript lvalue
// ============================================================================

void test_lngptr() {
    long buf[5];
    long *p;
    int  i;

    printf("--- long ptr subscript lvalue ---\n");

    i = 0;
    while (i < 5) { buf[i] = 0L; i++; }
    p = buf;

    p[0] = 100000L;
    p[3] = 300000L;
    check("p[0]=100000", p[0] == 100000L && buf[0] == 100000L);
    check("p[3]=300000", p[3] == 300000L && buf[3] == 300000L);

    p[LAST] = 999999L;
    check("p[LAST]=999999", p[LAST] == 999999L && buf[LAST] == 999999L);
    p[MID]  = -65537L;
    check("p[MID]=-65537",  p[MID]  == -65537L && buf[MID]  == -65537L);
    p[ZERO] = 131072L;
    check("p[ZERO]=131072", p[ZERO] == 131072L && buf[ZERO] == 131072L);

    // global pointer
    glp = buf;
    glp[0] = 111111L;
    check("glp[0]=111111", buf[0] == 111111L);
    glp[LAST] = 444444L;
    check("glp[LAST]=444444", buf[LAST] == 444444L);
}

// ============================================================================
// Phase 23 (merged): Compound assignment through subscript (+=, -=, *=)
// Bug risk: compound op may not treat subscript as lvalue for the store half.
// ============================================================================

void test_compnd() {
    int  arr[5];
    int  *p;
    int  i;

    printf("--- compound assign via subscript ---\n");

    i = 0;
    while (i < 5) { arr[i] = 10; i++; }
    p = arr;

    arr[0] += 5;
    check("arr[0]+=5",  arr[0] == 15);
    arr[1] -= 3;
    check("arr[1]-=3",  arr[1] == 7);
    arr[2] *= 2;
    check("arr[2]*=2",  arr[2] == 20);

    // constant index
    arr[LAST] += 100;
    check("arr[LAST]+=100", arr[LAST] == 110);
    arr[MID]  -= 1;
    check("arr[MID]-=1",    arr[MID]  == 19);  // arr[2] was 20 after *=2

    // pointer form
    p[0] += 1;
    check("p[0]+=1",    arr[0] == 16);
    p[LAST] += 1;
    check("p[LAST]+=1", arr[LAST] == 111);
    p[MID]  *= 3;
    check("p[MID]*=3",  arr[MID] == 57);  // arr[2] was 19 after -=1
}

// ============================================================================
// Phase 24 (merged): Prefix ++ and -- through subscript
// ============================================================================

void test_preinc() {
    int  arr[5];
    int  *p;

    printf("--- prefix ++/-- via subscript ---\n");

    arr[0] = 10; arr[1] = 20; arr[2] = 30; arr[3] = 40; arr[4] = 50;
    p = arr;

    ++arr[0];
    check("++arr[0]",     arr[0] == 11);
    --arr[1];
    check("--arr[1]",     arr[1] == 19);
    ++arr[LAST];
    check("++arr[LAST]",  arr[LAST] == 51);
    --arr[MID];
    check("--arr[MID]",   arr[MID]  == 29);

    ++p[0];
    check("++p[0]",       arr[0] == 12);
    --p[4];
    check("--p[4]",       arr[4] == 50);
    ++p[LAST];
    check("++p[LAST]",    arr[LAST] == 51);
    --p[MID];
    check("--p[MID]",     arr[MID]  == 28);
}

// ============================================================================
// Phase 25 (merged): Postfix ++ and -- through subscript
// ============================================================================

void test_postinc() {
    int  arr[3];
    int  *p;
    int  v;

    printf("--- postfix ++/-- via subscript ---\n");

    arr[0] = 5; arr[1] = 10; arr[2] = 15;
    p = arr;

    // postfix returns old value
    v = arr[0]++;
    check("arr[0]++ ret",  v == 5);
    check("arr[0]++ side", arr[0] == 6);

    v = arr[1]--;
    check("arr[1]-- ret",  v == 10);
    check("arr[1]-- side", arr[1] == 9);

    v = p[MID]++;
    check("p[MID]++ ret",  v == 15);
    check("p[MID]++ side", arr[MID] == 16);
}

// ============================================================================
// Phase 26 (merged): 2D int array subscript lvalue
// Bug risk: stride calculation wrong; arr[1][0] aliases arr[0][N-1].
// ============================================================================

void test_2dint() {
    int  m[3][4];
    int  r, c;

    printf("--- 2D int array lvalue ---\n");

    // zero-fill
    r = 0;
    while (r < 3) {
        c = 0;
        while (c < 4) { m[r][c] = 0; c++; }
        r++;
    }

    // write each cell with a unique value
    r = 0;
    while (r < 3) {
        c = 0;
        while (c < 4) { m[r][c] = r * 10 + c; c++; }
        r++;
    }

    check("m[0][0]=0",   m[0][0] == 0);
    check("m[0][3]=3",   m[0][3] == 3);
    check("m[1][0]=10",  m[1][0] == 10);
    check("m[1][3]=13",  m[1][3] == 13);
    check("m[2][0]=20",  m[2][0] == 20);
    check("m[2][3]=23",  m[2][3] == 23);

    // overwrite corner cells and verify no aliasing
    m[0][3] = 99;
    m[1][0] = 88;
    check("m[0][3]=99",  m[0][3] == 99);
    check("m[1][0]=88",  m[1][0] == 88);
    m[0][3] = 3;
    m[1][0] = 10;
    check("m[0][3] rest", m[0][3] == 3);
    check("m[1][0] rest", m[1][0] == 10);
}

// ============================================================================
// Phase 27 (merged): 2D long array subscript lvalue
// ============================================================================

void test_2dlong() {
    long m[2][3];
    int  r, c;

    printf("--- 2D long array lvalue ---\n");

    r = 0;
    while (r < 2) {
        c = 0;
        while (c < 3) { m[r][c] = 0L; c++; }
        r++;
    }

    m[0][0] = 100000L;
    m[0][1] = 200000L;
    m[0][2] = 300000L;
    m[1][0] = 400000L;
    m[1][1] = 500000L;
    m[1][2] = 600000L;

    check("m[0][0]",  m[0][0] == 100000L);
    check("m[0][2]",  m[0][2] == 300000L);
    check("m[1][0]",  m[1][0] == 400000L);
    check("m[1][2]",  m[1][2] == 600000L);

    // overwrite, check no bleed between neighbouring longs
    m[0][2] = -32769L;
    m[1][0] = 65536L;
    check("m[0][2] neg",    m[0][2] == -32769L);
    check("m[1][0] 65536",  m[1][0] == 65536L);
    check("m[0][1] intact", m[0][1] == 200000L);
    check("m[1][1] intact", m[1][1] == 500000L);
}

// ============================================================================

int main() {
    passed = 0;
    failed = 0;

    printf("=== subscr.c: lvalue subscript tests ===\n");

    test_named();
    test_ptrvar();
    test_define();
    test_cast();
    test_depth2();
    test_arith();
    test_constidx();
    test_retval();
    test_int();
    test_uint();
    test_long();
    test_ulong();
    test_uchar();
    test_castp_int();
    test_castp_long();
    test_struct();
    test_chrarr();
    test_intarr();
    test_longarr();
    test_chrptr();
    test_intptr();
    test_lngptr();
    test_compnd();
    test_preinc();
    test_postinc();
    test_2dint();
    test_2dlong();

    printf("\n=== Results: %d passed, %d failed ===\n", passed, failed);
    if (failed) getchar();
    if (failed) return 1;
    return 0;
}

