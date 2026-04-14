/* pretest.h -- Included by pretest.c to exercise #include processing.
 * Also pulls in inc_d1.h -> inc_d2.h -> inc_d3.h to test 4-deep nesting. */
#define INCLUDE_WORKS  1
#define INCLUDED_VAL   42
#include "inc_d1.h"
