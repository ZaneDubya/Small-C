#include "stdio.h"

// Parsed representation of a single % format specifier.
struct FmtSpec {
    int conv;                           // conversion letter
    int left, pad;                      // justify flag, fill character
    int width, prec, hasprec;           // field geometry
    int islong, isshort;                // length modifier flags
    int hasplus, hasspace, hasalt;      // sign / prefix flags
};

// sprintf(buf, ctlstring, arg, arg, ...) - Formatted print to string.
// Operates as described by Kernighan & Ritchie.
// Flags:       -, +, space, #, 0
// Width/prec:  decimal literal or * (runtime argument)
// Modifiers:   h, l
// Conversions: b, c, d, i, n, o, p, s, u, x, X  (b is non-standard)
int sprintf(char *buf, char *fmt, ...) {
    return(_sprint(buf, &fmt));
}

// Load a 32-bit long from the variadic argument stream into *dest,
// advancing the stream pointer *na by two int-sized slots.
void loadLong(int *dest, int **na) {
    *dest       = **na; ++(*na);
    *(dest + 1) = **na; ++(*na);
}

// parseFmtSpec(ctl, nxtarg, s)
// Advances *ctl past flags / width / precision / length-modifier / conv.
// Consumes runtime '*' width or precision values from *nxtarg.
// Fills all fields of *s; sets s->conv to the conversion character.
void parseFmtSpec(char **ctl, int **nxtarg, struct FmtSpec *s) {
    char *p;
    p = *ctl;

    // flags (any order, repeatable)
    s->left = 0; s->hasplus = 0; s->hasspace = 0; s->hasalt = 0; s->pad = ' ';
    while(*p == '-' || *p == '+' || *p == ' ' || *p == '#' || *p == '0') {
        if     (*p == '-') s->left     = 1;
        else if(*p == '+') s->hasplus  = 1;
        else if(*p == ' ') s->hasspace = 1;
        else if(*p == '#') s->hasalt   = 1;
        else               s->pad      = '0';
        ++p;
    }

    // field width
    if(*p == '*') {
        s->width = **nxtarg; ++(*nxtarg);
        ++p;
        // Negative runtime width means left-justify; guard INT_MIN.
        if(s->width < 0) {
            s->left  = 1;
            s->width = (s->width == -32768) ? 32767 : -s->width;
        }
    }
    else if(isdigit(*p)) {
        s->width = atoi(p++);
        while(isdigit(*p)) ++p;
    }
    else s->width = 0;

    // precision
    s->hasprec = 0; s->prec = 0;
    if(*p == '.') {
        ++p;
        if(*p == '*') {
            s->prec = **nxtarg; ++(*nxtarg);
            ++p;
            if(s->prec >= 0) s->hasprec = 1;
            else             s->prec    = 0;
        }
        else {
            s->hasprec = 1;
            s->prec = atoi(p);
            while(isdigit(*p)) ++p;
        }
    }

    // length modifier
    // 'h' (isshort): on this 16-bit target int == short, so %h is a
    // documented no-op; the flag is retained for future use or docs.
    s->islong = 0; s->isshort = 0;
    if     (*p == 'l') { s->islong  = 1; ++p; }
    else if(*p == 'h') { s->isshort = 1; ++p; }
    s->conv = *p++;

    *ctl = p;
}

