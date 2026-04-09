// voidptrs.c -- Test program for void * (generic pointer) support in Small-C.
//
// All variable and function names are at most 12 characters (Small-C limit).

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
// Global data used as backing store.
// These substitute for malloc() since pointer-returning functions cannot yet
// be declared with proper void* return types (needs Next Plan A).
// ============================================================================

int   gint;
char  gchar;
int   garr[4];
char  cbuf[8];

void *gvp;            // global void* variable
void *gvp2;           // second global void*

// ============================================================================
// Phase 1: sizeof tests
// ============================================================================

void test_sizeof() {
    void *lp;

    printf("--- sizeof ---\n");

    // sizeof(void *) must equal BPW = 2 on 8086
    check("sizeof(void*)==2", sizeof(void *) == 2);

    // sizeof(void) == 0 in this implementation (implementation-defined)
    check("sizeof(void)==0",  sizeof(void) == 0);

    // global and local void* occupy exactly 2 bytes
    check("sizeof(gvp)==2",   sizeof(gvp)  == 2);
    check("sizeof(lp)==2",    sizeof(lp)   == 2);
}

// ============================================================================
// Phase 2: Declaration and assignment tests
// ============================================================================

void test_decl() {
    void *p;
    void *q;
    int   x;

    printf("--- decl/assign ---\n");

    x = 42;

    // Any typed pointer can be assigned to void* without a cast (C90 rule)
    p = &x;
    check("void* = &int",   p != 0);

    // void* can be copied to another void*
    q = p;
    check("void* = void*",  q == p);

    // global void* assignment
    gint = 99;
    gvp  = &gint;
    check("gvp = &gint",    gvp == &gint);

    // void* assigned to void* (copy)
    gvp2 = gvp;
    check("gvp2 = gvp",     gvp2 == gvp);
}

// ============================================================================
// Phase 3: Cast to typed pointer and dereference
// ============================================================================

void test_castint() {
    void *p;
    int  *ip;
    int   v;

    printf("--- cast int* ---\n");

    v  = 1234;
    p  = &v;
    ip = (int *)p;
    check("(int*)p addr",   ip == &v);
    check("*(int*)p==1234", *ip == 1234);
    *ip = 5678;
    check("write via int*", v == 5678);
}

void test_castchar() {
    void *p;
    char *cp;
    char  c;

    printf("--- cast char* ---\n");

    c  = 'A';
    p  = &c;
    cp = (char *)p;
    check("(char*)p addr",  cp == &c);
    check("*(char*)p=='A'", *cp == 'A');
    *cp = 'Z';
    check("write via char*", c == 'Z');
}

void test_castuint() {
    void         *p;
    unsigned int *up;
    unsigned int  u;

    printf("--- cast uint* ---\n");

    u  = 60000;
    p  = &u;
    up = (unsigned int *)p;
    check("(uint*)p addr",    up == &u);
    check("*(uint*)p==60000", *up == 60000);
}

void test_castback() {
    // Round-trip: int array -> void* -> int*
    int  arr[3];
    void *p;
    int  *ip;

    printf("--- round-trip ---\n");

    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;
    p  = arr;
    ip = (int *)p;
    check("ip[0]==10", ip[0] == 10);
    check("ip[1]==20", ip[1] == 20);
    check("ip[2]==30", ip[2] == 30);
}

// ============================================================================
// Phase 4: void* as function parameter (accepts any pointer type)
// ============================================================================

// Receives void* and reads an int through it
int rdint(void *p) {
    int *ip;
    ip = (int *)p;
    return *ip;
}

// Writes an int through a void* parameter
void wrint(void *p, int val) {
    int *ip;
    ip  = (int *)p;
    *ip = val;
}

// Reads a char from a void* byte buffer at index idx
char rdchar(void *p, int idx) {
    char *cp;
    cp = (char *)p;
    return cp[idx];
}

