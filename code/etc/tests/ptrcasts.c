// ptrcasts.c -- Test (type *) pointer cast expressions in Small-C.
//
// Purpose:
//   Verify that explicit pointer casts to typed pointer types produce
//   correct code for dereferencing, subscripting, arithmetic scaling,
//   signed vs. unsigned byte fetches, and write-through operations.
//
// Functionality covered:
//   - Unary dereference through a cast pointer: *(char *)&intvar reads low byte
//   - Reverse cast: *(int *)charbuf reads a 16-bit word from a byte buffer
//   - Signed byte fetch: (char *)ptr dereference sign-extends (GETb1p)
//   - Unsigned byte fetch: (unsigned char *)ptr dereference zero-extends (GETb1pu)
//   - Subscript element scaling: ((char *)intarr)[n] scales index by 1
//   - Subscript element scaling: ((int *)charbuf)[n] scales index by 2
//   - Write-through cast: assigning via a (char *) cast to an int variable
//   - void * cast compatibility with other typed casts
//   - All identifiers are at most 12 characters (Small-C identifier limit)

#include "../../smallc22/stdio.h"

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
// Phase 1: basic dereference through cast pointer
// ============================================================================
void test_deref() {
    int  ivar;
    char cbuf[4];
    int  *ip;
    char *cp;

    printf("--- deref ---\n");

    // *(char *) on an int: read the low byte
    ivar = 0x1234;
    cp = (char *)&ivar;
    check("*(char*)int lo", *cp == 0x34);

    // *(int *) on a char buffer loaded with known bytes
    cbuf[0] = 0x78;
    cbuf[1] = 0x56;
    ip = (int *)cbuf;
    check("*(int*)cbuf",    *ip == 0x5678);
}

// ============================================================================
// Phase 2: signed vs unsigned byte fetch
// Cast to unsigned char * must use GETb1pu (zero-extend),
// cast to signed char * must use GETb1p  (sign-extend).
// ============================================================================
void test_bsign() {
    int  ivar;
    unsigned char *ucp;
    char          *scp;

    printf("--- byte sign ---\n");

    ivar = -1;          // bit pattern 0xFFFF

    ucp = (unsigned char *)&ivar;
    check("uchr* == 255", *ucp == 255);

    scp = (char *)&ivar;
    check("chr*  == -1",  *scp == -1);
}

// ============================================================================
// Phase 3: subscript element-scale through cast
// ((char *)intarr)[n] must scale index by 1, not 2.
// ((int  *)cbuf  )[n] must scale index by 2, not 1.
// ============================================================================
void test_subscr() {
    int  iarr[3];
    char cbuf[6];
    char *cp;
    int  *ip;

    printf("--- subscript ---\n");

    // Store known words; access as bytes
    iarr[0] = 0x0201;
    iarr[1] = 0x0403;
    cp = (char *)iarr;
    check("chrcast[0]==1",  cp[0] == 1);
    check("chrcast[1]==2",  cp[1] == 2);
    check("chrcast[2]==3",  cp[2] == 3);
    check("chrcast[3]==4",  cp[3] == 4);

    // Store known bytes; access as words (little-endian)
    cbuf[0] = 0x78;
    cbuf[1] = 0x56;
    cbuf[2] = 0x34;
    cbuf[3] = 0x12;
    ip = (int *)cbuf;
    check("intcast[0]==5678", ip[0] == 0x5678);
    check("intcast[1]==1234", ip[1] == 0x1234);
}

// ============================================================================
// Phase 4: pointer arithmetic scale after cast
// (char *)p + n  must advance by n bytes
// (int  *)p + n  must advance by 2*n bytes
// ============================================================================
void test_ptrmath() {
    int  iarr[4];
    char buf[8];
    char *cp;
    int  *ip;
    int  i;

    printf("--- ptr arith ---\n");

    for (i = 0; i < 4; i++) iarr[i] = i + 1;
    cp = (char *)iarr;
    check("char+1 step=1",  (cp + 1) - cp == 1);
    check("char+2 step=2",  (cp + 2) - cp == 2);

    for (i = 0; i < 8; i++) buf[i] = i;
    ip = (int *)buf;
    check("int+1 step=2",   (char *)(ip + 1) - (char *)ip == 2);
    check("int+2 step=4",   (char *)(ip + 2) - (char *)ip == 4);
}

