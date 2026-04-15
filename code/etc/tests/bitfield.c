// bitfields.c -- Comprehensive test program for struct/union bit-field support
//                in Small-C 2.2.
//
// Purpose:
//   Verify that the compiler correctly handles bit-field declarations,
//   packed layout, signed and unsigned readback, neighbor preservation,
//   unnamed padding fields, zero-width force-alignment, compound
//   assignment operators, pre/post increment/decrement, union bit-fields,
//   sizeof correctness, and pointer-to-struct bit-field access.
//
// Functionality covered:
//   - Basic packing (unsigned, two fields in one word)
//   - Signed readback (int bit-field with negative value)
//   - Neighbor preservation (write A, write B, rewrite A, check B unchanged)
//   - Unnamed padding bit-field (gap in same allocation unit)
//   - Zero-width unnamed bit-field (force new allocation unit)
//   - Compound assignment: s.a += n
//   - Pre-increment: ++s.a
//   - Post-increment: s.a++ (expression value == pre-mod value)
//   - Pre-decrement: --s.a
//   - Post-decrement: s.a-- (expression value == pre-mod value)
//   - Union bit-fields (all fields at offset 0)
//   - sizeof struct with only bit-fields == sizeof one allocation unit
//   - sizeof struct with mixed normal and bit-field members
//   - Access via pointer to struct (->)
//   - char-base bit-field (stored in 16-bit unit per spec)
//   - Full-width bit-field (16 bits, occupies entire allocation unit)

#include <stdio.h>

int passed, failed;

// ============================================================================
// Struct/union definitions for tests
// ============================================================================

// Basic packing: two unsigned fields in one word.
struct BasicPack {
    unsigned a : 3;
    unsigned b : 5;
};

// Signed readback.
struct Signed {
    int x : 3;
};

// Neighbor preservation.
struct Preserve {
    unsigned a : 3;
    unsigned b : 5;
};

// Unnamed padding: a 2-bit gap between a and b.
struct Padded {
    unsigned a : 3;
    unsigned   : 2;
    unsigned b : 5;
};

// Zero-width unnamed: force-align b to next allocation unit.
struct ZeroAlign {
    unsigned a : 3;
    unsigned   : 0;
    unsigned b : 5;
};

// Compound assignment / increment / decrement targets.
struct Arith {
    int      val : 8;
    unsigned cnt : 4;
};

// Union bit-fields (all at offset 0, independent views of same storage).
union UBF {
    unsigned all : 8;
    unsigned lo  : 4;
};

// Mixed: a normal int member followed by a bit-field block.
struct Mixed {
    int      regular;
    unsigned bf1 : 3;
    unsigned bf2 : 5;
};

// char-base bit-field (stored in one 16-bit allocation unit per spec).
struct CharBF {
    unsigned char lo : 4;
    unsigned char hi : 4;
};

// Full-width bit-field.
struct FullWord {
    int whole : 16;
};

// For pointer-to-struct access tests.
struct Spread {
    unsigned low  : 4;
    unsigned high : 4;
};

// ============================================================================
// Global instances
// ============================================================================

struct BasicPack   gbp;
struct Signed      gsgn;
struct Preserve    gpres;
struct Padded      gpad;
struct ZeroAlign   gza;
struct Arith       garith;
union  UBF         gubf;
struct Mixed       gmix;
struct CharBF      gcbf;
struct FullWord    gfw;
struct Spread      gspr;

// ============================================================================
// Helper functions
// ============================================================================

void check(char *desc, int cond) {
    if (cond) {
        printf("  PASS: %s\n", desc);
        passed++;
    }
    else {
        printf("  FAIL: %s\n", desc);
        printf("  Press Enter to continue...");
        getchar();
        failed++;
    }
}

void checkVal(char *desc, int exp, int got) {
    if (exp == got) {
        printf("  PASS: %s\n", desc);
        passed++;
    }
    else {
        printf("  FAIL: %s (exp %d, got %d)\n", desc, exp, got);
        printf("  Press Enter to continue...");
        getchar();
        failed++;
    }
}

// ============================================================================
// Test: basic packing (spec §15 test 1)
// ============================================================================
void testBasicPack() {
    struct BasicPack s;
    printf("--- Basic packing ---\n");
    s.a = 5;
    s.b = 17;
    checkVal("s.a = 5", 5, s.a);
    checkVal("s.b = 17", 17, s.b);
}

// ============================================================================
// Test: signed readback (spec §15 test 2)
// ============================================================================
void testSigned() {
    struct Signed s;
    printf("--- Signed readback ---\n");
    s.x = -1;
    checkVal("s.x = -1", -1, s.x);
    s.x = 3;   /* 3-bit signed max positive = 3 */
    checkVal("s.x = 3 (max pos)", 3, s.x);
    s.x = -4;  /* 3-bit signed min = -4 */
    checkVal("s.x = -4 (min)", -4, s.x);
}

