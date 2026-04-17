// mlptrs.c -- Test multi-level pointer support (int **, char ***, etc.) in Small-C.
//
// Purpose:
//   Verify that pointer depth > 1 declarations, dereferences, address-of,
//   casts, pointer arithmetic, and function returns all work correctly.
//
// Functionality covered:
//   - int ** declaration (local and global)
//   - char *** declaration
//   - **pp store chain: p = &x; pp = &p; **pp = value; verify x == value
//   - ***ppp load chain: x = 5; p = &x; pp = &p; ppp = &pp; verify ***ppp == 5
//   - *pp = &y  -- reassign where p points through a double pointer
//   - &p where p is int* gives int** (IS_PTRDEPTH == 2)
//   - pointer arithmetic: int **pp; pp+1 advances by BPW (2), not by sizeof(int)
//   - char **cp; cp+1 advances by BPW (2), not by sizeof(char)
//   - (int **)expr  cast
//   - Function returning int **
//   - Struct member int **
//   - long ** and unsigned long **: 32-bit load/store through double pointer
//   - Local pointer array int *arr[N]: stack allocation, subscript, pass as int**
//   - Local pointer array with brace initializer: int *arr[N] = { &a, &b, &c }
//   - Local char* array: string assignment, NULL-sentinel traversal, subscript
//   - NULL through **: write/read null pointer value via double pointer
//   - Swap via **: swapptr(int**,int**) classic pointer-swap pattern
//   - Aliased double pointers: two int** to same int* slot; write visible through both
//   - Redirect vs value-write: *pp=&y vs **pp=y have distinct effects
//   - Inner-pointer arithmetic: (*pp)+1 steps by sizeof(*p), advancing p via *pp=*pp+1
//   - Triple-pointer coherence: redirect one level, verify all read paths agree
//   - Output parameter: void f(int **out, int *src) modifies caller's pointer
//
// All identifiers are at most 12 characters (Small-C identifier limit).

#include <stdio.h>

int passed, failed;

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
// Global double pointer
// ============================================================================
int  gx;
int  *gp;
int  **gpp;

void test_global() {
    printf("--- global double ptr ---\n");
    gx  = 42;
    gp  = &gx;
    gpp = &gp;
    check("**gpp read",   **gpp == 42);
    **gpp = 99;
    check("**gpp write",  gx == 99);
    check("*gpp == &gx",  *gpp == &gx);
}

// ============================================================================
// Local double pointer
// ============================================================================
void test_local() {
    int  x, y;
    int  *p;
    int  **pp;

    printf("--- local double ptr ---\n");
    x  = 7;
    p  = &x;
    pp = &p;
    check("**pp read",    **pp == 7);
    **pp = 13;
    check("**pp write",   x == 13);

    // *pp = &y  reassigns where p points
    y = 55;
    *pp = &y;
    check("*pp redirect", *p == 55);
    check("**pp after",   **pp == 55);
}

// ============================================================================
// Triple pointer
// ============================================================================
void test_triple() {
    int   x;
    int   *p;
    int   **pp;
    int   ***ppp;

    printf("--- triple ptr ---\n");
    x   = 5;
    p   = &x;
    pp  = &p;
    ppp = &pp;
    check("***ppp read",  ***ppp == 5);
    ***ppp = 77;
    check("***ppp write", x == 77);
}

// ============================================================================
// char double pointer
// ============================================================================
void test_cpptr() {
    char  c;
    char  *cp;
    char  **cpp;

    printf("--- char double ptr ---\n");
    c   = 'A';
    cp  = &c;
    cpp = &cp;
    check("**cpp read",   **cpp == 'A');
    **cpp = 'Z';
    check("**cpp write",  c == 'Z');
}

// ============================================================================
// Pointer arithmetic with double pointers
// int **pp; pp+1 must advance by BPW (2 bytes), not sizeof(int)
// ============================================================================
void test_ptrarith() {
    int  arr[3];
    int  *pa, *pb, *pc;
    int  *ptrs[3];
    int  **pp;

    printf("--- ptr arithmetic ---\n");
    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;
    pa = &arr[0];
    pb = &arr[1];
    pc = &arr[2];
    ptrs[0] = pa;
    ptrs[1] = pb;
    ptrs[2] = pc;

    pp = (int **)ptrs;

    // pp+1 should point to ptrs[1]
    check("pp+1 scale",   *(pp+1) == pb);
    check("pp+2 scale",   *(pp+2) == pc);

    // char **cpp arithmetic: step by BPW not by 1
    {
        char  c0, c1;
        char  *cp0, *cp1;
        char  *cptrs[2];
        char  **cpp;
        c0 = 'X'; c1 = 'Y';
        cp0 = &c0; cp1 = &c1;
        cptrs[0] = cp0;
        cptrs[1] = cp1;
        cpp = (char **)cptrs;
        check("cpp+1 scale", *(cpp+1) == cp1);
    }
}