void test_param() {
    int  v;
    char buf[4];

    printf("--- param ---\n");

    v = 777;
    check("rdint",           rdint(&v) == 777);
    wrint(&v, 888);
    check("wrint",           v == 888);
    buf[0] = 'H';
    buf[1] = 'i';
    buf[2] = 0;
    check("rdchar[0]=='H'",  rdchar(buf, 0) == 'H');
    check("rdchar[1]=='i'",  rdchar(buf, 1) == 'i');
}

// ============================================================================
// Phase 5: void* comparisons (equality, null check)
// ============================================================================

void test_compare() {
    void *p;
    void *q;
    int   x, y;

    printf("--- compare ---\n");

    x = 1;  y = 2;
    p = &x;  q = &y;
    check("p != q",    p != q);
    check("p == p",    p == p);
    check("q != 0",    q != 0);
    p = 0;
    check("null==0",   p == 0);
    check("!null",     !p);
    p = &x;
    check("non-null",  p != 0);
}

// ============================================================================
// Phase 6: Slab allocator pattern
// (WORKAROUND: allocbytes() stores the result in global gvp because functions
// cannot yet return void* by declaration -- needs Next Plan A.)
// ============================================================================

char slab[16];
int  slabused;

void allocbytes(int n) {
    char *cp;
    cp       = slab + slabused;
    gvp      = cp;
    slabused = slabused + n;
}

void test_slab() {
    int  *ip;
    char *cp;

    printf("--- slab ---\n");

    slabused = 0;

    allocbytes(2);
    ip  = (int *)gvp;
    *ip = 9999;
    check("slab int",       *ip == 9999);

    allocbytes(3);
    cp    = (char *)gvp;
    cp[0] = 'X';
    cp[1] = 'Y';
    cp[2] = 'Z';
    check("slab char[0]",   cp[0] == 'X');
    check("slab char[1]",   cp[1] == 'Y');
    check("slab char[2]",   cp[2] == 'Z');

    // First int must still be intact (no overlap)
    ip = (int *)slab;
    check("slab intact",    *ip == 9999);
}

// ============================================================================
// Phase 7: void* in struct member
// ============================================================================

struct vbox {
    void *data;
    int   tag;
};

struct vbox gbox;

void test_struct() {
    int  v;
    int  *ip;

    printf("--- struct member ---\n");

    v         = 42;
    gbox.data = &v;
    gbox.tag  = 7;
    check("box.data!=0",   gbox.data != 0);
    check("box.tag==7",    gbox.tag  == 7);
    ip = (int *)gbox.data;
    check("*box.data==42", *ip == 42);
    // FUTURE struct pointer cast: (struct vbox *)someptr
}

// ============================================================================
// Phase 8: Array of void pointers
// ============================================================================

void *ptrtab[4];

void test_ptrarr() {
    int  a, b, c, d;
    int *ip;

    printf("--- ptr array ---\n");

    a = 1;  b = 2;  c = 3;  d = 4;
    ptrtab[0] = &a;
    ptrtab[1] = &b;
    ptrtab[2] = &c;
    ptrtab[3] = &d;
    ip = (int *)ptrtab[0];  check("tab[0]==1", *ip == 1);
    ip = (int *)ptrtab[1];  check("tab[1]==2", *ip == 2);
    ip = (int *)ptrtab[2];  check("tab[2]==3", *ip == 3);
    ip = (int *)ptrtab[3];  check("tab[3]==4", *ip == 4);
}

// ============================================================================
// Phase 9: Type pun -- cast void* to unsigned int* reads same bit pattern
// ============================================================================

void test_typepun() {
    int          ival;
    void        *vp;
    unsigned int *up;

    printf("--- type pun ---\n");

    ival = -1;          // all bits 1: 0xFFFF == 65535 as unsigned
    vp   = &ival;
    up   = (unsigned int *)vp;
    check("pun -1 as uint", *up == 65535);
}

// ============================================================================
// Phase 10: Pass void* through multiple function levels
// ============================================================================

// Advances base by n bytes and stores result in gvp
void bumpaddr(void *base, int n) {
    char *cp;
    cp  = (char *)base;
    gvp = cp + n;
}

