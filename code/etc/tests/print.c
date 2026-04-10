// print.c -- printf/sprintf smoke test for Small-C stdio library.
//
// Purpose:
//   Verify that the printf and sprintf functions in the Small-C standard
//   library produce correct formatted output for all supported format
//   specifiers and edge cases.
//
// Functionality covered:
//   - Plain string with no conversion specifiers
//   - %d: positive integer, negative integer, zero
//   - %d: multiple arguments in one format string
//   - %s: string argument, empty string argument
//   - Mixed %d and %s in the same format string
//   - %x: hexadecimal output (implementation emits uppercase digits)
//   - %x: zero value
//   - %c: single character from integer code point
//   - %%: literal percent sign escape
//   - Literal text surrounding a conversion specifier
//   - %u: unsigned decimal integer (including max 16-bit value 65535)
//   - sprintf writes to a caller-supplied buffer (not stdout)
//
/* print.c -- printf/sprintf smoke test with PASS/FAIL reporting.
** Each case formats into a buffer with sprintf, then compares
** against the expected string.  A summary is printed at the end.
*/
#include "../../smallc22/stdio.h"

int passed, failed;
char buf[64];

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
        printf("    expected: %s\n", expect);
        printf("    got:      %s\n", got);
        failed++;
        getchar();
    }
}

int main() {
    passed = 0;
    failed = 0;

    /* 1: plain string, no conversion */
    sprintf(buf, "plain");
    check("plain string", buf, "plain");

    /* 2: %d positive */
    sprintf(buf, "%d", 42);
    check("%d positive", buf, "42");

    /* 3: %d negative */
    sprintf(buf, "%d", -7);
    check("%d negative", buf, "-7");

    /* 4: %d zero */
    sprintf(buf, "%d", 0);
    check("%d zero", buf, "0");

    /* 5: multiple %d args */
    sprintf(buf, "%d %d %d", 1, 2, 3);
    check("multiple %d", buf, "1 2 3");

    /* 6: %s */
    sprintf(buf, "%s", "hello");
    check("%s", buf, "hello");

    /* 7: mixed %d and %s */
    sprintf(buf, "i=%d s=%s", 42, "hello");
    check("mixed %d %s", buf, "i=42 s=hello");

    /* 8: %x lowercase hex */
    sprintf(buf, "%x", 255);
    check("%x (255)", buf, "FF");
    // NOTE - THIS IS NON-STANDARD BEHAVIOR: uppercase hex digits.  The C standard allows either
    // uppercase or lowercase, but lowercase is more common.  We use uppercase because this is 
    // the version implemented in SmallLib's ITOAB.
    // TODO: add %X for uppercase hex and make %x lowercase to match common convention.

    /* 9: %x zero */
    sprintf(buf, "%x", 0);
    check("%x (0)", buf, "0");

    /* 10: %c character */
    sprintf(buf, "%c", 65);
    check("%c ('A')", buf, "A");

    /* 11: %% literal percent */
    sprintf(buf, "100%%");
    check("%% literal", buf, "100%");

    /* 12: adjacent literal text around conversion */
    sprintf(buf, "a%db", 9);
    check("literal around %d", buf, "a9b");

    /* 13: empty string arg */
    sprintf(buf, "%s", "");
    check("%s empty string", buf, "");

    /* 14: %u unsigned */
    sprintf(buf, "%u", 65535);
    check("%u (65535)", buf, "65535");

    /* Summary */
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
