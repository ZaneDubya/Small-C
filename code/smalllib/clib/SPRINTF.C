#include "stdio.h"
/*
** sprintf(buf, ctlstring, arg, arg, ...) - Formatted print to string.
** Operates as described by Kernighan & Ritchie.
** Flags:       -, +, space, #, 0
** Width/prec:  decimal literal or * (runtime argument)
** Modifiers:   h, l
** Conversions: b, c, d, i, n, o, p, s, u, x, X  (b is non-standard)
*/
int sprintf(char *buf, char *fmt, ...) {
    return(_sprint(buf, &fmt));
}

/*
** _sprint(buf, argptr) - core string formatter; backs sprintf().
** buf:    destination buffer (null-terminated on return).
** argptr: pointer to the fmt argument on the stack; variadic args follow.
*/
int _sprint(char *buf, int *nxtarg) {
    char *ctl, *sptr, *dst, str[48], pfx[4], *p;
    int arg, left, pad, cc, len, prec, width, *lp, sign,
        islong, isshort, hasplus, hasspace, hasalt, hasprec,
        conv, intconv, nonzero, pfxlen, fill, extra, i;
    long lval;
    cc = 0;
    dst = buf;
    ctl = *nxtarg++;
    while(*ctl) {
        if(*ctl != '%') {
            *dst++ = *ctl++;
            ++cc;
            continue;
        }
        ++ctl;
        if(*ctl == '%') {
            *dst++ = *ctl++;
            ++cc;
            continue;
        }
        /* --- flags (any order, repeatable) --- */
        left = 0; hasplus = 0; hasspace = 0; hasalt = 0; pad = ' ';
        while(*ctl == '-' || *ctl == '+' || *ctl == ' ' ||
              *ctl == '#' || *ctl == '0') {
            if     (*ctl == '-') left     = 1;
            else if(*ctl == '+') hasplus  = 1;
            else if(*ctl == ' ') hasspace = 1;
            else if(*ctl == '#') hasalt   = 1;
            else                 pad      = '0';
            ++ctl;
        }
        /* --- field width --- */
        if(*ctl == '*') {
            width = *nxtarg++;
            ++ctl;
            if(width < 0) { left = 1; width = -width; }
        }
        else if(isdigit(*ctl)) {
            width = atoi(ctl++);
            while(isdigit(*ctl)) ++ctl;
        }
        else width = 0;
        /* --- precision --- */
        hasprec = 0; prec = 0;
        if(*ctl == '.') {
            ++ctl;
            if(*ctl == '*') {
                prec = *nxtarg++;
                ++ctl;
                if(prec >= 0) hasprec = 1;
                else          prec    = 0;
            }
            else {
                hasprec = 1;
                prec = atoi(ctl);
                while(isdigit(*ctl)) ++ctl;
            }
        }
        /* --- length modifier --- */
        islong = 0; isshort = 0;
        if     (*ctl == 'l') { islong  = 1; ++ctl; }
        else if(*ctl == 'h') { isshort = 1; ++ctl; }
        conv = *ctl++;
        /* --- %n: store character count --- */
        if(conv == 'n') {
            lp = *nxtarg++;
            if(islong) { *lp = cc; *(lp + 1) = 0; }
            else        *lp = cc;
            continue;
        }
        /* --- conversion --- */
        sign     = 0;
        sptr     = str;
        str[0]   = '\0';
        intconv  = 0;
        if(conv == 'c') {
            arg = *nxtarg++;
            str[0] = arg;
            str[1] = '\0';
        }
        else if(conv == 's') {
            sptr = *nxtarg++;
        }
        else if(conv == 'd' || conv == 'i') {
            intconv = 1;
            if(islong) {
                lp = &lval;
                *lp       = *nxtarg++;
                *(lp + 1) = *nxtarg++;
                if(*(lp + 1) & 0x8000) {
                    sign = '-';
                    /* two's complement negate: correct for LONG_MIN */
                    *lp       = -*lp;
                    *(lp + 1) = ~*(lp + 1);
                    if(*lp == 0) *(lp + 1) += 1;
                }
                else if(hasplus)  sign = '+';
                else if(hasspace) sign = ' ';
                ltoab(lval, str, 10);
            }
            else {
                arg = *nxtarg++;
                if(arg < 0) {
                    sign = '-';
                    /*
                    ** Pass (0 - arg) to itoab, which treats its first
                    ** argument as unsigned.  For INT_MIN (-32768), the
                    ** subtraction wraps to 0x8000 = unsigned 32768, which
                    ** itoab correctly converts to "32768".
                    */
                    itoab(0 - arg, str, 10);
                }
                else {
                    if(hasplus)  sign = '+';
                    else if(hasspace) sign = ' ';
                    itoab(arg, str, 10);
                }
            }
        }
        else if(conv == 'u') {
            intconv = 1;
            if(islong) {
                lp = &lval;
                *lp = *nxtarg++; *(lp + 1) = *nxtarg++;
                ltoab(lval, str, 10);
            }
            else { arg = *nxtarg++; itoab(arg, str, 10); }
        }
        else if(conv == 'o') {
            intconv = 1;
            if(islong) {
                lp = &lval;
                *lp = *nxtarg++; *(lp + 1) = *nxtarg++;
                ltoab(lval, str, 8);
            }
            else { arg = *nxtarg++; itoab(arg, str, 8); }
        }
        else if(conv == 'x') {
            intconv = 1;
            if(islong) {
                lp = &lval;
                *lp = *nxtarg++; *(lp + 1) = *nxtarg++;
                ltoab(lval, str, 16);
            }
            else { arg = *nxtarg++; itoab(arg, str, 16); }
            p = str;
            while(*p) { if(*p >= 'A' && *p <= 'F') *p += 32; ++p; }
        }
        else if(conv == 'X') {
            intconv = 1;
            if(islong) {
                lp = &lval;
                *lp = *nxtarg++; *(lp + 1) = *nxtarg++;
                ltoab(lval, str, 16);
            }
            else { arg = *nxtarg++; itoab(arg, str, 16); }
        }
        else if(conv == 'b') {
            intconv = 1;
            if(islong) {
                lp = &lval;
                *lp = *nxtarg++; *(lp + 1) = *nxtarg++;
                ltoab(lval, str, 2);
            }
            else { arg = *nxtarg++; itoab(arg, str, 2); }
        }
        else if(conv == 'p') {
            intconv = 1;
            arg = *nxtarg++;
            itoab(arg, str, 16);
        }
        else {
            *dst = '\0';
            return (cc);
        }
        len = strlen(sptr);
        pfx[0] = '\0'; pfxlen = 0;
        if(intconv) {
            /* precision = minimum digit count: zero-pad up to prec */
            if(hasprec && prec > len) {
                extra = prec - len;
                /* shift digit string right by extra to make room */
                p = str + len;
                *(p + extra) = '\0';
                while(p > str) { --p; *(p + extra) = *p; }
                i = 0;
                while(i < extra) { str[i] = '0'; ++i; }
                len = prec;
            }
            /* alternate-form prefix (evaluated after precision fill) */
            nonzero = !(str[0] == '0' && str[1] == '\0');
            if(hasalt) {
                if(conv == 'o' && str[0] != '0') {
                    pfx[0] = '0'; pfx[1] = '\0'; pfxlen = 1;
                }
                else if(conv == 'x' && nonzero) {
                    pfx[0] = '0'; pfx[1] = 'x'; pfx[2] = '\0'; pfxlen = 2;
                }
                else if(conv == 'X' && nonzero) {
                    pfx[0] = '0'; pfx[1] = 'X'; pfx[2] = '\0'; pfxlen = 2;
                }
            }
            /* explicit precision suppresses the '0' field-fill flag */
            if(hasprec) pad = ' ';
        }
        else if(conv == 's') {
            /* precision truncates string to at most prec characters */
            if(hasprec && prec < len) len = prec;
        }
        /* '-' (left-justify) overrides '0' (zero-pad) */
        if(left) pad = ' ';
        /* field fill needed */
        fill = pfxlen + len;
        if(sign) ++fill;
        if(width > fill) fill = width - fill;
        else             fill = 0;
        /* --- emit --- */
        if(!left && pad == '0') {
            /* sign and prefix first, then zero fill, then digits */
            if(sign)  { *dst++ = sign; ++cc; }
            p = pfx;  while(*p) { *dst++ = *p++; ++cc; }
            while(fill--) { *dst++ = '0'; ++cc; }
            while(len--)  { *dst++ = *sptr++; ++cc; }
        }
        else if(!left) {
            /* space fill, then sign/prefix/digits */
            while(fill--)  { *dst++ = ' '; ++cc; }
            if(sign)  { *dst++ = sign; ++cc; }
            p = pfx;  while(*p) { *dst++ = *p++; ++cc; }
            while(len--)   { *dst++ = *sptr++; ++cc; }
        }
        else {
            /* sign/prefix/digits, then space fill */
            if(sign)  { *dst++ = sign; ++cc; }
            p = pfx;  while(*p) { *dst++ = *p++; ++cc; }
            while(len--)   { *dst++ = *sptr++; ++cc; }
            while(fill--)  { *dst++ = ' '; ++cc; }
        }
    }
    *dst = '\0';
    return(cc);
}