void test_chain() {
    char  buf[8];
    char *cp;
    int   i;

    printf("--- chain ---\n");

    i = 0;
    while (i < 8) {
        buf[i] = i + 10;
        i++;
    }
    bumpaddr(buf, 0);
    cp = (char *)gvp;
    check("buf[0]", *cp == 10);
    bumpaddr(buf, 3);
    cp = (char *)gvp;
    check("buf[3]", *cp == 13);
    bumpaddr(buf, 7);
    cp = (char *)gvp;
    check("buf[7]", *cp == 17);
}

// ============================================================================
// Phase 11: void*-returning function (Plan A: IDENT_PTR_FUNCTION)
// ============================================================================

int wrapbuf[2];

void *slabptr() {
    return (void *)slab;
}

void test_wrapper() {
    int *ip;

    printf("--- ptr-returning fn ---\n");

    wrapbuf[0] = 111;
    wrapbuf[1] = 222;
    ip = (int *)slabptr();
    ip[0] = 333;
    check("slabptr fn",    ip[0] == 333);
    ip = wrapbuf;
    check("wrap[0]==111", ip[0] == 111);
    check("wrap[1]==222", ip[1] == 222);
}

// ============================================================================
// Phase 12: Cast variety -- verify (unsigned int *), (char *), (void *) all
//           accepted syntactically with void* and typed-pointer operands
// ============================================================================

void test_casts() {
    int          iv;
    char         cv;
    unsigned int uv;
    void        *vp;
    int          *ip;
    char         *cp;
    unsigned int *up;

    printf("--- cast variety ---\n");

    iv = 10;  cv = 20;  uv = 30;

    vp = &iv;
    ip = (int *)vp;
    check("(int*)ok",   *ip == 10);

    vp = &cv;
    cp = (char *)vp;
    check("(char*)ok",  *cp == 20);

    vp = &uv;
    up = (unsigned int *)vp;
    check("(uint*)ok",  *up == 30);

    // Explicit cast to void* from typed pointer
    vp = (void *)ip;
    check("(void*)ip",  vp == ip);

    vp = (void *)cp;
    check("(void*)cp",  vp == cp);
}

// ============================================================================
// Phase 13: Pointer-returning function variety (Plan A)
// ============================================================================

int gbuf[2];
int *getibuf() { return gbuf; }

char gcbuf[4];
char *getcbuf() { return gcbuf; }

void test_ptrnfn() {
    int  *ip;
    char *cp;

    printf("--- ptr-returning fns ---\n");

    ip = getibuf();
    ip[0] = 77;  ip[1] = 88;
    check("getibuf[0]==77",  ip[0] == 77);
    check("getibuf[1]==88",  ip[1] == 88);

    cp = getcbuf();
    cp[0] = 'P';  cp[1] = 'Q';
    check("getcbuf[0]=='P'", cp[0] == 'P');
    check("getcbuf[1]=='Q'", cp[1] == 'Q');

    // chain: call immediately, cast, use
    ip = (int *)getibuf();
    check("chain cast",     ip[0] == 77);
}

// ============================================================================
// Phase 14: Struct pointer casts (Plan B)
// ============================================================================

struct spt { int x; int y; };
struct spt g_spt;

void test_strcast() {
    void       *vp;
    struct spt *sp;

    printf("--- struct ptr cast ---\n");

    g_spt.x = 10;  g_spt.y = 20;
    vp = &g_spt;
    sp = (struct spt *)vp;
    check("sp->x==10",     sp->x == 10);
    check("sp->y==20",     sp->y == 20);
    sp->x = 99;
    check("write sp->x",   g_spt.x == 99);
}

// ============================================================================
// Phase 15: long int overflow (positive -> negative)
// ============================================================================

void test_lovflow() {
    long v;
    int *p;

    printf("--- long overflow ---\n");

    p    = &v;
    p[0] = -1;        // low word  = 0xFFFF
    p[1] = 0x7FFF;    // high word = 0x7FFF  =>  v = 0x7FFFFFFF
    v    = v + 1;
    p    = &v;
    // 0x7FFFFFFF + 1 = 0x80000000 = -2147483648 (signed long)
    check("of lo word==0",    p[0] == 0);
    check("of hi==-32768",    p[1] == -32768); // 0x8000 as signed int
    // also test: subtracting 1 from 0x80000000 wraps back
    v = v - 1;
    p = &v;
    check("sub lo==-1",       p[0] == -1);
    check("sub hi==0x7FFF",   p[1] == 0x7FFF);
}

