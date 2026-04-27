#include "stdio.h"

// cmn_prnt.c -- Shared formatting engine for sprintf() and fprintf()/printf().
//
// Exports:
//   _sprint(buf, nxtarg)  -- write formatted output to a string buffer
//   _print(fd, nxtarg)    -- write formatted output to a file descriptor
//
// Assembly-size design notes:
//   - Named int locals for direct [BP-N] loads (no LEA+ADD+MOV[BX] for arrays)
//   - Flag parsing: for(;;){ch=*ctl;...} loads *ctl once per iteration instead
//     of 5+4 times for while(a||b||c||d||e) condition + body
//   - Unsigned dispatch: base-select chain contains the break, eliminating the
//     redundant 6-way outer gate that duplicated all six conv comparisons
//   - fill_sum temp: (sign != 0) evaluated once; cc += fill + fill_sum avoids
//     the second branch
//   - sprint flag + direct dst: **pdst = x; ++(*pdst) (8 instr per char) replaced
//     by *dst++ = x (6 instr per char); eliminates 14 double-dereferences
//   - emitField, prepField, parseFmtSpec, convToStr, loadLong: all inlined
//   - cc counted once from geometry, not per-character in emit loops

// Flags:       -, +, space, #, 0
// Width/prec:  decimal literal or * (runtime argument)
// Modifiers:   h, l
// Conversions: b, c, d, i, n, o, p, s, u, x, X  (b is non-standard)

