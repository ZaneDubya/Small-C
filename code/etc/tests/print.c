/* print.c -- printf/sprintf smoke test for Small-C stdio library.
**
** Covers all supported format specifiers, flags, width/precision
** (literal and runtime *), length modifiers, and edge cases.
**
** Specifiers tested: b, c, d, i, n, o, p, s, u, x, X
** Flags tested:      -, +, space, #, 0
** Modifiers tested:  h, l
** Width/prec:        decimal literal, * (runtime argument)
*/

#include <stdio.h>

int passed, failed;
char buf[128];

void check(char *desc, char *got, char *expect) {
    char *g, *e;
    g = got; e = expect;
    while (*g && *e && *g == *e) { g++; e++; }
    if (*g == *e) {
        printf("  PASS: %s\n", desc);
        passed++;
    }
    else {
        printf("  FAIL: %s\n", desc);
        printf("    expected: [%s]\n", expect);
        printf("    got:      [%s]\n", got);
        failed++;
        getchar();
    }
}

void checkn(char *desc, int got, int expect) {
    if (got == expect) {
        printf("  PASS: %s\n", desc);
        passed++;
    }
    else {
        printf("  FAIL: %s\n", desc);
        printf("    expected: %d\n", expect);
        printf("    got:      %d\n", got);
        failed++;
        getchar();
    }
}

/* ========================== test group 1 ===========================
** Tests 1-21: plain string, %%, %d, %i, %u, %o, %x, %X
** =================================================================== */

void test_integers() {

    /* ===================== Plain string ===================== */

    /* 1 */
    sprintf(buf, "plain");
    check("plain string", buf, "plain");

    /* ===================== %%  ===================== */

    /* 2 */
    sprintf(buf, "100%%");
    check("%% literal", buf, "100%");

    /* ===================== %d ===================== */

    /* 3 */
    sprintf(buf, "%d", 42);
    check("%d positive", buf, "42");

    /* 4 */
    sprintf(buf, "%d", -7);
    check("%d negative", buf, "-7");

    /* 5 */
    sprintf(buf, "%d", 0);
    check("%d zero", buf, "0");

    /* 6: multiple args */
    sprintf(buf, "%d %d %d", 1, 2, 3);
    check("%d multiple args", buf, "1 2 3");

    /* 7: INT_MIN -- most negative 16-bit signed value */
    sprintf(buf, "%d", -32768);
    check("%d INT_MIN (-32768)", buf, "-32768");

    /* ===================== %i ===================== */

    /* 8 */
    sprintf(buf, "%i", 42);
    check("%i positive", buf, "42");

    /* 9 */
    sprintf(buf, "%i", -7);
    check("%i negative", buf, "-7");

    /* 10 */
    sprintf(buf, "%i", 0);
    check("%i zero", buf, "0");

    /* ===================== %u ===================== */

    /* 11 */
    sprintf(buf, "%u", 65535);
    check("%u max 16-bit (65535)", buf, "65535");

    /* 12: bit pattern 0x8000 treated as unsigned 32768 */
    sprintf(buf, "%u", 32768);
    check("%u (32768)", buf, "32768");

    /* ===================== %o ===================== */

    /* 13 */
    sprintf(buf, "%o", 8);
    check("%o (8)", buf, "10");

    /* 14 */
    sprintf(buf, "%o", 0);
    check("%o (0)", buf, "0");

    /* 15 */
    sprintf(buf, "%o", 255);
    check("%o (255)", buf, "377");

    /* ===================== %x and %X ===================== */

    /* 16: %x lowercase */
    sprintf(buf, "%x", 255);
    check("%x (255) lowercase", buf, "ff");

    /* 17: %x zero */
    sprintf(buf, "%x", 0);
    check("%x (0)", buf, "0");

    /* 18: %x mixed A-F digits */
    sprintf(buf, "%x", 0x1ABC);
    check("%x (0x1abc)", buf, "1abc");

    /* 19: %X uppercase */
    sprintf(buf, "%X", 255);
    check("%X (255) uppercase", buf, "FF");

    /* 20: %X zero */
    sprintf(buf, "%X", 0);
    check("%X (0)", buf, "0");

    /* 21: %X mixed digits */
    sprintf(buf, "%X", 0x1ABC);
    check("%X (0x1ABC)", buf, "1ABC");
}

/* ========================== test group 2 ===========================
** Tests 22-41: %c, %s, %b, %p, flags (+, space, #)
** =================================================================== */