// ============================================================================
// Phase 16: cast negative long <-> unsigned long (bits preserved)
// ============================================================================

void test_lngcast() {
    long          sv;
    unsigned long uv;
    int          *p;

    printf("--- long/ulong cast ---\n");

    // Case 1: -1 (all bits set) cast to unsigned long
    p    = &sv;
    p[0] = -1;    // lo = 0xFFFF
    p[1] = -1;    // hi = 0xFFFF  =>  sv = 0xFFFFFFFF = -1L
    uv   = (unsigned long)sv;
    p    = &uv;
    check("cast -1L lo",    p[0] == -1);  // bit pattern unchanged
    check("cast -1L hi",    p[1] == -1);

    // Case 2: 0x80000000 (min signed long) cast to unsigned long
    p    = &sv;
    p[0] = 0;         // lo = 0x0000
    p[1] = -32768;    // hi = 0x8000  =>  sv = -2147483648L
    uv   = (unsigned long)sv;
    p    = &uv;
    check("cast minL lo",   p[0] == 0);
    check("cast minL hi",   p[1] == -32768);  // 0x8000 unchanged

    // Case 3: unsigned long to signed long back (round-trip)
    p    = &uv;
    p[0] = 1;
    p[1] = -32768;    // 0x80000001
    sv   = (long)uv;
    p    = &sv;
    check("back lo==1",     p[0] == 1);
    check("back hi==-32768",p[1] == -32768);
}

// ============================================================================
// Phase 17: Implicit void* -> typed pointer assignment (no cast, C90 §6.3.2.3)
// ============================================================================

void test_iconv() {
    int   x;
    void *vp;
    int  *ip;
    char  c;
    char *cp;

    printf("--- implicit conv ---\n");

    x  = 123;
    vp = &x;
    ip = vp;                    // C90: implicit void* -> int*, no cast needed
    check("impl int*",  *ip == 123);

    c  = 'Q';
    vp = &c;
    cp = vp;                    // C90: implicit void* -> char*, no cast needed
    check("impl char*", *cp == 'Q');
}

// ============================================================================
// Phase 18: memcpy/memset pattern (canonical void* stdlib usage)
// ============================================================================

void mymemcpy(void *dst, void *src, int n) {
    char *d;
    char *s;
    int   i;
    d = (char *)dst;
    s = (char *)src;
    i = 0;
    while (i < n) {
        d[i] = s[i];
        i++;
    }
}

void mymemset(void *dst, char c, int n) {
    char *d;
    int   i;
    d = (char *)dst;
    i = 0;
    while (i < n) {
        d[i] = c;
        i++;
    }
}

void test_memhelp() {
    int  msrc[3];
    int  mdst[3];
    char mbuf[4];

    printf("--- memcpy/memset ---\n");

    msrc[0] = 10;  msrc[1] = 20;  msrc[2] = 30;
    mdst[0] = 0;   mdst[1] = 0;   mdst[2] = 0;
    mymemcpy(mdst, msrc, 6);    // 3 ints * BPW=2 bytes
    check("mcpy[0]==10",    mdst[0] == 10);
    check("mcpy[1]==20",    mdst[1] == 20);
    check("mcpy[2]==30",    mdst[2] == 30);

    mymemset(mbuf, 'A', 4);
    check("mset[0]=='A'",   mbuf[0] == 'A');
    check("mset[1]=='A'",   mbuf[1] == 'A');
    check("mset[3]=='A'",   mbuf[3] == 'A');

    // Verify memset doesn't bleed beyond n bytes (mbuf is backed by stack)
    mymemset(mbuf, 0, 4);
    check("mset clear",     mbuf[0] == 0 && mbuf[3] == 0);
}

