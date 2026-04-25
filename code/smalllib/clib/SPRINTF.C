#include "stdio.h"

// sprintf(buf, ctlstring, arg, arg, ...) - Formatted print to string.
// Backed by _sprint() in cmn_prnt.c.
// Flags:       -, +, space, #, 0
// Width/prec:  decimal literal or * (runtime argument)
// Modifiers:   h, l
// Conversions: b, c, d, i, n, o, p, s, u, x, X  (b is non-standard)
int sprintf(char *buf, char *fmt, ...) {
    return(_sprint(buf, &fmt));
}
