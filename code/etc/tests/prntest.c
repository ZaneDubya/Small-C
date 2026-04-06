/* prntest.c -- Quick printf smoke test.
** Reports numbered lines; each should match its label exactly.
*/
#include "../../smallc22/stdio.h"

int main() {
    int i;
    char *s;
    i = 42;
    s = "hello";

    /* 1: plain string, no args */
    printf("1: plain\n");

    /* 2: single integer arg */
    printf("2: i=%d\n", i);

    /* 3: two args */
    printf("3: i=%d s=%s\n", i, s);

    /* 4: three args */
    printf("4: %d %d %d\n", 1, 2, 3);

    /* 5: hex formatting */
    printf("5: hex=%x\n", 255);

    /* 6: string arg only */
    printf("6: s=%s\n", s);

    /* 7: no newline at end of format -- next line should follow immediately */
    printf("7: no-nl ");
    printf("same-line\n");

    printf("done\n");
    return 0;
}