void test_misc_convs() {

    /* ===================== %c ===================== */

    /* 22 */
    sprintf(buf, "%c", 65);
    check("%c ('A')", buf, "A");

    /* ===================== %s ===================== */

    /* 23 */
    sprintf(buf, "%s", "hello");
    check("%s", buf, "hello");

    /* 24: empty string */
    sprintf(buf, "%s", "");
    check("%s empty", buf, "");

    /* 25: mixed %d and %s */
    sprintf(buf, "i=%d s=%s", 42, "hello");
    check("mixed %d %s", buf, "i=42 s=hello");

    /* 26: adjacent literal around conversion */
    sprintf(buf, "a%db", 9);
    check("literal around %d", buf, "a9b");

    /* ===================== %b (binary, non-standard) ===================== */

    /* 27 */
    sprintf(buf, "%b", 5);
    check("%b (5)", buf, "101");

    /* 28 */
    sprintf(buf, "%b", 0);
    check("%b (0)", buf, "0");

    /* ===================== %p ===================== */

    /* 29: pointer of 0 */
    sprintf(buf, "%p", 0);
    check("%p (0)", buf, "0");

    /* 30: pointer of 255 -- implementation emits uppercase hex */
    sprintf(buf, "%p", 255);
    check("%p (255)", buf, "FF");

    /* ===================== + flag ===================== */

    /* 31 */
    sprintf(buf, "%+d", 42);
    check("%+d positive", buf, "+42");

    /* 32 */
    sprintf(buf, "%+d", -7);
    check("%+d negative", buf, "-7");

    /* 33 */
    sprintf(buf, "%+d", 0);
    check("%+d zero", buf, "+0");

    /* ===================== space flag ===================== */

    /* 34 */
    sprintf(buf, "% d", 42);
    check("% d positive", buf, " 42");

    /* 35: sign beats space for negative */
    sprintf(buf, "% d", -7);
    check("% d negative", buf, "-7");

    /* 36: + flag beats space flag */
    sprintf(buf, "%+ d", 42);
    check("%+ d (+ beats space)", buf, "+42");

    /* ===================== # flag ===================== */

    /* 37: %#x non-zero */
    sprintf(buf, "%#x", 255);
    check("%#x (255)", buf, "0xff");

    /* 38: %#x zero (no prefix on zero) */
    sprintf(buf, "%#x", 0);
    check("%#x (0)", buf, "0");

    /* 39: %#X non-zero */
    sprintf(buf, "%#X", 255);
    check("%#X (255)", buf, "0XFF");

    /* 40: %#o non-zero gets leading 0 */
    sprintf(buf, "%#o", 8);
    check("%#o (8)", buf, "010");

    /* 41: %#o of 0 (already has leading zero, no extra) */
    sprintf(buf, "%#o", 0);
    check("%#o (0)", buf, "0");
}

/* ========================== test group 3 ===========================
** Tests 42-57: field width, runtime *, precision, - and 0 flags
** =================================================================== */

void test_width_prec() {

    /* ===================== Field width ===================== */

    /* 42: right-justify positive */
    sprintf(buf, "%5d", 42);
    check("%5d positive", buf, "   42");

    /* 43: right-justify negative */
    sprintf(buf, "%5d", -7);
    check("%5d negative", buf, "   -7");

    /* 44: left-justify */
    sprintf(buf, "%-5d", 42);
    check("%-5d", buf, "42   ");

    /* 45: zero-pad positive */
    sprintf(buf, "%05d", 42);
    check("%05d positive", buf, "00042");

    /* 46: zero-pad negative (sign before zeros, not after) */
    sprintf(buf, "%06d", -42);
    check("%06d negative", buf, "-00042");

    /* ===================== * runtime width ===================== */

    /* 47: positive * width */
    sprintf(buf, "%*d", 5, 42);
    check("%*d (w=5, v=42)", buf, "   42");

    /* 48: negative * width implies left-justify */
    sprintf(buf, "%*d", -5, 42);
    check("%*d (w=-5, v=42)", buf, "42   ");

    /* ===================== Precision for %s ===================== */

    /* 49: truncate string */
    sprintf(buf, "%.3s", "hello");
    check("%.3s", buf, "hel");

    /* 50: width + precision */
    sprintf(buf, "%7.3s", "hello");
    check("%7.3s", buf, "    hel");

    /* 51: * precision for %s */
    sprintf(buf, "%.*s", 3, "hello");
    check("%.*s (p=3)", buf, "hel");

    /* ===================== Precision for integers (minimum digits) ===================== */

    /* 52: %.5d pads to 5 digits */
    sprintf(buf, "%.5d", 12);
    check("%.5d (12)", buf, "00012");

    /* 53: %.5d negative */
    sprintf(buf, "%.5d", -12);
    check("%.5d (-12)", buf, "-00012");

    /* 54: %.3x minimum hex digits */
    sprintf(buf, "%.3x", 0xA);
    check("%.3x (0xa)", buf, "00a");

    /* 55: precision suppresses 0 field-fill flag */
    sprintf(buf, "%06.3d", 42);
    check("%06.3d (42)", buf, "   042");

    /* 56: * precision for integer */
    sprintf(buf, "%.*d", 5, 12);
    check("%.*d (p=5, v=12)", buf, "00012");

    /* ===================== - and 0 interaction ===================== */

    /* 57: left-justify overrides zero-pad */
    sprintf(buf, "%-05d", 42);
    check("%-05d (- beats 0)", buf, "42   ");
}

