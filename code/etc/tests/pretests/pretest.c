/* pretest.c -- Preprocessor test suite for Small-C ccpre.
 *
 * Covers:
 *   1.  #define object macros -- integer, char, and string values
 *   2.  Macro expansion inside expressions
 *   3.  Multi-word macro values
 *   4.  #ifdef defined symbol    -> true branch executes
 *   5.  #ifdef undefined symbol  -> else branch executes
 *   6.  #ifndef defined symbol   -> else branch executes
 *   7.  #ifndef undefined symbol -> true branch executes
 *   8.  Nested conditional blocks (two levels)
 *   9.  Three-deep nested conditionals (iflevel/skiplevel state machine)
 *  10.  String literal pass-through with escaped quote inside
 *  11.  Char literal escape sequences (\n \t \\)
 *  12.  Block comment stripping  ( slash-star ... star-slash )
 *  13.  Line comment stripping   ( // C99 style )
 *  14.  #include from a header file
 *  15.  #error inside a false branch is silently skipped
 *  16.  #undef removes macro from #ifdef visibility
 *  17.  redefine-after-#undef yields the new value
 *  18.  Predefined macros: __FILE__, __LINE__, __DATE__, __TIME__
 *  19.  #line overrides __LINE__ and __FILE__
 *  20.  Nested #include 4 levels deep
 */

#include <stdio.h>
#include "pretest.h"

int passed, failed;

/* ============================================================
 * Macro definitions under test
 * ============================================================ */

#define INT_ZERO    0
#define INT_ONE     1
#define INT_NEG     -1
#define INT_LARGE   32767

#define CHAR_A      65      /* ASCII 'A' */
#define CHAR_SQ     39      /* ASCII apostrophe */

/* Multi-word macro value: the two tokens are pasted into the output line */
#define MULTI  1 + 1

/* ============================================================
 * Conditional compilation markers
 * ============================================================ */

#define DEFINED_SYM 1
/* UNDEF_SYM is intentionally never defined */

/* --- #ifdef --- */

#ifdef DEFINED_SYM
int s_IfdefDef() { return 1; }
#else
int s_IfdefDef() { return 0; }
#endif

#ifdef UNDEF_SYM
int s_IfdefUndef() { return 0; }
#else
int s_IfdefUndef() { return 1; }
#endif

/* --- #ifndef --- */

#ifndef DEFINED_SYM
int s_IfndefDef() { return 0; }
#else
int s_IfndefDef() { return 1; }
#endif

#ifndef UNDEF_SYM
int s_IfndefUndef() { return 1; }
#else
int s_IfndefUndef() { return 0; }
#endif

/* --- nested (two levels): outer defined, inner not ---
 * Expected: outer true branch entered; inner else branch taken. */
#ifdef DEFINED_SYM
  #ifdef UNDEF_SYM
    int s_Nested2a() { return 0; }
  #else
    int s_Nested2a() { return 1; }
  #endif
#endif

/* --- nested (two levels): outer not defined ---
 * Expected: outer else branch taken; entire inner block skipped. */
#ifdef UNDEF_SYM
  #ifdef DEFINED_SYM
    int s_Nested2b() { return 0; }
  #else
    int s_Nested2b() { return 0; }
  #endif
#else
  int s_Nested2b() { return 1; }
#endif

/* --- nested (three levels): DEFINED / !UNDEF / DEFINED ---
 * Expected: innermost true branch executes. */
#ifdef DEFINED_SYM
  #ifndef UNDEF_SYM
    #ifdef DEFINED_SYM
      int s_Nested3() { return 1; }
    #else
      int s_Nested3() { return 0; }
    #endif
  #else
    int s_Nested3() { return 0; }
  #endif
#else
  int s_Nested3() { return 0; }
#endif

/* --- #error must be silent inside a skipped block ---
 * UNDEF_SYM is never defined, so the #error below must NOT fire.
 * If it does, compilation halts before this function is ever defined. */
#ifdef UNDEF_SYM
  #error This #error must not fire -- it is inside a false branch.
#endif
int s_ErrorSkipped() { return 1; }

/* --- #undef removes macro from #ifdef visibility ---
 * WILL_BE_UNDEFINED is defined then immediately un-defined.
 * The #ifdef below must take the #else branch. */
#define WILL_BE_UNDEFINED 77
#undef WILL_BE_UNDEFINED
#ifdef WILL_BE_UNDEFINED
int s_UndefHides() { return 0; }
#else
int s_UndefHides() { return 1; }
#endif

/* --- redefine after #undef yields the new value ---
 * The tombstone slot in macn must be reusable; the second #define must
 * overwrite it so that REDEF_ME expands to 20, not 10. */
#define REDEF_ME 10
#undef REDEF_ME
#define REDEF_ME 20
int s_Redefined() { return REDEF_ME; }

/* ============================================================
 * Test harness
 * ============================================================ */

void check(char *desc, int cond) {
    if (cond) {
        printf("  PASS: %s\n", desc);
        passed++;
    }
    else {
        printf("  FAIL: %s\n", desc);
        getchar();
        failed++;
    }
}

/* ============================================================
 * main
 * ============================================================ */