// ============================================================================
// Phase 19: (long *) and (unsigned long *) casts from void*
// ============================================================================

void test_lcast2() {
    long          lv;
    unsigned long uv;
    void         *vp;
    int          *ip;

    printf("--- void*->long* ---\n");

    // Seed lv via int* (BPD=4, two words)
    ip    = (int *)&lv;
    ip[0] = 100;
    ip[1] = 200;
    vp    = &lv;
    // Cast void* to long* and verify the value is accessible via int* of that
    ip    = (int *)(long *)vp;
    check("lp lo==100",     ip[0] == 100);
    check("lp hi==200",     ip[1] == 200);

    // Same for unsigned long*
    ip    = (int *)&uv;
    ip[0] = 300;
    ip[1] = 400;
    vp    = &uv;
    ip    = (int *)(unsigned long *)vp;
    check("up lo==300",     ip[0] == 300);
    check("up hi==400",     ip[1] == 400);
}

// ============================================================================
// Phase 20: (unsigned char *) cast -- no sign extension on read
// ============================================================================

void test_ucast() {
    unsigned char  ubuf[4];
    void          *vp;
    unsigned char *ucp;
    char          *cp;

    printf("--- unsigned char* ---\n");

    ubuf[0] = 0xFF;
    ubuf[1] = 0x80;
    ubuf[2] = 0x01;
    ubuf[3] = 0x00;
    vp  = ubuf;

    ucp = (unsigned char *)vp;
    check("uc[0]==255",     ucp[0] == 255);
    check("uc[1]==128",     ucp[1] == 128);
    check("uc[2]==1",       ucp[2] == 1);
    check("uc[3]==0",       ucp[3] == 0);

    // Signed char reads same bytes but 0xFF is -1, 0x80 is -128
    cp = (char *)vp;
    check("sc[0]==-1",      cp[0] == -1);
    check("sc[1]==-128",    cp[1] == -128);

    // Unsigned and signed diverge on same bit pattern
    check("uc!=sc 0xFF",    (int)ucp[0] != (int)cp[0]);  // 255 != -1
}

// ============================================================================
// Phase 21: void*-returning function returning NULL (0)
// ============================================================================

void *nullfn() {
    return 0;
}

void test_nullret() {
    void *p;

    printf("--- null return ---\n");

    p = nullfn();
    check("nullfn==0",   p == 0);
    check("!nullfn",     !p);
    check("slabptr!=0",  slabptr() != 0);   // non-null reference
}

// ============================================================================
// Phase 22: Immediate use of void*-returning function result (no temp store)
// ============================================================================

void test_immed() {
    int *ip;

    printf("--- immed fn use ---\n");

    // Write through a cast of the return value directly
    *(int *)slabptr() = 42;
    check("immed write",    *(int *)slabptr() == 42);

    // NOTE: C90 allows subscripting a cast function-call result directly:
    //   ((int *)slabptr())[1] = 99;
    // This is equivalent to *(((int *)slabptr()) + 1) = 99 per C90 §6.5.2.1.
    // Small-C does not support applying [] to a non-variable expression (the
    // subscript operator requires its left operand to be a named variable or
    // array, not an arbitrary rvalue). This is a missing C90 feature.
    // ((int *)slabptr())[1] = 99;
    // check("immed idx",      ((int *)slabptr())[1] == 99);
}

// ============================================================================
// Phase 23: Ternary expression with void* operands (C90 §6.5.15)
// ============================================================================

void test_ternary() {
    int   x;
    int   y;
    void *p;
    void *q;
    void *r;
    int  *ip;

    printf("--- ternary ---\n");

    x = 10;  y = 20;
    p = &x;  q = &y;

    r = (x > 0) ? p : q;
    check("tern true==p",   r == p);

    r = (x < 0) ? p : q;
    check("tern false==q",  r == q);

    // Read through ternary result
    ip = (int *)((x > 0) ? p : q);
    check("tern deref",     *ip == 10);
}

// ============================================================================
// Phase 24: Relational < > <= >= on void* within same array (C90 §6.8.6)
// ============================================================================

