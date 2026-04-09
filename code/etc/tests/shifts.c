// shifts.c -- Test unsigned vs signed right shift correctness.
// Verifies that >> on unsigned types emits SHR (logical, zero-fill)
// and >> on signed types emits SAR (arithmetic, sign-fill).

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

void checkv(char *desc, int got, int expected) {
    if (got == expected) {
        printf("  PASS: %s\n", desc);
        passed++;
    }
    else {
        printf("  FAIL: %s  got=%d expected=%d\n", desc, got, expected);
        failed++;
        getchar();
    }
}

// ============================================================================
// 16-bit signed right shift -- must produce negative result (sign-fill)
// ============================================================================

void test_s16() {
    int n;
    n = -32768;         // 0x8000
    checkv("s16 >>1",  n >> 1,  -16384);   // SAR: 0xC000
    checkv("s16 >>4",  n >> 4,  -2048);    // SAR: 0xF800
    checkv("s16 >>15", n >> 15, -1);       // SAR: 0xFFFF
    n = -1;             // 0xFFFF
    checkv("s16 -1>>8", n >> 8, -1);       // SAR: all ones
}

// ============================================================================
// 16-bit unsigned right shift -- must zero-fill (positive result)
// ============================================================================

void test_u16() {
    unsigned int u;
    u = 0x8000;         // 32768
    checkv("u16 >>1",  u >> 1,  0x4000);   // SHR: zero-fill
    checkv("u16 >>4",  u >> 4,  0x0800);
    checkv("u16 >>15", u >> 15, 1);
    u = 0xFFFF;
    checkv("u16 >>8",  u >> 8,  0x00FF);   // SHR: not 0xFFFF
    check("u16 >>8+", (u >> 8) != -1);     // must not be sign-filled
}

// ============================================================================
// 16-bit unsigned >>= compound assignment
// ============================================================================

void test_u16_asgn() {
    unsigned int u;
    u = 0x8000;
    u >>= 1;
    check("u16>>=1",  u == 0x4000);
    u = 0xFFFF;
    u >>= 4;
    check("u16>>=4",  u == 0x0FFF);
    u = 0x8000;
    u >>= 15;
    check("u16>>=15", u == 1);
}

// ============================================================================
// 16-bit constant folding: unsigned >>
// ============================================================================

void test_cnst16() {
    unsigned int u;
    u = 0x8000 >> 1;    // constant fold must use logical shift
    check("cnst>>1",  u == 0x4000);
    u = 0xFFFF >> 8;
    check("cnst>>8",  u == 0x00FF);
}

// ============================================================================
// Summary
// ============================================================================

void main() {
    passed = 0;
    failed = 0;
    printf("=== Shift Tests ===\n");
    test_s16();
    test_u16();
    test_u16_asgn();
    test_cnst16();
    printf("--- %d passed, %d failed ---\n", passed, failed);
}
