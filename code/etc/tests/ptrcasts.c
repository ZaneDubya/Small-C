// ptrcasts.c -- Test (type *) pointer cast expressions in Small-C.
//
// Tests: unary * through cast, subscript element scale, arithmetic scale,
// byte signed vs unsigned fetch, write-through cast, void* cast.
//
// All identifiers are <= 12 characters (Small-C limit).

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

int main() {
    passed = 0;
    failed = 0;

    test_deref();
    test_bsign();
    test_subscr();
    test_ptrmath();
    test_write();
    test_voidcast();

    printf("\n%d passed, %d failed\n", passed, failed);
    if (failed) getchar();
    return failed;
}