void test_ptrrel() {
    int   arr[4];
    void *lo;
    void *hi;
    void *mid;

    printf("--- relational ---\n");

    lo  = &arr[0];
    mid = &arr[2];
    hi  = &arr[3];

    check("lo < hi",    lo < hi);
    check("hi > lo",    hi > lo);
    check("lo <= mid",  lo <= mid);
    check("hi >= mid",  hi >= mid);
    check("lo <= lo",   lo <= lo);
    check("hi >= hi",   hi >= hi);
    check("!(lo > hi)", !(lo > hi));
}

// ============================================================================
// Phase 25: Byte-offset arithmetic via (char*)vp + n then cast back
// ============================================================================

void test_bytoff() {
    int   offar[4];
    void *base;
    char *cp;
    int  *ip;

    printf("--- byte offset ---\n");

    offar[0] = 111;
    offar[1] = 222;
    offar[2] = 333;
    offar[3] = 444;
    base = offar;

    cp = (char *)base;

    ip = (int *)(cp + 0);
    check("off+0==111",  *ip == 111);

    ip = (int *)(cp + 2);
    check("off+2==222",  *ip == 222);

    ip = (int *)(cp + 4);
    check("off+4==333",  *ip == 333);

    ip = (int *)(cp + 6);
    check("off+6==444",  *ip == 444);

    // Advance base itself via bump
    bumpaddr(base, 4);
    ip = (int *)gvp;
    check("bump+4==333", *ip == 333);
}

// ============================================================================
// Phase 26: NULL literal as void* argument
// ============================================================================

int isnullp(void *p) {
    return p == 0;
}

void test_nullarg() {
    void *vp;
    int   probe;    // local: BP-relative address, guaranteed non-null.
                    // _passed lands at data offset 0 (voidptrs.obj is linked
                    // before clib, so CALL.ASM's sentinel 'dw 1' comes after),
                    // making &passed == 0 and unsuitable as a non-null address.

    printf("--- null arg ---\n");

    probe = 1;
    check("lit 0 null",     isnullp(0));
    check("nonzero !null",  !isnullp(&probe));
    vp = 0;
    check("void* 0 null",   isnullp(vp));
    vp = &probe;
    check("void* ptr !null",!isnullp(vp));
}

// ============================================================================
// Phase 27: Nested / chained casts
// ============================================================================

void test_ncast() {
    char  nc;
    void *vp;
    char *cp;
    int  *ip;

    printf("--- nested cast ---\n");

    nc = 65;    // 'A' = 0x41

    // char* -> void* -> char* round-trip preserves address
    vp = (void *)(&nc);
    cp = (char *)vp;
    check("nc rt addr",     cp == &nc);
    check("nc rt val",      *cp == 65);

    // Two-step: (int*)(void*)(&nc) -- address is preserved through both casts
    ip = (int *)(void *)(&nc);
    check("nest addr ok",   (char *)ip == &nc);

    // Three-hop: void* -> char* -> void* -> char*
    vp = &nc;
    cp = (char *)vp;
    vp = (void *)cp;
    cp = (char *)vp;
    check("3-hop addr",     cp == &nc);
    check("3-hop val",      *cp == 65);
}

// ============================================================================
// Main
// ============================================================================

void main() {
    passed = 0;
    failed = 0;

    printf("=== Small-C void * Tests ===\n\n");

    test_sizeof();
    test_decl();
    test_castint();
    test_castchar();
    test_castuint();
    test_castback();
    test_param();
    test_compare();
    test_slab();
    test_struct();
    test_ptrarr();
    test_typepun();
    test_chain();
    test_wrapper();
    test_casts();
    test_ptrnfn();
    test_strcast();
    test_lovflow();
    test_lngcast();
    test_iconv();
    test_memhelp();
    test_lcast2();
    test_ucast();
    test_nullret();
    test_immed();
    test_ternary();
    test_ptrrel();
    test_bytoff();
    test_nullarg();
    test_ncast();

    printf("\n=== Results: %d passed, %d failed ===\n", passed, failed);
    if (failed != 0)
        getchar();
}