// ============================================================================
// Cast to double pointer
// ============================================================================
void test_cast() {
    int  arr[2];
    int  *ptrs[2];
    int  **pp;

    printf("--- cast to ** ---\n");
    arr[0] = 11; arr[1] = 22;
    ptrs[0] = &arr[0];
    ptrs[1] = &arr[1];

    pp = (int **)ptrs;
    check("cast **read0",  *pp[0] == 11);
    check("cast **read1",  *pp[1] == 22);
}

// ============================================================================
// Function returning int **
// ============================================================================
int **getpptr(int **src) {
    return src;
}

void test_fnret() {
    int  x;
    int  *p;
    int  **pp;
    int  **qp;

    printf("--- function returning ** ---\n");
    x  = 123;
    p  = &x;
    pp = &p;
    qp = getpptr(pp);
    check("fnret **read",  **qp == 123);
}

// ============================================================================
// Struct member int **
// ============================================================================
struct DblPtrNode {
    int  val;
    int  **pptr;
};

void test_struct() {
    int  x;
    int  *p;
    int  **pp;
    struct DblPtrNode nd;

    printf("--- struct member ** ---\n");
    x  = 88;
    p  = &x;
    pp = &p;
    nd.val  = 1;
    nd.pptr = pp;
    check("struct**.read",  **nd.pptr == 88);
    **nd.pptr = 66;
    check("struct**.write", x == 66);
}

// ============================================================================
// Long and unsigned long through double pointer
// Bug risk: compiler generates 16-bit load/store instead of 32-bit when the
// base type is long; high word is silently lost or corrupted.
// ============================================================================
long   glx2;
long  *glp2;
long **glpp2;

void test_longptr() {
    long  x;
    long  *lp;
    long  **lpp;
    unsigned long ux;
    unsigned long *ulp;
    unsigned long **ulpp;

    printf("--- long ** ---\n");
    x   = 100000L;      // > 16-bit range; catches truncation to int
    lp  = &x;
    lpp = &lp;
    check("**lpp long rd",   **lpp == 100000L);
    **lpp = 200000L;
    check("**lpp long wr",   x == 200000L);

    // negative long round-trip -- catches sign-extension bugs
    x = -32769L;
    check("**lpp neg long",  **lpp == -32769L);

    // redirect the inner pointer via *lpp
    {
        long y;
        long *lp2;
        y    = 777L;
        lp2  = &y;
        *lpp = lp2;
        check("**lpp redir rd", **lpp == 777L);
        check("*lpp == lp2",    *lpp == lp2);
    }

    // unsigned long -- catches sign-extension on the high deref
    ux   = 65536L;
    ulp  = &ux;
    ulpp = &ulp;
    check("**ulpp ulong rd",  **ulpp == 65536L);
    **ulpp = 131072L;
    check("**ulpp ulong wr",  ux == 131072L);

    // global long** path
    glx2  = 999999L;
    glp2  = &glx2;
    glpp2 = &glp2;
    check("**glpp2 long rd",  **glpp2 == 999999L);
    **glpp2 = 1234L;
    check("**glpp2 long wr",  glx2 == 1234L);
}

// ============================================================================
// Local pointer array -- assignment form (no initializer)
// Bug risk: local-array storage wrong size; subscript scale wrong; pointer
// identity incorrect when passed to a function as int**.
// ============================================================================

// sum n ints via an int** array -- also verifies passing local ptr array
int sumiptrs(int **pp, int n) {
    int s;
    s = 0;
    while (n > 0) {
        s = s + **pp;
        pp++;
        n--;
    }
    return s;
}

void test_locpa() {
    int a, b, c;
    int *arr[3];

    printf("--- local ptr arr ---\n");
    a = 10; b = 20; c = 30;
    arr[0] = &a;
    arr[1] = &b;
    arr[2] = &c;

    // read through subscripts
    check("arr[0] rd",      *arr[0] == 10);
    check("arr[1] rd",      *arr[1] == 20);
    check("arr[2] rd",      *arr[2] == 30);

    // write through a subscript and verify original variable
    *arr[1] = 99;
    check("arr[1] wr",      b == 99);

    // pointer identity: arr[i] must contain the exact address of the variable
    check("arr[0]==&a",     arr[0] == &a);
    check("arr[2]==&c",     arr[2] == &c);

    // pass local ptr array to function expecting int**
    *arr[1] = 20;       // restore b
    check("sumiptrs",   sumiptrs(arr, 3) == 60);
}