/* ========================== test group 4 ===========================
** Tests 58-73: %n, long conversions, h modifier
** =================================================================== */

void test_n_and_long() {
    int n;
    long lv, ln;
    int *lnp;

    /* ===================== %n ===================== */

    /* 58: %n stores character count */
    n = 0;
    sprintf(buf, "abc%n", &n);
    checkn("%n (3 chars before)", n, 3);

    /* 59: %n mid-string */
    n = 0;
    sprintf(buf, "hello %nworld", &n);
    checkn("%n mid-string (6)", n, 6);

    /* 60: %hn same as %n on this target (int == short) */
    n = 0;
    sprintf(buf, "hi%hn", &n);
    checkn("%hn (2 chars before)", n, 2);

    /* 61-62: %ln stores 32-bit long (lo word = cc, hi word = 0) */
    lnp = &ln;
    *lnp = 99; *(lnp + 1) = 99;          /* pre-dirty both words */
    sprintf(buf, "hello%ln", &ln);
    checkn("%ln lo word (5)", *lnp, 5);
    checkn("%ln hi word (0)", *(lnp + 1), 0);

    /* ===================== Long conversions ===================== */

    /* 63: %ld positive */
    lv = 100000;
    sprintf(buf, "%ld", lv);
    check("%ld (100000)", buf, "100000");

    /* 64: %ld negative */
    lv = -100000;
    sprintf(buf, "%ld", lv);
    check("%ld (-100000)", buf, "-100000");

    /* 65: %ld zero */
    lv = 0;
    sprintf(buf, "%ld", lv);
    check("%ld (0)", buf, "0");

    /* 66: %lu unsigned long */
    lv = 65536;
    sprintf(buf, "%lu", lv);
    check("%lu (65536)", buf, "65536");

    /* 67: %lx lowercase */
    lv = 0x1ABC;
    sprintf(buf, "%lx", lv);
    check("%lx (0x1abc)", buf, "1abc");

    /* 68: %lX uppercase */
    lv = 0x1ABC;
    sprintf(buf, "%lX", lv);
    check("%lX (0x1ABC)", buf, "1ABC");

    /* 69: %lo octal */
    lv = 8;
    sprintf(buf, "%lo", lv);
    check("%lo (8)", buf, "10");

    /* 70: %+ld positive */
    lv = 42;
    sprintf(buf, "%+ld", lv);
    check("%+ld positive", buf, "+42");

    /* 71: %+ld negative */
    lv = -42;
    sprintf(buf, "%+ld", lv);
    check("%+ld negative", buf, "-42");

    /* ===================== h modifier ===================== */

    /* 72: %hd same as %d on this target */
    sprintf(buf, "%hd", 42);
    check("%hd (42)", buf, "42");

    /* 73: %hu same as %u on this target */
    sprintf(buf, "%hu", 65535);
    check("%hu (65535)", buf, "65535");
}

int main() {
    passed = 0;
    failed = 0;

    test_integers();
    test_misc_convs();
    test_width_prec();
    test_n_and_long();

    /* ===================== Summary ===================== */

    printf("\n=== Test Summary ===\n");
    printf("Passed: %d\n", passed);
    printf("Failed: %d\n", failed);
    printf("Total:  %d\n", passed + failed);
    if (failed == 0)
        printf("ALL TESTS PASSED.\n");
    else
        printf("*** THERE WERE FAILURES ***\n");

    if (failed) getchar();
    return failed;
}