main() {
    int x;
    char *p;

    passed = 0;
    failed = 0;

    printf("=== Small-C Preprocessor Tests ===\n");

    /* --- integer macros --- */
    printf("\n-- Integer macros --\n");
    check("INT_ZERO == 0",       INT_ZERO == 0);
    check("INT_ONE == 1",        INT_ONE == 1);
    check("INT_NEG == -1",       INT_NEG == -1);
    check("INT_LARGE == 32767",  INT_LARGE == 32767);

    /* --- macros in expressions --- */
    printf("\n-- Macros in expressions --\n");
    x = INT_ONE + INT_ONE;
    check("INT_ONE + INT_ONE == 2",    x == 2);
    x = INT_LARGE - INT_LARGE;
    check("INT_LARGE - INT_LARGE == 0", x == 0);
    x = INT_NEG * INT_NEG;
    check("INT_NEG * INT_NEG == 1",    x == 1);

    /* --- multi-word macro value --- */
    printf("\n-- Multi-word macro --\n");
    x = MULTI;
    check("MULTI (1 + 1) == 2",  x == 2);

    /* --- char macros --- */
    printf("\n-- Char macros --\n");
    check("CHAR_A == 65",   CHAR_A == 65);
    check("CHAR_SQ == 39",  CHAR_SQ == 39);

    /* --- #include (from pretest.h) --- */
    printf("\n-- #include --\n");
    check("INCLUDE_WORKS == 1",  INCLUDE_WORKS == 1);
    check("INCLUDED_VAL == 42",  INCLUDED_VAL == 42);

    /* --- string literal with escaped quote ---
     * scanLiteral must not treat the \" as the closing delimiter.
     * p = "say \"hi\""  =>  s a y   " h i "  (8 chars + NUL)
     *                        0 1 2 3 4 5 6 7  */
    printf("\n-- String literal escape pass-through --\n");
    p = "say \"hi\"";
    check("escaped quote: p[4] == 34 (\")",   p[4] == 34);
    check("char after esc-quote p[5] == 'h'", p[5] == 'h');
    check("closing esc-quote: p[7] == 34",    p[7] == 34);

    /* --- char literal escape sequences --- */
    printf("\n-- Char literal escape sequences --\n");
    check("'\\n' == 10",  '\n' == 10);
    check("'\\t' == 9",   '\t' == 9);
    check("'\\\\' == 92", '\\' == 92);
    check("'A' == 65",    'A'  == 65);

    /* --- block comment stripping --- */
    printf("\n-- Block comment stripping --\n");
    x = 1; /* entire block comment */ x = x + 1;
    check("code after block comment: x == 2",    x == 2);
    x = /* comment in mid-expr */ 99;
    check("block comment mid-expression: x == 99", x == 99);

    /* --- C99 line comment stripping --- */
    printf("\n-- Line comment stripping --\n");
    x = 7; // this comment must be stripped
    check("x == 7 after line comment", x == 7);
    x = 3; // another comment
    x = x + x; // and again
    check("x == 6 after two line-commented lines", x == 6);

    /* --- conditional compilation --- */
    printf("\n-- Conditional compilation --\n");
    check("#ifdef defined sym  -> true branch",   s_IfdefDef()    == 1);
    check("#ifdef undef sym    -> false branch",   s_IfdefUndef()  == 1);
    check("#ifndef defined sym -> false branch",   s_IfndefDef()   == 1);
    check("#ifndef undef sym   -> true branch",    s_IfndefUndef() == 1);

    /* --- nested conditionals --- */
    printf("\n-- Nested conditionals --\n");
    check("2-deep: outer def, inner undef -> inner #else",  s_Nested2a() == 1);
    check("2-deep: outer undef -> outer #else, inner skip", s_Nested2b() == 1);
    check("3-deep: def/!undef/def -> innermost true branch", s_Nested3() == 1);

    /* --- #error handling ---
     * s_ErrorSkipped() is defined only if the #error in the false branch
     * was correctly suppressed; if the compiler halted, we never get here. */
    printf("\n-- #error --\n");
    check("#error inside false branch is silently skipped", s_ErrorSkipped() == 1);

    /* --- #undef --- */
    printf("\n-- #undef --\n");
    check("#undef hides macro from #ifdef",          s_UndefHides()  == 1);
    check("redefine-after-#undef yields new value",  s_Redefined()   == 20);

    /* --- predefined macros ---
     * __DATE__[0] is the first letter of the month abbreviation (e.g. 'A').
     * __TIME__[2] is always ':' (separating HH from MM). */
    printf("\n-- predefined macros --\n");
    check("__LINE__ > 0",              __LINE__ > 0);
    check("__FILE__[0] != 0",          __FILE__[0] != 0);
    check("__DATE__[0] is alpha",      __DATE__[0] >= 'A' && __DATE__[0] <= 'S');
    check("__TIME__[2] == ':'",        __TIME__[2] == ':');

    /* --- #line ---
     * After '#line 999', __LINE__ on the very next source line is 999:
     * doline() stores N-1; the following doInline() increments to N. */
    printf("\n-- #line --\n");
#line 999
    check("#line sets __LINE__",       __LINE__ == 999);
#line 1 "linefile.c"
    check("#line sets __FILE__",       __FILE__[0] == 'l');

    /* --- nested #include ---
     * pretest.c includes pretest.h (depth 1) which includes inc_d1.h (depth 2)
     * which includes inc_d2.h (depth 3) which includes inc_d3.h (depth 4).
     * Each header defines a unique macro; all four must be visible here. */
    printf("\n-- nested #include --\n");
    check("depth-1 macro (INCLUDE_WORKS)",  INCLUDE_WORKS  == 1);
    check("depth-2 macro (NESTED_D1)",      NESTED_D1      == 10);
    check("depth-3 macro (NESTED_D2)",      NESTED_D2      == 20);
    check("depth-4 macro (NESTED_D3)",      NESTED_D3      == 30);

    /* --- summary --- */
    printf("\n=== Results: %d passed, %d failed ===\n", passed, failed);
    if (failed)
        printf("*** FAILURES DETECTED ***\n");
    else
        printf("All tests passed.\n");
}