// convToStr(s, nxtarg, str, sptr_out, sign_out, lval, intconv_out)
// Consumes the conversion argument(s) from *nxtarg.
// Writes the unsigned digit string (or char value) to str[].
// For %s, sets *sptr_out to the argument string; otherwise to str.
// Sets *sign_out ('-', '+', ' ', or 0) and *intconv_out (1 for numeric).
// Returns 1 on success, 0 for an unrecognised conversion (signals abort).
int convToStr(struct FmtSpec *s, int **nxtarg,
              char *str, char **sptr_out, int *sign_out,
              long *lval, int *intconv_out) {
    int arg, *lp;
    char *p;

    *sign_out    = 0;
    *sptr_out    = str;
    str[0]       = '\0';
    *intconv_out = 0;

    if(s->conv == 'c') {
        arg = **nxtarg; ++(*nxtarg);
        str[0] = arg; str[1] = '\0';
    }
    else if(s->conv == 's') {
        *sptr_out = **nxtarg; ++(*nxtarg);
        if(*sptr_out == NULL) *sptr_out = "(null)";
    }
    else if(s->conv == 'd' || s->conv == 'i') {
        *intconv_out = 1;
        if(s->islong) {
            lp = lval;                  // type-pun long* as int* for word access
            loadLong(lp, nxtarg);
            if(*(lp + 1) & 0x8000) {
                *sign_out = '-';
                *lp = -*lp; *(lp + 1) = ~*(lp + 1);
                if(*lp == 0) *(lp + 1) += 1;
            }
            else if(s->hasplus)  *sign_out = '+';
            else if(s->hasspace) *sign_out = ' ';
            ltoab(*lval, str, 10);
        }
        else {
            arg = **nxtarg; ++(*nxtarg);
            if(arg < 0) {
                *sign_out = '-';
                // Pass (0 - arg) to itoab, which treats its argument as
                // unsigned.  For INT_MIN (-32768), the subtraction wraps
                // to 0x8000 = unsigned 32768, correctly rendered.
                itoab(0 - arg, str, 10);
            }
            else {
                if(s->hasplus)       *sign_out = '+';
                else if(s->hasspace) *sign_out = ' ';
                itoab(arg, str, 10);
            }
        }
    }
    else if(s->conv == 'u') {
        *intconv_out = 1;
        if(s->islong) { lp = lval; loadLong(lp, nxtarg); ltoab(*lval, str, 10); }
        else          { arg = **nxtarg; ++(*nxtarg); itoab(arg, str, 10); }
    }
    else if(s->conv == 'o') {
        *intconv_out = 1;
        if(s->islong) { lp = lval; loadLong(lp, nxtarg); ltoab(*lval, str,  8); }
        else          { arg = **nxtarg; ++(*nxtarg); itoab(arg, str,  8); }
    }
    else if(s->conv == 'x') {
        *intconv_out = 1;
        if(s->islong) { lp = lval; loadLong(lp, nxtarg); ltoab(*lval, str, 16); }
        else          { arg = **nxtarg; ++(*nxtarg); itoab(arg, str, 16); }
        p = str;
        while(*p) { if(*p >= 'A' && *p <= 'F') *p += 32; ++p; }
    }
    else if(s->conv == 'X') {
        *intconv_out = 1;
        if(s->islong) { lp = lval; loadLong(lp, nxtarg); ltoab(*lval, str, 16); }
        else          { arg = **nxtarg; ++(*nxtarg); itoab(arg, str, 16); }
    }
    else if(s->conv == 'b') {
        *intconv_out = 1;
        if(s->islong) { lp = lval; loadLong(lp, nxtarg); ltoab(*lval, str,  2); }
        else          { arg = **nxtarg; ++(*nxtarg); itoab(arg, str,  2); }
    }
    else if(s->conv == 'p') {
        *intconv_out = 1;
        arg = **nxtarg; ++(*nxtarg);
        itoab(arg, str, 16);
    }
    else return 0;

    return 1;
}

// prepField(s, intconv, str, sptr, sign_in,
//           pfx, pfxlen_out, len_out, extra_out,
//           fill_out, pad_out, sign_out)
// Applies precision (computing extra leading zeros without touching str),
// builds the alternate-form prefix into pfx[], computes the fill count,
// and resolves the final pad character and sign.
// All results are returned through the _out parameters.
void prepField(struct FmtSpec *s, int intconv,
               char *str, char *sptr, int sign_in,
               char *pfx, int *pfxlen_out,
               int *len_out, int *extra_out,
               int *fill_out, int *pad_out, int *sign_out) {
    int len, nonzero, extra, fill, pfxlen, pad;

    len    = strlen(sptr);
    extra  = 0;
    pfxlen = 0;
    pfx[0] = '\0';
    pad    = s->pad;
    *sign_out = sign_in;

    if(intconv) {
        nonzero = !(str[0] == '0' && str[1] == '\0');

        if(s->hasprec) {
            // C std: precision 0 with zero value -> emit nothing, not "0".
            if(s->prec == 0 && !nonzero) {
                len = 0; str[0] = '\0'; *sign_out = 0;
            }
            // Precision requires more digits than we have: record the count
            // of leading zeros to emit at output time, avoiding any write
            // to str and eliminating the fixed-buffer overflow.
            else if(s->prec > len) {
                extra = s->prec - len;
                len   = s->prec;        // total logical digit width
            }
            pad = ' ';                  // explicit prec suppresses '0'-fill
        }

        // Alternate-form prefix (evaluated after precision, before fill).
        if(s->hasalt) {
            if(s->conv == 'o' && str[0] != '0' && !extra) {
                // extra leading zeros already provide a leading 0
                pfx[0] = '0'; pfx[1] = '\0'; pfxlen = 1;
            }
            else if(s->conv == 'x' && nonzero) {
                pfx[0] = '0'; pfx[1] = 'x'; pfx[2] = '\0'; pfxlen = 2;
            }
            else if(s->conv == 'X' && nonzero) {
                pfx[0] = '0'; pfx[1] = 'X'; pfx[2] = '\0'; pfxlen = 2;
            }
        }
    }
    else if(s->conv == 's') {
        // precision truncates string to at most prec characters
        if(s->hasprec && s->prec < len) len = s->prec;
    }

    if(s->left) pad = ' ';             // left-justify overrides '0'-pad

    fill = pfxlen + len;
    if(*sign_out) ++fill;
    fill = (s->width > fill) ? s->width - fill : 0;

    *len_out    = len;
    *extra_out  = extra;
    *fill_out   = fill;
    *pad_out    = pad;
    *pfxlen_out = pfxlen;
}

