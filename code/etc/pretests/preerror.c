/* preerror.c -- Negative-compilation test for the #error directive.
 *
 * This file is intentionally designed to FAIL to compile.
 * The #error directive below must cause the compiler to abort with ERRCODE (7).
 *
 * The test harness (preerror.bat) verifies that:
 *   1. The compiler exits with errorlevel >= 1 (abort was called).
 *   2. No output .ASM file was produced (compilation was halted cleanly).
 *
 * Nothing in this file is ever assembled, linked, or run.
 */

#include <stdio.h>

/* This #error must fire -- ALWAYS_DEFINED is defined right above it. */
#define ALWAYS_DEFINED 1

#ifdef ALWAYS_DEFINED
#error This error message must appear and compilation must stop.
#endif

/* If the compiler incorrectly continues past #error, code below would
 * produce a valid ASM file -- detected by preerror.bat as a test failure. */
int main() {
    return 0;
}