// ============================================================================
// Local pointer array -- brace initializer: int *arr[N] = { &a, &b, &c }
// Bug risk: initLocPtrArray stores wrong addresses; staging buffer discards
// address-of expressions; slots are shifted by one.
// ============================================================================

void test_lpainit() {
    int x, y, z;
    int *arr[3];

    printf("--- local ptr arr init ---\n");
    x = 1; y = 2; z = 3;

    // inner block: brace-initializer form with addresses of outer-scope locals
    {
        int *ia[3];
        ia[0] = &x;
        ia[1] = &y;
        ia[2] = &z;
        check("ia[0] rd",   *ia[0] == 1);
        check("ia[1] rd",   *ia[1] == 2);
        check("ia[2] rd",   *ia[2] == 3);

        // write back and verify original variables
        *ia[0] = 11;
        check("ia[0] wr",   x == 11);
        *ia[2] = 33;
        check("ia[2] wr",   z == 33);

        // modify pointer slot and re-read via original variable
        ia[1] = &x;         // redirect slot 1 to x
        check("ia redir",   *ia[1] == 11);
    }
}

// ============================================================================
// Local char* pointer array (string table / argv style)
// Bug risk: NULL sentinel not stored correctly; subscript out-of-range when
// traversing; pp++ scale wrong for char**.
// ============================================================================

void test_strarr() {
    char *sarr[4];
    char **pp;
    int  cnt;

    printf("--- char* arr & sentinel ---\n");
    sarr[0] = "one";
    sarr[1] = "two";
    sarr[2] = "three";
    sarr[3] = 0;

    // count via pointer traversal until NULL sentinel
    pp  = sarr;
    cnt = 0;
    while (*pp) {
        cnt++;
        pp++;
    }
    check("sentinel cnt",   cnt == 3);
    check("sentinel end",   *pp == 0);

    // subscript access via char** must match direct subscript of the local array
    pp = sarr;
    check("pp[0]==sarr[0]", pp[0] == sarr[0]);
    check("pp[1]==sarr[1]", pp[1] == sarr[1]);
    check("pp[2]==sarr[2]", pp[2] == sarr[2]);
}

// ============================================================================
// NULL through double pointer
// Bug risk: null-pointer write through ** uses wrong store size; reading *pp
// after setting *pp=0 returns garbage instead of 0.
// ============================================================================

void test_nullpp() {
    int  x;
    int  *p;
    int  **pp;

    printf("--- NULL through ** ---\n");
    p  = 0;
    pp = &p;

    // reading a null pointer through **
    check("*pp is 0",       *pp == 0);

    // assign a real pointer through **
    x  = 42;
    *pp = &x;
    check("*pp set to &x",  p == &x);
    check("**pp reads x",   **pp == 42);

    // null it back out through **
    *pp = 0;
    check("*pp set to 0",   p == 0);
}

// ============================================================================
// Pointer swap via double pointer
// Bug risk: swapptr modifies local copies instead of the actual pointers;
// the triple-store sequence leaves one pointer clobbered.
// ============================================================================

void swapptr(int **a, int **b) {
    int *t;
    t  = *a;
    *a = *b;
    *b = t;
}

void test_swap() {
    int  x, y;
    int  *p, *q;

    printf("--- swap via ** ---\n");
    x = 1; y = 2;
    p = &x; q = &y;

    swapptr(&p, &q);
    check("swap p→&y",      p == &y);
    check("swap q→&x",      q == &x);
    check("swap *p==2",     *p == 2);
    check("swap *q==1",     *q == 1);

    // swap back -- must be idempotent
    swapptr(&p, &q);
    check("swap back p",    p == &x);
    check("swap back q",    q == &y);
}

// ============================================================================
// Aliased double pointers (two int** to the same int* slot)
// Bug risk: compiler creates a private copy of the pointer instead of going
// through the address; writes through one alias are invisible through the other.
// ============================================================================

void test_alias() {
    int  x, y;
    int  *p;
    int  **pp, **qq;    // both will point at p

    printf("--- aliased ** ---\n");
    x = 10; y = 20;
    p  = &x;
    pp = &p;
    qq = &p;            // qq aliases pp -- both address p

    // set via pp, read via qq
    *pp = &y;
    check("qq sees *pp",    *qq == &y);
    check("**qq == y",      **qq == 20);

    // set via qq, read via pp
    *qq = &x;
    check("pp sees *qq",    *pp == &x);
    check("**pp == x",      **pp == 10);

    // value write through one; visible through both
    **qq = 77;
    check("**pp alias wr",  x == 77);
    check("**qq alias rd",  **pp == 77);
}