// emitField(dst, sptr, pfx, sign, fill, extra, len, left, pad)
// Writes sign, prefix, padding, and digit/string characters to *dst,
// advancing the pointer as it goes.
// Returns the number of characters written.
int emitField(char **dst, char *sptr, char *pfx,
              int sign, int fill, int extra, int len,
              int left, int pad) {
    char *p;
    int cc, i;
    cc = 0;

    if(!left && pad == '0') {
        // sign and prefix first, then zero-fill, then digits
        if(sign)           { **dst = sign;  ++(*dst); ++cc; }
        p = pfx; while(*p) { **dst = *p++; ++(*dst); ++cc; }
        while(fill--)      { **dst = '0';   ++(*dst); ++cc; }
        i = extra;     while(i--)        { **dst = '0';    ++(*dst); ++cc; }
        i = len-extra; while(i-- > 0)   { **dst = *sptr++; ++(*dst); ++cc; }
    }
    else if(!left) {
        // space-fill, then sign / prefix / digits
        while(fill--)      { **dst = ' ';   ++(*dst); ++cc; }
        if(sign)           { **dst = sign;  ++(*dst); ++cc; }
        p = pfx; while(*p) { **dst = *p++; ++(*dst); ++cc; }
        i = extra;     while(i--)        { **dst = '0';    ++(*dst); ++cc; }
        i = len-extra; while(i-- > 0)   { **dst = *sptr++; ++(*dst); ++cc; }
    }
    else {
        // sign / prefix / digits, then space-fill
        if(sign)           { **dst = sign;  ++(*dst); ++cc; }
        p = pfx; while(*p) { **dst = *p++; ++(*dst); ++cc; }
        i = extra;     while(i--)        { **dst = '0';    ++(*dst); ++cc; }
        i = len-extra; while(i-- > 0)   { **dst = *sptr++; ++(*dst); ++cc; }
        while(fill--)      { **dst = ' ';   ++(*dst); ++cc; }
    }
    return cc;
}

// _sprint(buf, argptr) - core string formatter; backs sprintf().
// buf:    destination buffer (null-terminated on return).
// argptr: pointer to the fmt argument on the stack; variadic args follow.
int _sprint(char *buf, int *nxtarg) {
    struct FmtSpec s;
    char *ctl, *sptr, *dst, str[48], pfx[4];
    int *lp, cc, sign, intconv, len, extra, fill, pad, pfxlen;
    long lval;

    cc  = 0;
    dst = buf;
    ctl = *nxtarg++;

    while(*ctl) {
        if(*ctl != '%') { *dst++ = *ctl++; ++cc; continue; }
        ++ctl;
        if(*ctl == '%') { *dst++ = *ctl++; ++cc; continue; }

        parseFmtSpec(&ctl, &nxtarg, &s);

        // %n: side-effect only; no conversion or emit phase
        if(s.conv == 'n') {
            lp = *nxtarg++;
            if(s.islong) { *lp = cc; *(lp + 1) = 0; }
            else           *lp = cc;
            continue;
        }

        if(!convToStr(&s, &nxtarg, str, &sptr, &sign, &lval, &intconv))
            break;                      // unknown conversion: stop

        prepField(&s, intconv, str, sptr, sign,
                  pfx, &pfxlen, &len, &extra, &fill, &pad, &sign);

        cc += emitField(&dst, sptr, pfx, sign, fill, extra, len, s.left, pad);
    }
    *dst = '\0';
    return(cc);
}