// ============================================================================
// Phase 5: write through cast pointer
// *(int *)charbuf = value  must store a word
// ============================================================================
void test_write() {
    char cbuf[4];
    int  *ip;
    int  ivar;
    char *cp;

    printf("--- write-thru ---\n");

    cbuf[0] = 0;
    cbuf[1] = 0;
    ip = (int *)cbuf;
    *ip = 0x1234;
    check("lo byte==0x34",  cbuf[0] == 0x34);
    check("hi byte==0x12",  cbuf[1] == 0x12);

    // Writing a char through (char *) into an int var
    ivar = 0xFFFF;
    cp = (char *)&ivar;
    *cp = 0x00;
    check("chr write lo",   (ivar & 0x00FF) == 0);
    check("chr hi intact",  (ivar & 0xFF00) == 0xFF00);
}

// ============================================================================
// Phase 6: cast from void *
// (int *)voidptr  allows dereference and subscript
// ============================================================================
void test_voidcast() {
    int  iarr[3];
    void *vp;
    int  *ip;
    char *cp;

    printf("--- void cast ---\n");

    iarr[0] = 100;
    iarr[1] = 200;
    vp = iarr;

    ip = (int *)vp;
    check("(int*)vp[0]",  ip[0] == 100);
    check("(int*)vp[1]",  ip[1] == 200);

    cp = (char *)vp;
    check("(chr*)vp!=0",  cp != 0);
    // First byte of iarr[0] == 100 in a word is 100 (0x0064): low byte == 100
    check("(chr*)vp[0]",  cp[0] == 100);
}

// ============================================================================
// Phase 7: (long *) and (unsigned long *) cast mechanics
// Dereference, subscript element-scale, pointer arithmetic, write-through.
// ============================================================================
void test_lngcast() {
    long          larr[2];
    long          lv;
    unsigned long ulv;
    long          *lp;
    unsigned long *ulp;
    int           *ip;
    int            b, a;

    printf("--- long* cast ---\n");

    // *(long *): 32-bit load -- fill via int*, read back via long*
    ip    = larr;
    ip[0] = 1234;
    ip[1] = 5;
    lp    = (long *)larr;
    lv    = *lp;
    ip    = &lv;
    check("*(lng*)lo=1234", *ip     == 1234);
    check("*(lng*)hi=5",    *(ip+1) == 5);

    // (long *) subscript: index scales by 4
    ip    = larr;
    ip[2] = 999;
    ip[3] = 0;
    lp    = (long *)larr;
    lv    = lp[1];
    ip    = &lv;
    check("lng*[1] lo=999", *ip     == 999);
    check("lng*[1] hi=0",   *(ip+1) == 0);

    // pointer arithmetic: (long *)p + 1 advances 4 bytes
    lp = larr;
    b  = lp;
    a  = lp + 1;
    check("lng*+1 step=4",  (a - b) == 4);

    // write-through: *(long *) = 32-bit value
    lv  = 70000;
    lp  = (long *)larr;
    *lp = lv;
    ip  = larr;
    check("*(lng*)wr lo",   *ip     == 4464);
    check("*(lng*)wr hi",   *(ip+1) == 1);

    // *(unsigned long *): 32-bit load
    ip    = larr;
    ip[0] = 4464;
    ip[1] = 1;
    ulp   = (unsigned long *)larr;
    ulv   = *ulp;
    ip    = &ulv;
    check("*(ulng*)lo=4464", *ip     == 4464);
    check("*(ulng*)hi=1",    *(ip+1) == 1);

    // (unsigned long *) pointer arithmetic: step = 4
    ulp = (unsigned long *)larr;
    b   = ulp;
    a   = ulp + 1;
    check("ulng*+1 step=4", (a - b) == 4);
}