// ============================================================================
// Test: neighbor preservation (spec §15 test 3)
// ============================================================================
void testPreserve() {
    struct Preserve s;
    printf("--- Neighbor preservation ---\n");
    s.a = 7;
    s.b = 3;
    s.a = 1;
    checkVal("a=1 after overwrite", 1, s.a);
    checkVal("b=3 still unchanged", 3, s.b);
    /* Also check a-overwrite doesn't bleed into b */
    s.b = 31;  /* max 5-bit unsigned */
    s.a = 0;
    checkVal("b=31 unchanged after a=0", 31, s.b);
    checkVal("a=0", 0, s.a);
}

// ============================================================================
// Test: unnamed padding (spec §15 test 4)
// ============================================================================
void testPadded() {
    struct Padded s;
    printf("--- Unnamed padding ---\n");
    s.a = 7;
    s.b = 15;
    checkVal("padded s.a = 7", 7, s.a);
    checkVal("padded s.b = 15", 15, s.b);
    /* Rewrite a; b must be unchanged */
    s.a = 2;
    checkVal("padded a=2", 2, s.a);
    checkVal("padded b=15 unchanged", 15, s.b);
}

// ============================================================================
// Test: zero-width force alignment (spec §15 test 5)
// ============================================================================
void testZeroAlign() {
    struct ZeroAlign s;
    printf("--- Zero-width force alignment ---\n");
    s.a = 7;
    s.b = 31;
    checkVal("za a=7", 7, s.a);
    checkVal("za b=31", 31, s.b);
    /* a and b must be in separate allocation units: writing b should not affect a */
    s.b = 1;
    checkVal("za a=7 after b write", 7, s.a);
    checkVal("za b=1", 1, s.b);
}

// ============================================================================
// Test: sizeof (spec §15 + §16)
// ============================================================================
void testSizeof() {
    printf("--- sizeof ---\n");
    /* A struct with only bit-fields packed into one word: sizeof == 2 */
    checkVal("sizeof BasicPack == 2", 2, sizeof(struct BasicPack));
    checkVal("sizeof Signed == 2",    2, sizeof(struct Signed));
    /* ZeroAlign has two separate allocation units: sizeof == 4 */
    checkVal("sizeof ZeroAlign == 4", 4, sizeof(struct ZeroAlign));
    /* Mixed: one int (2 bytes) + one allocation unit for bit-fields (2 bytes) = 4 */
    checkVal("sizeof Mixed == 4",     4, sizeof(struct Mixed));
    /* Union bit-fields: size == one allocation unit (BPW = 2) */
    checkVal("sizeof UBF == 2",       2, sizeof(union UBF));
}

// ============================================================================
// Test: compound assignment
// ============================================================================
void testCompound() {
    struct Arith s;
    printf("--- Compound assignment ---\n");
    s.val = 10;
    s.cnt = 3;
    s.val += 5;
    checkVal("val += 5 -> 15", 15, s.val);
    checkVal("cnt unchanged after val +=", 3, s.cnt);
    s.cnt += 2;
    checkVal("cnt += 2 -> 5", 5, s.cnt);
    checkVal("val unchanged after cnt +=", 15, s.val);
    s.val -= 3;
    checkVal("val -= 3 -> 12", 12, s.val);
    s.cnt -= 1;
    checkVal("cnt -= 1 -> 4", 4, s.cnt);
    s.val = 4;
    s.val *= 3;
    checkVal("val *= 3 -> 12", 12, s.val);
}

// ============================================================================
// Test: pre-increment / pre-decrement
// ============================================================================
void testPreInc() {
    struct Arith s;
    printf("--- Pre-increment/decrement ---\n");
    s.val = 5;
    s.cnt = 2;
    ++s.val;
    checkVal("++val -> 6", 6, s.val);
    checkVal("cnt unchanged after ++val", 2, s.cnt);
    --s.val;
    checkVal("--val -> 5", 5, s.val);
    ++s.cnt;
    checkVal("++cnt -> 3", 3, s.cnt);
}

// ============================================================================
// Test: post-increment / post-decrement (expression value)
// ============================================================================
void testPostInc() {
    struct Arith s;
    int old;
    printf("--- Post-increment/decrement ---\n");
    s.val = 7;
    s.cnt = 1;
    old = s.val++;
    checkVal("val++ returns pre-mod (7)", 7, old);
    checkVal("val after ++ == 8", 8, s.val);
    checkVal("cnt unchanged after val++", 1, s.cnt);
    old = s.val--;
    checkVal("val-- returns pre-mod (8)", 8, old);
    checkVal("val after -- == 7", 7, s.val);
    old = s.cnt++;
    checkVal("cnt++ returns pre-mod (1)", 1, old);
    checkVal("cnt after ++ == 2", 2, s.cnt);
}