int _format(char *buf, int fd, int *nxtarg) {
    // Format-spec fields: named locals for direct [BP-N] access.
    int conv, left, pad, width, prec, hasprec, islong;
    int hasplus, hasspace, hasalt;
    // Working variables.
    char *ctl, *p, *sptr, *dst, str[34], pfx[4];
    int *lp, cc, sign, intconv, arg, base;
    int len, extra, fill, fill_sum, pfxlen, pad2, ndig, nonzero;
    int sprint, ch;
    long lval;

    cc     = 0;
    dst    = buf;
    sprint = (buf != NULL);
    ctl    = (char *)*nxtarg++;

    while(*ctl) {

        // Literal character.
        if(*ctl != '%') {
            if(sprint)
                *dst++ = *ctl++;
            else
                fputc(*ctl++, fd);
            ++cc;
            continue;
        }
        ++ctl;

        // Escaped '%%'.
        if(*ctl == '%') {
            if(sprint)
                *dst++ = *ctl++;
            else
                fputc(*ctl++, fd);
            ++cc;
            continue;
        }

        // --- Parse flags ---
        // ch caches *ctl so the loop body and the loop-exit test share one load
        // instead of the while(a||b||c||d||e) pattern that loads *ctl 5+4 times.
        left = hasplus = hasspace = hasalt = 0;
        pad  = ' ';
        for(;;) {
            ch = *ctl;
            if(ch == '-')
                left     = 1;
            else if(ch == '+')
                hasplus  = 1;
            else if(ch == ' ')
                hasspace = 1;
            else if(ch == '#')
                hasalt   = 1;
            else if(ch == '0')
                pad      = '0';
            else
                break;
            ++ctl;
        }

        // --- Field width ---
        if(*ctl == '*') {
            width = *nxtarg++;
            ++ctl;
            if(width < 0) {
                left  = 1;
                width = -width;
            }
        }
        else if(isdigit(*ctl)) {
            width = atoi(ctl);
            while(isdigit(*ctl))
                ++ctl;
        }
        else {
            width = 0;
        }

        // --- Precision ---
        hasprec = prec = 0;
        if(*ctl == '.') {
            ++ctl;
            if(*ctl == '*') {
                prec = *nxtarg++;
                ++ctl;
                if(prec >= 0)
                    hasprec = 1;
                else
                    prec = 0;
            }
            else {
                hasprec = 1;
                prec    = atoi(ctl);
                while(isdigit(*ctl))
                    ++ctl;
            }
        }

        // --- Length modifier ---
        // 'h': on this 16-bit target int == short, so %h is a no-op.
        islong = 0;
        if(*ctl == 'l') {
            islong = 1;
            ++ctl;
        }
        else if(*ctl == 'h') {
            ++ctl;
        }

        conv = *ctl++;

        // %n: store character count; no output.
        if(conv == 'n') {
            lp  = (int *)*nxtarg++;
            *lp = cc;
            if(islong)
                *(lp + 1) = 0;
            continue;
        }

        // --- Convert argument to string ---
        sign    = 0;
        sptr    = str;
        str[0]  = '\0';
        intconv = 0;

        if(conv == 'c') {
            arg    = *nxtarg++;
            str[0] = (char)arg;
            str[1] = '\0';
        }
        else if(conv == 's') {
            sptr = (char *)*nxtarg++;
            if(!sptr)
                sptr = "(null)";
        }
        else if(conv == 'd' || conv == 'i') {
            intconv = 1;
            if(islong) {
                lp        = (int *)&lval;
                *lp       = *nxtarg++;
                *(lp + 1) = *nxtarg++;
                if(*(lp + 1) & 0x8000) {
                    // Negate two's complement across two 16-bit halves.
                    sign      = '-';
                    *lp       = -*lp;
                    *(lp + 1) = ~*(lp + 1);
                    if(!*lp)
                        *(lp + 1) += 1;
                }
                else if(hasplus)
                    sign = '+';
                else if(hasspace)
                    sign = ' ';
                ltoab(lval, str, 10);
            }
            else {
                arg = *nxtarg++;
                if(arg < 0) {
                    sign = '-';
                    // (0 - arg) wraps INT_MIN to 0x8000 = unsigned 32768.
                    itoab(0 - arg, str, 10);
                }
                else {
                    if(hasplus)
                        sign = '+';
                    else if(hasspace)
                        sign = ' ';
                    itoab(arg, str, 10);
                }
            }
        }
        else {
            // Unsigned conversions: u, o, x, X, b, p.
            // The base-select chain validates conv; unknown chars break here,
            // eliminating the separate 6-way outer gate that was a full
            // duplicate of these same comparisons.
            if(conv == 'o')
                base = 8;
            else if(conv == 'b')
                base = 2;
            else if(conv == 'u')
                base = 10;
            else if(conv == 'x' || conv == 'X' || conv == 'p')
                base = 16;
            else
                break;          // unknown conversion: stop
            intconv = 1;
            if(islong) {
                lp        = (int *)&lval;
                *lp       = *nxtarg++;
                *(lp + 1) = *nxtarg++;
                ltoab(lval, str, base);
            }
            else {
                arg = *nxtarg++;
                itoab(arg, str, base);
            }
            // %x: lowercase -- shift A-F to a-f.
            if(conv == 'x') {
                p = str;
                while(*p) {
                    if(*p >= 'A' && *p <= 'F')
                        *p += 32;
                    ++p;
                }
            }
        }

        // --- Field geometry ---
        len    = strlen(sptr);
        extra  = 0;
        pfxlen = 0;
        pfx[0] = '\0';
        pad2   = pad;

        if(intconv) {
            // Direct comparison avoids __lneg library call.
            nonzero = (str[0] != '0' || str[1] != '\0');

            if(hasprec) {
                if(prec == 0 && !nonzero) {
                    // C std: precision 0 with zero value -> emit nothing.
                    len    = 0;
                    str[0] = '\0';
                    sign   = 0;
                }
                else if(prec > len) {
                    extra = prec - len;
                    len   = prec;
                }
                pad2 = ' ';     // explicit precision suppresses '0'-pad
            }

            if(hasalt) {
                if(conv == 'o' && str[0] != '0' && !extra) {
                    pfx[0]  = '0';
                    pfx[1]  = '\0';
                    pfxlen  = 1;
                }
                else if((conv == 'x' || conv == 'X') && nonzero) {
                    pfx[0]  = '0';
                    pfx[1]  = (char)conv;     // 'x' or 'X'
                    pfx[2]  = '\0';
                    pfxlen  = 2;
                }
            }
        }
        else if(conv == 's') {
            if(hasprec && prec < len)
                len = prec;
        }

        if(left)
            pad2 = ' ';

        // fill_sum computed once: reused for both fill and cc so that
        // (sign != 0) is only evaluated once instead of twice.
        fill_sum = pfxlen + len + (sign != 0);
        fill     = (width > fill_sum) ? width - fill_sum : 0;
        ndig     = len - extra;
        cc      += fill + fill_sum;     // = max(width, fill_sum); no branch

        // --- Emit field ---
        // sprint flag resolved once; *dst++ avoids the **pdst double-dereference.
        if(sprint) {
            if(!left && pad2 != '0')
                while(fill--) *dst++ = ' ';
            if(sign) *dst++ = (char)sign;
            p = pfx;
            while(*p) *dst++ = *p++;
            if(!left && pad2 == '0')
                while(fill--) *dst++ = '0';
            while(extra--) *dst++ = '0';
            while(ndig-- > 0) *dst++ = *sptr++;
            if(left)
                while(fill--) *dst++ = ' ';
        }
        else {
            if(!left && pad2 != '0')
                while(fill--) fputc(' ', fd);
            if(sign) fputc(sign, fd);
            p = pfx;
            while(*p) fputc(*p++, fd);
            if(!left && pad2 == '0')
                while(fill--) fputc('0', fd);
            while(extra--) fputc('0', fd);
            while(ndig-- > 0) fputc(*sptr++, fd);
            if(left)
                while(fill--) fputc(' ', fd);
        }
    }

    if(sprint)
        *dst = '\0';
    return cc;
}

// _sprint(buf, nxtarg) - formatted print to string buffer; backs sprintf().
int _sprint(char *buf, int *nxtarg) {
    return _format(buf, 0, nxtarg);
}

// _print(fd, nxtarg) - formatted print to file descriptor; backs fprintf()/printf().
int _print(int fd, int *nxtarg) {
    return _format(NULL, fd, nxtarg);
}