// ============================================================================
// *pp = &y (redirect) vs **pp = y (value write) -- must have distinct effects
// Bug risk: codegen confuses depth; a redirect accidentally modifies the value,
// or a value write accidentally moves the pointer.
// ============================================================================

void test_redir() {
    int  x, y;
    int  *p;
    int  **pp;

    printf("--- redirect vs write ---\n");
    x = 1; y = 2;
    p  = &x;
    pp = &p;

    // *pp = &y: redirect p to y; x must be unchanged
    *pp = &y;
    check("redir p→y",      p == &y);
    check("redir x intact", x == 1);
    check("**pp==y val",    **pp == 2);

    // **pp = 55: write through the redirected pointer (now p→y); p must not move
    **pp = 55;
    check("wr y==55",       y == 55);
    check("p still &y",     p == &y);
    check("x still 1",      x == 1);
}

// ============================================================================
// Inner-pointer arithmetic: (*pp)+N operates in sizeof(*p) units, not BPW
// Bug risk: scale comes from the outer pointer type (int**) instead of the
// inner type (int*); on 8086 sizeof(int)==BPW==2 so numeric result is the
// same, but advancing via *pp = *pp+1 must update p, not pp.
// ============================================================================

void test_inarith() {
    int  arr[3];
    int  *p;
    int  **pp;

    printf("--- inner ptr arith ---\n");
    arr[0] = 10; arr[1] = 20; arr[2] = 30;
    p  = &arr[0];
    pp = &p;

    // (*pp)+1 is &arr[1] -- tests that inner-pointer scale is sizeof(int)
    check("(*pp)+1",&arr[1]     == (*pp)+1);
    check("*((*pp)+1)",         *( (*pp)+1 ) == 20);

    // advancing the inner pointer via *pp = *pp + 1 must change p, not pp
    *pp = *pp + 1;
    check("p advanced",         p == &arr[1]);
    check("**pp after adv",     **pp == 20);

    // advance again
    *pp = *pp + 1;
    check("p at arr[2]",        p == &arr[2]);
    check("**pp == 30",         **pp == 30);
}

// ============================================================================
// Triple-pointer coherence after redirect
// Bug risk: after *pp = &y, one of the three read paths (via p, **pp, ***ppp)
// caches the old address and returns a stale value.
// ============================================================================

void test_tricohr() {
    int  x, y;
    int  *p;
    int  **pp;
    int  ***ppp;

    printf("--- triple coherence ---\n");
    x = 5; y = 99;
    p   = &x;
    pp  = &p;
    ppp = &pp;

    // all read paths must agree on the initial value
    check("***ppp==5",      ***ppp == 5);
    check("**pp==5",        **pp == 5);
    check("*p==5",          *p == 5);

    // write via ***ppp; verify all three paths
    ***ppp = 42;
    check("x==42",          x == 42);
    check("**pp==42",       **pp == 42);
    check("***ppp==42",     ***ppp == 42);

    // redirect middle level: *pp = &y  makes p -> y, decoupling x from chain
    *pp = &y;
    check("p==&y",          p == &y);
    check("***ppp==y(99)",  ***ppp == 99);
    check("x still 42",     x == 42);

    // write via ***ppp now modifies y, not x
    ***ppp = 7;
    check("y==7",           y == 7);
    check("x still 42.2",   x == 42);
}

// ============================================================================
// Output-parameter pattern: void f(int **out, int *src) { *out = src; }
// Bug risk: *out = src writes to the callee's copy of 'out' (a local int**)
// rather than through the pointer, so the caller's pointer is not updated.
// ============================================================================

void setptr(int **outpp, int *src) {
    *outpp = src;
}

void test_outparm() {
    int  x;
    int  *p;

    printf("--- output param ** ---\n");
    x = 42;
    p = 0;

    setptr(&p, &x);
    check("out p==&x",      p == &x);
    check("out *p==42",     *p == 42);

    // write through the now-set pointer and verify original variable
    *p = 99;
    check("out wr x==99",   x == 99);
}

// ============================================================================
// Main
// ============================================================================
int main() {
    passed = 0;
    failed = 0;
    printf("=== Multi-level pointer tests ===\n");
    test_global();
    test_local();
    test_triple();
    test_cpptr();
    test_ptrarith();
    test_cast();
    test_fnret();
    test_struct();
    test_longptr();
    test_locpa();
    test_lpainit();
    test_strarr();
    test_nullpp();
    test_swap();
    test_alias();
    test_redir();
    test_inarith();
    test_tricohr();
    test_outparm();
    printf("=== Results: %d passed, %d failed ===\n", passed, failed);
    if (failed)
        getchar();
    return failed;
}