// ============================================================================
// Test: union bit-fields
// ============================================================================
void testUnion() {
    union UBF u;
    printf("--- Union bit-fields ---\n");
    u.all = 0xAB & 0xFF;  /* 8-bit unsigned: 0xAB = 171 */
    /* lo overlaps the same storage; it reads the low 4 bits of all */
    checkVal("union all write", 0xAB & 0xFF, u.all);
    /* lo should read the same 4 bits as all's low nibble */
    checkVal("union lo (low 4 bits)", 0xAB & 0x0F, u.lo);
    /* Write via lo; all should reflect the change in the low 4 bits */
    u.lo = 5;
    checkVal("union lo = 5", 5, u.lo);
}

// ============================================================================
// Test: mixed normal + bit-field struct
// ============================================================================
void testMixed() {
    struct Mixed m;
    printf("--- Mixed struct ---\n");
    m.regular = 1234;
    m.bf1 = 6;
    m.bf2 = 20;
    checkVal("mixed regular = 1234", 1234, m.regular);
    checkVal("mixed bf1 = 6", 6, m.bf1);
    checkVal("mixed bf2 = 20", 20, m.bf2);
    /* Overwrite regular; bit-fields must be unaffected */
    m.regular = 9999;
    checkVal("bf1 after regular write", 6, m.bf1);
    checkVal("bf2 after regular write", 20, m.bf2);
}

// ============================================================================
// Test: char-base bit-field
// ============================================================================
void testCharBF() {
    struct CharBF s;
    printf("--- char-base bit-field ---\n");
    s.lo = 9;
    s.hi = 6;
    checkVal("char bf lo = 9", 9, s.lo);
    checkVal("char bf hi = 6", 6, s.hi);
    s.lo = 0;
    checkVal("hi unchanged after lo=0", 6, s.hi);
}

// ============================================================================
// Test: full-word (16-bit) bit-field
// ============================================================================
void testFullWord() {
    struct FullWord s;
    printf("--- Full-word bit-field ---\n");
    s.whole = 1000;
    checkVal("full-word = 1000", 1000, s.whole);
    s.whole = -1;
    checkVal("full-word signed -1", -1, s.whole);
}

// ============================================================================
// Test: access via pointer to struct (->)
// ============================================================================
void testPtr() {
    struct Spread s;
    struct Spread *p;
    printf("--- Pointer access ---\n");
    p = &s;
    p->low  = 10;
    p->high = 5;
    checkVal("ptr low = 10",  10, p->low);
    checkVal("ptr high = 5",   5, p->high);
    /* Write via pointer; read directly */
    checkVal("s.low after ptr write",  10, s.low);
    checkVal("s.high after ptr write",  5, s.high);
    /* Neighbor check via pointer */
    p->low = 3;
    checkVal("high unchanged after ptr low write", 5, p->high);
    checkVal("low = 3",  3, p->low);
}

// ============================================================================
// Test: global struct bit-field write/read
// ============================================================================
void testGlobal() {
    printf("--- Global struct ---\n");
    gbp.a = 6;
    gbp.b = 25;
    checkVal("global a = 6",  6, gbp.a);
    checkVal("global b = 25", 25, gbp.b);
    gbp.a = 0;
    checkVal("global b unchanged after a=0", 25, gbp.b);
}

// ============================================================================
// Test: multiple write cycles (stress bit isolation)
// ============================================================================
void testStress() {
    struct BasicPack s;
    int i;
    printf("--- Stress: multiple write cycles ---\n");
    i = 0;
    while (i < 8) {
        s.a = i & 7;
        s.b = (i * 3) & 31;
        check("stress a readable", s.a == (i & 7));
        check("stress b readable", s.b == ((i * 3) & 31));
        i++;
    }
}

// ============================================================================
// main
// ============================================================================
int main() {
    passed = 0;
    failed = 0;

    printf("============================================================\n");
    printf("  Small-C Bit-Field Test Suite\n");
    printf("============================================================\n");

    testBasicPack();
    testSigned();
    testPreserve();
    testPadded();
    testZeroAlign();
    testSizeof();
    testCompound();
    testPreInc();
    testPostInc();
    testUnion();
    testMixed();
    testCharBF();
    testFullWord();
    testPtr();
    testGlobal();
    testStress();

    printf("============================================================\n");
    printf("  Results: %d passed, %d failed\n", passed, failed);
    printf("============================================================\n");
    if (failed)
        return 1;
    return 0;
}
