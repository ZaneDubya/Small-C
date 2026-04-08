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
    check("%x (255)", buf, "ff");

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

    return failed;
}