// ============================================================================
// Phase 8: pointer step size for all typed pointers
// Prefix and postfix ++ and -- must advance/retreat by sizeof(*type).
// char*/uchar*: 1,  int*/uint*: 2,  long*/ulong*: 4
// ============================================================================
void test_ptrstep() {
    char          ca[2];
    unsigned char uca[2];
    int           ia[2];
    unsigned      uia[2];
    long          la[2];
    unsigned long ula[2];
    char          *cp;
    unsigned char *ucp;
    int           *ip;
    unsigned      *uip;
    long          *lp;
    unsigned long *ulp;
    int            b, a;

    printf("--- ptr step ---\n");

    // char*: step = 1
    cp = ca;   b = cp; ++cp; a = cp;
    check("preinc chr*=1",   (a-b) == 1);
    cp = ca;   b = cp; cp++; a = cp;
    check("postinc chr*=1",  (a-b) == 1);
    cp = ca+1; b = cp; --cp; a = cp;
    check("predec chr*=1",   (b-a) == 1);
    cp = ca+1; b = cp; cp--; a = cp;
    check("postdec chr*=1",  (b-a) == 1);

    // unsigned char*: step = 1
    ucp = uca;   b = ucp; ++ucp; a = ucp;
    check("preinc uchr*=1",  (a-b) == 1);
    ucp = uca;   b = ucp; ucp++; a = ucp;
    check("postinc uchr*=1", (a-b) == 1);
    ucp = uca+1; b = ucp; --ucp; a = ucp;
    check("predec uchr*=1",  (b-a) == 1);
    ucp = uca+1; b = ucp; ucp--; a = ucp;
    check("postdec uchr*=1", (b-a) == 1);

    // int*: step = 2
    ip = ia;   b = ip; ++ip; a = ip;
    check("preinc int*=2",   (a-b) == 2);
    ip = ia;   b = ip; ip++; a = ip;
    check("postinc int*=2",  (a-b) == 2);
    ip = ia+1; b = ip; --ip; a = ip;
    check("predec int*=2",   (b-a) == 2);
    ip = ia+1; b = ip; ip--; a = ip;
    check("postdec int*=2",  (b-a) == 2);

    // unsigned int*: step = 2
    uip = uia;   b = uip; ++uip; a = uip;
    check("preinc uint*=2",  (a-b) == 2);
    uip = uia;   b = uip; uip++; a = uip;
    check("postinc uint*=2", (a-b) == 2);
    uip = uia+1; b = uip; --uip; a = uip;
    check("predec uint*=2",  (b-a) == 2);
    uip = uia+1; b = uip; uip--; a = uip;
    check("postdec uint*=2", (b-a) == 2);

    // long*: step = 4
    lp = la;   b = lp; ++lp; a = lp;
    check("preinc lng*=4",   (a-b) == 4);
    lp = la;   b = lp; lp++; a = lp;
    check("postinc lng*=4",  (a-b) == 4);
    lp = la+1; b = lp; --lp; a = lp;
    check("predec lng*=4",   (b-a) == 4);
    lp = la+1; b = lp; lp--; a = lp;
    check("postdec lng*=4",  (b-a) == 4);

    // unsigned long*: step = 4
    ulp = ula;   b = ulp; ++ulp; a = ulp;
    check("preinc ulng*=4",  (a-b) == 4);
    ulp = ula;   b = ulp; ulp++; a = ulp;
    check("postinc ulng*=4", (a-b) == 4);
    ulp = ula+1; b = ulp; --ulp; a = ulp;
    check("predec ulng*=4",  (b-a) == 4);
    ulp = ula+1; b = ulp; ulp--; a = ulp;
    check("postdec ulng*=4", (b-a) == 4);
}

// ============================================================================
// Phase 9: int* - int* element-count subtraction
// Subtracting two typed int* pointers must yield the element count, not bytes.
// down2 emits SWAP12 + GETw1n(1) + ASR12 to divide the raw byte difference by 2.
// ============================================================================
void test_ptrdiff_int() {
    int arr[4];
    int *p1, *p2;
    int diff;

    printf("--- ptrdiff int* ---\n");

    p1 = arr;
    p2 = arr + 3;
    diff = p2 - p1;
    check("int* p2-p1==3",  diff == 3);

    p2 = arr + 1;
    diff = p2 - p1;
    check("int* p2-p1==1",  diff == 1);

    diff = p1 - p2;
    check("int* p1-p2==-1", diff == -1);
}

int main() {
    passed = 0;
    failed = 0;

    test_deref();
    test_bsign();
    test_subscr();
    test_ptrmath();
    test_write();
    test_voidcast();
    test_lngcast();
    test_ptrstep();
    test_ptrdiff_int();

    printf("\n%d passed, %d failed\n", passed, failed);
    if (failed) getchar();
    return failed;
}
