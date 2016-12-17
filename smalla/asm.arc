>>> ADD32.C 389
/*
** 32 bit x + y to x
*/
add32(x, y) unsigned x[], y[]; {
  #asm
  MOV  BX,[BP+4]       ; locate y low
  MOV  AX,[BX]         ; fetch y low
  XCHG CX,BX           ; save address
  MOV  BX,[BP+6]       ; locate x low
  ADD  [BX],AX         ; add y low to x low
  XCHG CX,BX
  MOV  AX,[BX+2]       ; fetch y high
  XCHG CX,BX
  ADC  [BX+2],AX       ; add y high with carry
  #endasm
  }


>>> ADD64.C 876
/*
** 64-bit x + y to x
** return true if carried out of high order bit
*/
add64(x, y) unsigned x[], y[]; {
  #asm
  MOV  BX,[BP+4]       ; locate y
  MOV  AX,[BX]         ; fetch y[0]
  XCHG CX,BX           ; save address
  MOV  BX,[BP+6]       ; locate x
  ADD  [BX],AX         ; add y[0] to x[0]
  XCHG CX,BX           ; locate y
  MOV  AX,[BX+2]       ; fetch y[1]
  XCHG CX,BX           ; locate x
  ADC  [BX+2],AX       ; add y[1] to x[1] with carry
  XCHG CX,BX           ; locate y
  MOV  AX,[BX+4]       ; fetch y[2]
  XCHG CX,BX           ; locate x
  ADC  [BX+4],AX       ; add y[2] to x[2] with carry
  XCHG CX,BX           ; locate y
  MOV  AX,[BX+6]       ; fetch y[3]
  XCHG CX,BX           ; locate x
  ADC  [BX+6],AX       ; add y[3] to x[3] with carry
  RCL  AX,1            ; fetch CF
  AND  AX,1            ; strip other bits and return it
  #endasm
  }


>>> AND32.C 98
/*
** 32 bit x & y to x
*/
and32(x, y) unsigned x[], y[]; {
  x[0] &= y[0];
  x[1] &= y[1];
  }


>>> ATOFT.C 2816
/*
** convert ASCII string (s) to temporary float (f)
**
** return length of ASCII field used
**
** ASCII STRING
**
**    [whitespace] [sign] [digits] [.digits] [{d|D|e|E} [sign] digits]
**
** TEMPORARY FLOAT
**
**       79   78      64 63          0
**     ------ ---------- -------------
**    | sign | exponent | significand |
**    |   1  |    15    |     64      |
**     ------ ---------- -------------
**                       1^...........
*/
int round[5] = {1, 0, 0, 0, 0};

atoft(f, s) int *f; char *s; {
  int t[5], exp, neg, i;
  char *s1;
  s1 = s;                       /* note 1st addr for length calc */
  neg = 0;                      /* assume positive */
  pad(f, 0, 10);                /* initialize f */
  while(isspace(*s)) ++s;       /* skip leading white space */
  switch(*s) {                  /* record sign */
    case '-': neg = 1;
    case '+': ++s;
    }
  while(isdigit(*s)) {          /* integer digits */
    itoft(t, *s++ - '0');       /* convert digit to temp fp */
    ftmul10(f);                 /* multiply f by 10 */
    ftadd(f, t);                /* and add digit */
    }
  if(*s == '.') {               /* decimal point? */
    s++;  exp = 0;
    while(isdigit(*s)) {        /* fractional digits */
      itoft(t, *s++ - '0');     /* convert digit to temp fp */
      exp--;
      for(i = exp; i++; ) {
        if(ftdiv10(t) > 4) {    /* divide t by 10 and test remainder */
          round[4] = t[4];      /* set rounding exponent */
          ftadd(t, round);      /* round up by one? */
          }
        ftnormal(t);
        }
      ftadd(f, t);              /* and add it */
      }
    }
  switch(*s) {
    case 'd':    case 'D':
    case 'e':    case 'E':
      if(*++s == '+') ++s;
      s += dtoi(s, &exp);
      while(exp > 0) {exp--; ftmul10(f); ftnormal(f);}
      while(exp < 0) {exp++; ftdiv10(f); ftnormal(f);}
    }
  if(neg && f[3]) f[4] |= 0x8000;   /* set sign bit if neg and not 0 */
  return (s - s1);
  }

ftmul10(f) int *f; {
  int t[5];
  mov80(t, f);                  /* save original value */
  f[4] += 3;                    /* multiply by 8 */
  ftadd(f, t);                  /* one more */
  ftadd(f, t);                  /* one more */
  }

ftdiv10(f) int *f; {
  #asm
  mov  di,[bp+4]        ; get pointer to f
  add  di,8             ; point beyond high word of significand
  mov  dx,0             ; no initial remainder
  mov  cx,4             ; init loop counter
__ftd10a:
  push cx
  sub  di,2             ; decrement to next less significant word
  mov  ax,[di]          ; set-up DX = previous remainder, AX = new dividend
  mov  cx,10            ; set-up divisor
  div  cx               ; divide
  mov  [di],ax          ; store this part of quotient
  pop  cx
  loop __ftd10a
  mov  ax,dx            ; return remainder
  #endasm
  }

>>> ATOLB.C 492
/*
** atolb.c - Convert number of base b at s
**           to unsigned long at n
** highest base is 16
*/
atolb(s, n, b) unsigned char *s; unsigned n[], b; {
  int digit, base[2], len;
  base[0] = b;
  base[1] = len = n[0] = n[1] = 0;
  while(isxdigit(digit = *s++)) {
    if(digit >= 'a')      digit -= 87;
    else if(digit >= 'A') digit -= 55;
    else                  digit -= '0';
    if(digit >= b) break;
    mul32(n, base);
    inc32(n, digit);
    ++len;
    }
  return (len);
  }

>>> ATOP.C 1502
#include <stdio.h>
/*
** convert ASCII string (s) to packed decimal (p)
**
** return length of ASCII field used
**
** ASCII STRING
**
**    [whitespace] [sign] [digits]
**
** PACKED FORMAT
**
**       9     8     7     6     5     4     3     2     1     0
**     ----- ----- ----- ----- ----- ----- ----- ----- ----- ----- 
**    | sgn | 9 9 | 9 9 | 9 9 | 9 9 | 9 9 | 9 9 | 9 9 | 9 9 | 9 9 |
**     ----- ----- ----- ----- ----- ----- ----- ----- ----- ----- 
**    + = 00
**    - = 80
*/
atop(p, s) unsigned char *p, *s; {
  unsigned char *s1, *s2, *s3;
  int i, odd;
  s1 = s;                         /* note beginning */
  pad(p, 0, 10);                  /* initialize p (positive) */
  while(isspace(*s)) ++s;         /* skip leading white space */
  switch(*s) {
    case '-': p[9] = 0x80;        /* mark negative */
    case '+': ++s;                /* bypass sign char */
    }
  s2 = s;                         /* note first digit */
  while(isdigit(*s)) ++s;         /* set end of source field */
  s3 = s;                         /* note end */
  i = 0;  odd = YES;
  while(s2 <= --s) {              /* another source digit? */
    if(odd) {                     /* odd digit */
      p[i] = *s & 0x0F;           /* store in low nibble */
      odd = NO;
      }
    else {                        /* even digit */
      p[i] |= *s << 4;            /* store in high nibble */
      if(++i > 8) break;          /* end of packed field? */
      odd = YES;
      }
    }
  return (s3 - s1);
  }

>>> CMP32.C 369
/*
** compare signed 32-bit values x and y
*/
cmp32(x, y) int x[], y[]; {
  if(x[1] == y[1])
       if(x[0] == y[0])   return ( 0);
       else {
            unsigned xx;
            xx = x[0];
            if(xx < y[0]) return (-1);
            else          return ( 1);
            }
  else if(x[1] < y[1])    return (-1);
       else               return ( 1);
  }

>>> CMP64.C 299
/*
** compare unsigned 64-bit values x and y
*/
cmp64(x, y) unsigned x[], y[]; {
  if(x[3] != y[3]) return (x[3] > y[3] ? 1 : -1);
  if(x[2] != y[2]) return (x[2] > y[2] ? 1 : -1);
  if(x[1] != y[1]) return (x[1] > y[1] ? 1 : -1);
  if(x[0] != y[0]) return (x[0] > y[0] ? 1 : -1);
  return (0);
  }
>>> CPL32.C 86
/*
** 32 bit ~x to x
*/
cpl32(x) unsigned x[]; {
  x[0] = ~x[0];
  x[1] = ~x[1];
  }

>>> DIV32.C 1588
/*
** 32 bit x / y -- quotient to x, remainder to y
*/
div32(x, y) unsigned x[], y[]; {
  int d[4], t[4];
  d[0] = x[0];
  d[1] = x[1];
  #asm
  MOV   DI,[BP+6]    ; locate x -->  remainder
  MOV   SI,[BP+4]    ; locate y - divisor
  LEA   BX,[BP-8]    ; locate d - dividend --> quotient
;
; algorithm from Christopher L. Morgan's "Bluebook ..."
;
  PUSH DI
  LEA  DI,[BP-16]      ; locate t - temporary divisor
  MOV  CX,2
  CLD
  REP  MOVSW
  XOR  AX,AX
  MOV  CX,2
  REP  STOSW
  POP  DI
  LEA  SI,[BP-16]      ; locate t - temporary divisor
  MOV  CX,1
__1:
  TEST [BP-10],8000H   ; MSB of t
  JNZ  __2
  CALL __dsal
  INC  CX
  JMP  __1
__2:
  CALL __cmp
  JA   __3
  CALL __sub
  STC
  JMP  __4
__3:
  CLC
__4:
  CALL __qsal
  CALL __dslr
  LOOP __2
  JMP  __exit

__cmp:
  PUSH SI
  PUSH DI
  PUSH CX
  STD
  ADD  SI,6
  ADD  DI,6
  MOV  CX,4
  REP  CMPSW
  POP  CX
  POP  DI
  POP  SI
  RET

__sub:
  PUSH SI
  PUSH DI
  PUSH CX
  CLC
  MOV  CX,4
__sub1:
  MOV  AX,[SI]
  INC  SI
  INC  SI
  SBB  [DI],AX
  INC  DI
  INC  DI
  LOOP __sub1
  POP  CX
  POP  DI
  POP  SI
  RET

__qsal:
  PUSH BX
  PUSH CX
  MOV  CX,2
__qsal1:
  RCL  WORD PTR [BX],1
  INC  BX
  INC  BX
  LOOP __qsal1
  POP  CX
  POP  BX
  RET

__dsal:
  PUSH SI
  PUSH CX
  MOV  CX,4
__dsal1:
  RCL  WORD PTR [SI],1
  INC  SI
  INC  SI
  LOOP __dsal1
  POP  CX
  POP  SI
  RET 

__dslr:
  PUSH SI
  PUSH CX
  ADD  SI,6
  MOV  CX,4
  CLC
__dslr1:
  RCR  WORD PTR [SI],1
  DEC  SI
  DEC  SI
  LOOP __dslr1
  POP  CX
  POP  SI
  RET 

__exit:
  #endasm
  y[0] = x[0];  y[1] = x[1];
  x[0] = d[0];  x[1] = d[1];
  }


>>> EQ32.C 182
/*
** 32 bit x == y to x
*/
eq32(x, y) unsigned x[], y[]; {
  if(x[0] == y[0]
  && x[1] == y[1]) {
    x[0] = 1;
    x[1] = 0;
    }
  else {
    x[0] = 0;
    x[1] = 0;
    }
  }


>>> EXTEND.C 666
/*
** extend.c -- provide filename extension
**
** convert fn to upper case
** if fn has no extension, extend it with ext1
** if fn has an extension, require it to match ext1 or ext2
** return true if fn's original extension matches ext2, else false
*/
#include <stdio.h>
#include "asm.h"

extend(fn, ext1, ext2) char *fn, *ext1, *ext2; {
  char *cp;
  for(cp = fn; *cp; cp++) *cp = toupper(*cp);
  if(cp = strchr(fn, '.')) {
    if(lexcmp(cp, ext2) == 0)  return (YES);
    if(lexcmp(cp, ext1) == 0)  return (NO);
    error2("- Invalid Extension: ", fn);
    }
  if(strlen(fn) > MAXFN-4)  error2("- Filename Too Long: ", fn);
  strcat(fn, ext1);
  return (NO);
  }
>>> FILE.C 217
/*
** file.c -- file related functions
*/

open(name, mode) char *name, *mode; {
  int fd;
  if(fd = fopen(name, mode))  return(fd);
  cant(name);
  }

close(fd) int fd; {
  if(fclose(fd))  error("Close Error");
  }

>>> FT2D.C 990
/*
** convert temporary fp (t) to double precision fp (d)
**
** this routine truncates the significand, rather than rounding
**
** TEMPORARY
**       79   78      64 63          0
**     ------ ---------- -------------
**    | sign | exponent | significand |
**    |   1  |    15    |     64      |
**     ------ ---------- -------------
**                       1^...........
**
** DOUBLE PRECISION
**       63   62      52 51          0
**     ------ ---------- -------------
**    | sign | exponent | significand |
**    |   1  |    11    |     52      |
**     ------ ---------- -------------
*                    <1>^.............
*/
ft2d(d, t) unsigned *d, *t; {
  unsigned exp;
  if(t[4]) exp = (t[4] & 0x7FFF) - 0x3FFF + 0x3FF;
  else     exp = 0;
  d[3] = (t[4] & 0x8000) | (exp << 4) | ((t[3] >> 11) & 0xF);
  d[2] = ((t[3] & 0x7FF) << 5) | ((t[2] >> 11) & 0x1F);
  d[1] = ((t[2] & 0x7FF) << 5) | ((t[1] >> 11) & 0x1F);
  d[0] = ((t[1] & 0x7FF) << 5) | ((t[0] >> 11) & 0x1F);
  }
>>> FT2S.C 880
/*
** convert temporary fp (t) to single precision fp (s)
**
** this routine truncates the significand, rather than rounding
**
** TEMPORARY
**       79   78      64 63          0
**     ------ ---------- -------------
**    | sign | exponent | significand |
**    |   1  |    15    |     64      |
**     ------ ---------- -------------
**                       1^...........
**
** SINGLE PRECISION
**       31   30      23 22          0
**     ------ ---------- -------------
**    | sign | exponent | significand |
**    |   1  |     8    |     23      |
**     ------ ---------- -------------
*                    <1>^.............
*/
ft2s(s, t) unsigned *s, *t; {
  unsigned exp;
  if(t[4]) exp = (t[4] & 0x7FFF) - 0x3FFF + 0x7F;
  else     exp = 0;
  s[1] = (t[4] & 0x8000) | (exp << 7) | (((t[3] >> 9) & 0x3F));
  s[0] = ((t[3] & 0x1FF) << 7) | (((t[2] >> 9) & 0x3F));
  }
>>> FTADD.C 982
/*
** add temporary source fp (s) to temporary destination fp (d)
** assumes both numbers are initially normalized
** gives normalized result
** truncates lsb while aligning binary points
** places normalized sum in (d)
**
**       79   78      64 63          0
**     ------ ---------- -------------
**    | sign | exponent | significand |
**    |   1  |    15    |     64      |
**     ------ ---------- -------------
**                       1^...........
*/
ftadd(d, s) unsigned *d, *s; {
  unsigned i, t[5], x[5];  int diff;
  diff = (d[4] & 0x7FFF) - (s[4] & 0x7FFF);
  if(diff >  64) {             return;}
  if(diff < -64) {mov80(d, s); return;}
  mov80(t, s);
  while(diff > 0) {diff--; t[4]++; rsh64(t);}
  while(diff < 0) {diff++; d[4]++; rsh64(d);}
  if((d[4] & 0x8000) == (t[4] & 0x8000)) {
    if(add64(d, t)) {++d[4]; rsh64(d); d[3] |= 0x8000;}
    }
  else {
    if(cmp64(d, t) < 0) {mov80(x, d); mov80(d, t); mov80(t, x);}
    sub64(d, t);
    }
  ftnormal(d);
  }
>>> FTNORMAL.C 427
/*
** normalize the temporary fp number at (f)
**
**       79   78      64 63          0
**     ------ ---------- -------------
**    | sign | exponent | significand |
**    |   1  |    15    |     64      |
**     ------ ---------- -------------
**                       1^...........
**
**
*/
ftnormal(f) int *f; {
  if(f[3] || f[2] || f[1] || f[0])
    while((f[3] & 0x8000) == 0) { --f[4]; lsh64(f); }
  else f[4] = 0;
  }
>>> FTSUB.C 647
/*
** subtract temporary source fp (s) from temporary destination fp (d)
** assumes both numbers are initially normalized
** gives normalized result
** truncates lsb while aligning binary points
** returns normalized difference in (d)
**
**       79   78      64 63          0
**     ------ ---------- -------------
**    | sign | exponent | significand |
**    |   1  |    15    |     64      |
**     ------ ---------- -------------
**                       1^...........
*/
ftsub(d, s) unsigned *d, *s; {
  unsigned t[5];
  mov80(t, s);      /* copy s */
  t[4] ^= 0x8000;   /* reverse its sign */
  ftadd(d, t);      /* and add it to d */
  }
>>> GE32.C 201
/*
** 32 bit x >= y to x
*/
ge32(x, y) unsigned x[], y[]; {
  if( x[1] >  y[1]
  || (x[1] == y[1] && x[0] >= y[0])) {
    x[0] = 1;
    x[1] = 0;
    }
  else {
    x[0] = 0;
    x[1] = 0;
    }
  }


>>> GT32.C 199
/*
** 32 bit x > y to x
*/
gt32(x, y) unsigned x[], y[]; {
  if( x[1] >  y[1]
  || (x[1] == y[1] && x[0] > y[0])) {
    x[0] = 1;
    x[1] = 0;
    }
  else {
    x[0] = 0;
    x[1] = 0;
    }
  }


>>> INC32.C 283
/*
** increment 32 bit value at x with i
*/
inc32(x, i) unsigned x[], i; {
  #asm
  MOV BX,[BP+6]       ; locate x
  MOV AX,[BP+4]       ; fetch i
  ADD [BX],AX         ; add i to x low
  INC BX              ; bump to x high
  INC BX
  ADC WORD PTR [BX],0 ; add carry
  #endasm
  }

>>> INT.C 193
/*
** int.c -- integer manipulation functions
*/

/*
** get integer from "a"
*/
getint(a) int *a; {
  return (*a);
  }

/*
** put integer "i" at "a"
*/
putint(a, i) int *a, i; {
  *a = i;
  }

>>> ITOFT.C 830
/*
** convert integer (i) to temporary float (f)
**
**       79   78      64 63          0
**     ------ ---------- -------------
**    | sign | exponent | significand |
**    |   1  |    15    |     64      |
**     ------ ---------- -------------
**                       1^...........
*/
itoft(f, i) int *f, i; {
  if(i < 0) {            /* i is negative (and non-zero), so */
    f[4] = 0x8000;       /* set sign negative */
    i = -i;              /* set i positive */
    }
  else f[4] = 0;         /* set sign positive */
  pad(f, 0, 8);          /* initialize significnad to zero */
  if(i) {
    f[4] += 0x3FFF + 15; /* init exponent and bias it */
    while(i > 0) {
      --f[4];            /* adjust exponent */
      i <<= 1;           /* shift i */
      }
    f[3] = i;            /* load significand */
    }
  }
>>> LE32.C 201
/*
** 32 bit x <= y to x
*/
le32(x, y) unsigned x[], y[]; {
  if( x[1] <  y[1]
  || (x[1] == y[1] && x[0] <= y[0])) {
    x[0] = 1;
    x[1] = 0;
    }
  else {
    x[0] = 0;
    x[1] = 0;
    }
  }


>>> LNEG32.C 151
/*
** 32 bit !x to x
*/
lneg32(x) unsigned x[]; {
  if(x[0] == 0
  && x[1] == 0) {
    x[0] = 1;
    }
  else {
    x[0] = 0;
    x[1] = 0;
    }
  }

>>> LSH32.C 285
/*
** 32 bit x << y to x
*/
lsh32(x, y) unsigned x[], y[]; {
  if(y[1] || y[0] > 31) {
    x[1] = 0;
    x[0] = 0;
    }
  else {
    int i, bit;
    i = y[0];
    while(i--) {
      bit = x[1] & 0x8000;
      x[1] <<= 1;
      x[0] <<= 1;
      if(bit) x[1] |= 1;
      }
    }
  }


>>> LSH64.C 287
/*
** 64-bit x << 1 to x with zero fill
** return true if bit lost
*/
lsh64(x) int x[]; {
  int rv;  rv = x[3] < 0;
  x[3] = (x[3] << 1) | ((x[2] >> 15) & 1);
  x[2] = (x[2] << 1) | ((x[1] >> 15) & 1);
  x[1] = (x[1] << 1) | ((x[0] >> 15) & 1);
  x[0] = (x[0] << 1);
  return (rv);
  }

>>> LT32.C 199
/*
** 32 bit x < y to x
*/
lt32(x, y) unsigned x[], y[]; {
  if( x[1] <  y[1]
  || (x[1] == y[1] && x[0] < y[0])) {
    x[0] = 1;
    x[1] = 0;
    }
  else {
    x[0] = 0;
    x[1] = 0;
    }
  }


>>> LTOX.C 975
/*
** ltox -- converts 32-bit nbr to hex string of length sz
**         right adjusted and blank filled, returns str
**
**        if sz > 0 terminate with null byte
**        if sz = 0 find end of string
**        if sz < 0 use last byte for data
*/
ltox(nbr, str, sz) int nbr[]; char str[]; int sz;  {
  unsigned digit, offset, n[2];
  if(sz > 0) str[--sz] = 0;
  else if(sz < 0) sz = -sz;
  else while(str[sz] != 0) ++sz;
  n[0] = nbr[0];
  n[1] = nbr[1];
  while(sz) {
    digit = n[0] & 0x0F;              /* extract low-order digit */
    n[0] = (n[0] >> 4) & 0x0FFF;      /* shift right one nibble */
    n[0] |= (n[1] & 0x000F) << 12;
    n[1] = (n[1] >> 4) & 0x0FFF;
    if(digit < 10) offset = 48;       /* set ASCII base if 0-9 */
    else           offset = 55;       /* set ASCII base if A-F */
    str[--sz] = digit + offset;       /* store ASCII digit */
    if(n[0] == 0 && n[1] == 0) break; /* done? */
    }
  while(sz) str[--sz] = ' ';
  return (str);
  }

>>> MESS.C 489
/*
** mess.c -- message functions
*/
#include <stdio.h>

extern int list;

puts2(str1, str2) char *str1, *str2; {
  eput(str1);  eputn(str2);
  }

cant(str) char *str; {
  error2("- Can't Open ", str);
  }

error2(str1, str2) char *str1, *str2; {
  eput(str1);  error(str2);
  }

error(str) char *str; {
  eputn(str);  abort(10);
  }

eputn(str) char *str; {
  eput(str);  eput("\n");
  }

eput(str) char *str; {
  fputs(str, stderr);
  if(list & !iscons(stdout)) fputs(str, stdout);
  }

>>> MIT.C 1723
/*
** mit.c -- machine instruction table functions
*/
#include <stdio.h>
#include "asm.h"

extern unsigned
  mntbl[],            /* table of mnemonic indexes */
  optbl[],            /* table of operand indexes */
  mitbl[],            /* table of machine instr indexes */
  mncnt,              /* entries in mnemonic table */
  opcnt,              /* entries in operand table */
  micnt;              /* entries in machine instr table */

extern unsigned char
  mitbuf[];           /* instruction syntax buffer */

unsigned
  looks,              /* number of looks to find it */
  hashval;            /* global hash value for speed */

/*
** find mnemonic or operand
** return index of sought entry, else EOF
*/
find(str, tbl, cnt) unsigned char *str; unsigned tbl[], cnt; {
  unsigned h;
  h = hash(str, cnt);
  if(tbl[h << 1] == EOF) {
    ++looks;
    return (EOF);
    }
  do {
    ++looks;
    if(strcmp(mitbuf + tbl[h << 1], str) == 0) break;
    } while((h = tbl[(h << 1) + 1]) != EOF);
  return (h);
  }

/*
** calculate hash value
*/
hash(ptr, cnt) unsigned char *ptr; unsigned cnt; {
  hashval = 0;
  while(*ptr > ' ') {
    hashval = (hashval << 1) + toupper(*ptr++);
    }
  return (hashval % cnt);
  }

/*
** find instruction
** return index of sought entry, else EOF
*/
findi(instr) unsigned *instr; {
  unsigned *ip, h;
  h = hashi(instr);
  if(mitbl[h << 1] == EOF) {
    ++looks;
    return (EOF);
    }
  do {
    ++looks;
    ip = mitbuf + mitbl[h << 1];
    if((ip[0] == instr[0]) && (ip[1] == instr[1])) break;
    } while((h = mitbl[(h << 1) + 1]) != EOF);
  return (h);
  }

/*
** calculate instruction hash value
*/
hashi(inst) unsigned inst[]; {
  return(((inst[1] << 4) + inst[0]) % micnt);
  }

>>> MOD32.C 129
/*
** 32 bit x % y -- remainder to x and y
*/
mod32(x, y) unsigned x[], y[]; {
  div32(x, y);
  x[0] = y[0];
  x[1] = y[1];
  }

>>> MOV32.C 105
/*
** move 32 bit value at s to d
*/
mov32(d, s) unsigned d[], s[]; {
  d[0] = s[0];
  d[1] = s[1];
  }

>>> MOV80.C 149
/*
** move 80 bit value at s to d
*/
mov80(d, s) unsigned d[], s[]; {
  d[0] = s[0];
  d[1] = s[1];
  d[2] = s[2];
  d[3] = s[3];
  d[4] = s[4];
  }
>>> MUL32.C 767
/*
** 32 bit x * y to x
*/
mul32(x, y) unsigned x[], y[]; {
  int p[4];
  #asm
  MOV   SI,[BP+6]    ; locate x - 1st multiplicand
  MOV   DI,[BP+4]    ; locate y - 2nd multiplicand
  LEA   BX,[BP-8]    ; locate p --> product
;
; algorithm from Christopher L. Morgan's "Bluebook ..."
;
  PUSH BX
  XOR  AX,AX
  MOV  CX,4
  CLD
__1:
  MOV [BX],AX
  INC  BX
  INC  BX
  LOOP __1
  POP  BX
  MOV  CX,2
__2:
  PUSH CX
  MOV  DX,[SI]
  INC SI
  INC SI
  PUSH BX
  PUSH DI
  MOV  CX,2
__3:
  PUSH CX
  PUSH DX
  MOV  AX,[DI]
  INC  DI
  INC  DI
  MUL  DX
  ADD  [BX],AX
  INC  BX
  INC  BX
  ADC  [BX],DX
  POP  DX
  POP  CX
  LOOP __3
  POP  DI
  POP  BX
  INC  BX
  INC  BX
  POP  CX
  LOOP __2
  #endasm
  x[0] = p[0];
  x[1] = p[1];   /* ignore high order bits */
  }


>>> NE32.C 182
/*
** 32 bit x != y to x
*/
ne32(x, y) unsigned x[], y[]; {
  if(x[0] == y[0]
  && x[1] == y[1]) {
    x[0] = 0;
    x[1] = 0;
    }
  else {
    x[0] = 1;
    x[1] = 0;
    }
  }


>>> NEG32.C 81
/*
** 32 bit -x to x
*/
neg32(x) unsigned x[]; {
  cpl32(x);
  inc32(x, 1);
  }

>>> OR32.C 97
/*
** 32 bit x | y to x
*/
or32(x, y) unsigned x[], y[]; {
  x[0] |= y[0];
  x[1] |= y[1];
  }


>>> PUTOBJ.C 11488
/*
** putobj.c -- Generate OBJ Records
**
** Note: Many OBJ records contain repeating groups.  These have three
** functions to write a record:
** 
**     begXXXXXX() - begins (opens) the record - call once
**     putXXXXXX() - puts out one repetition   - call repeatedly
**     endXXXXXX() - ends (closes) the record  - call once
**
** Don't worry about record overflow.  The putXXXXXX() functions
** detect this and automatically close the open record and open
** a new one.  Only one OBJ record may be open at any given time.
**
** Records that do not have repeating groups have only a putXXXXXX()
** function which is called once to write the record.
**
** putFIXUPP and putTHREAD are special cases.  They only buffer
** data which is output when the currently open LEDATA or LIDATA
** record is closed.  Therefore, they can (and must) be called while
** one of those records is open.
*/
#include <stdio.h>
#include "obj1.h"
#include "obj2.h"
#include "obj3.h"

unsigned char
  *fbuf,               /* fixup buffer */
  *fnext,              /* next position in fixup buffer */
  *fend,               /* end of fixup buffer */
  *obuf;               /* output buffer */
unsigned
  currec,              /* current data record type */
  fixuse,              /* current data record's fixup use value */
  o1,                  /* current record's 1st arg */
  o2,                  /* current record's 2nd arg */
  o3,                  /* current record's 3rd arg */
  o4,                  /* current record's 4th arg */
  off[2],              /* LEDATA running offset */
  onext = 3,           /* output buffer data length */
  opref;               /* output buffer prefix length */

extern int
  ofd;                 /* output file descriptor */

/*
** write COMENT record to OBJ file
*/
putCOMENT(com, np, nl, class) char *com; int np, nl, class; {
  oopen(COMENT);
  obyte(((np << 1) | nl) << 6);
  obyte(class);
  oname(com, NO);
  oclose();
  }

/*
** write THEADR record to OBJ file
*/
putTHEADR(title) char *title; {
  if(obuf == 0) {
    if(!(obuf = malloc(OSIZE))
    || !(fbuf = malloc(FSIZE))) exit(1);
    fend = fbuf + FSIZE;
    }
  oopen(THEADR);
  oname(title, NO);
  oclose();
  }

/*
** write LNAMES record to OBJ file
*/
begLNAMES() {
  oopen(LNAMES);
  }
putLNAMES(name) char *name; {
  ocheck(LNAMES);
  if(OFREE < 32) {  /* another won't fit */
    oclose();       /* so close this record */
    oopen(LNAMES);  /* and start another one */
    }
  oname(name, YES);
  }
endLNAMES() {
  oclose();
  }

/*
** write SEGDEF record to OBJ file
*/
putSEGDEF(use, align, comb, big, frame, offset, len, snx, cnx, onx)
  unsigned use, align, comb, big, frame, offset[], len[], snx, cnx, onx; {
  oopen((use == 2) ? SEGDEF : SEG386);
  obyte((align << 5)      /* attributes */
      | ( comb << 2)
      | (  big << 1));
  if(align == A_ABS) {
    oword(&frame, 2);     /* frame */
    oword(offset, use);   /* offset */
    }
  oword(len, use);        /* length */
  oindex(snx);            /* segment name index */
  oindex(cnx);            /* class name index */
  oindex(onx);            /* overlay name index */
  oclose();
  }

/*
** write EXTDEF record to OBJ file
*/
begEXTDEF() {
  oopen(EXTDEF);
  }
putEXTDEF(name) char *name; {
  ocheck(EXTDEF);
  if(OFREE < 34) {  /* another won't fit */
    oclose();       /* so close this record */
    oopen(EXTDEF);  /* and start another one */
    }
  oname(name, NO);
  oindex(0);                      /* Type unspecified */
  }
endEXTDEF() {
  oclose();
  }

/*
** write GRPDEF record to OBJ file
*/
begGRPDEF(gnx) int gnx; {
  oopen(GRPDEF);
  oindex(o1 = gnx);
  }
putGRPDEF(segx) int segx; {
  ocheck(GRPDEF);
  if(OFREE < 3) {   /* another won't fit */
    oclose();       /* so close this record */
    begGRPDEF(o1);  /* and start another one */
    }
  obyte(0xFF);
  oindex(segx);
  }
endGRPDEF() {
  oclose();
  }

/*
** write PUBDEF record to OBJ file
*/
begPUBDEF(use, grpx, segx, frame) unsigned use, grpx, segx, frame; {
  oopen((use == 2) ? PUBDEF : PUB386);
  o1 = use;
  oindex(o2 = grpx);
  oindex(o3 = segx);
  o4 = frame;
  if(segx == 0) oword(&frame, 2);
  }
putPUBDEF(name, offset) char *name; unsigned offset[]; {
  ocheck((o1 == 2) ? PUBDEF : PUB386);
  if(OFREE < 38) {             /* another won't fit */
    oclose();                  /* so close this record */
    begPUBDEF(o1, o2, o3, o4); /* and start another one */
    }
  oname(name, NO);
  oword(offset, o1);
  oindex(0);
  }
endPUBDEF() {
  oclose();
  }

/*
** write LIDATA record to OBJ file
*/
begLIDATA(duse, fuse, segx, offset) unsigned duse, fuse, segx, offset[]; {
  begDATA(duse, fuse, segx, offset, (duse == 2) ? LIDATA : LID386);
  }
putLIDATA(rep, data, len) unsigned rep, len; char *data; {
  int zero; zero = 0;
  ocheck((o1 == 2) ? LIDATA : LID386);
  if(OFREE < (len + 5)) {          /* another won't fit */
    endDATA();                             /* so close this record */
    begDATA(o1, fixuse, o2, off, LIDATA);  /* and start another one */
    }
  oword(&rep, 2); if(o1 > 2) oword(&zero, 2);
  oword(&zero, 2);
  obyte(len);
  while(len--) obyte(*data++);
  inc32(off, rep*len);      /* bump running offset */
  }
endLIDATA() {
  endDATA();
  }

/*
** write LEDATA record to OBJ file
*/
begLEDATA(duse, fuse, segx, offset) unsigned duse, fuse, segx, offset[]; {
  begDATA(duse, fuse, segx, offset, (duse == 2) ? LEDATA : LED386);
  }
putLEDATA(data, len) char *data; unsigned len; {
  ocheck((o1 == 2) ? LEDATA : LED386);
  if(OFREE < len && len <= o1) {      /* may FIXUP, so don't span records */
    endDATA();                             /* close this record */
    begDATA(o1, fixuse, o2, off, LEDATA);  /* and start another one */
    }
  while(len--) {
    if(OFREE < 1) {                         /* another byte won't fit so */
      endDATA();                            /* close this record */
      begDATA(o1, fixuse, o2, off, LEDATA); /* and start another one */
      }
    obyte(*data++);
    inc32(off, 1);                          /* bump running offset */
    }
  }
endLEDATA() {
  endDATA();
  }

/*
** begin either type DATA record
*/
begDATA(duse, fuse, segx, offset, rec)
  unsigned duse, fuse, segx, offset[], rec; {
  oopen(rec);
  o1 = duse;
  oindex(o2 = segx);
  off[0] = offset[0];
  if(duse > 2) off[1] = offset[1];
  else         off[1] = 0;
  oword(off, duse);
  opref = onext;        /* note length of prefix */
  fnext = fbuf;         /* init fixup buffer */
  currec = rec;         /* note current rec type */
  fixuse = fuse;        /* note current fixup use value */
  }

/*
** end either type DATA record
*/
endDATA() {
  unsigned len, fix;
  unsigned char *f;
  oclose();
  if(fbuf < fnext) {
    fix = (fixuse == 2)           /* set current fixup record type */
        ? FIXUPP : FIX386;
    oopen(fix);                   /* open fixup record */
    f = fbuf;
    while(f < fnext) {            /* write bufferred fixups */
      len = *f++;
      if(OFREE < len) {           /* another won't fit */
        oclose();                 /* so close this record */
        oopen(fix);               /* and start another one */
        }
      while(--len) obyte(*f++);   /* decr first since len is 1 high */
      }
    oclose();                     /* close final fixup record */
    }
  }

/*
** put a fixup TRHEAD entry in the fixup buffer
*/
putTHREAD(thr, mth, ndx) int thr, mth, ndx; {
  unsigned char *len;
  if((fend - fnext) < 4) {        /* another may not fit */
    endDATA();                    /* so close & fixup current data rec */
    begDATA(o1, fixuse, o2, off, currec); /* and start another one */
    }
  len = fnext++;                  /* skip and note length byte */
  fbyte((mth << 2) |  thr);  /* TRD DAT byte */
  if((mth & 7) < 3) findex(ndx);
  *len = fnext - len;             /* set length of entry */
  }

/*
** write LEDATA item and put a fixup FIXUPP entry in the fixup buffer
*/
putLEDFIX(dat, len, use, mod, loc, fra, fdat, tar, tdat, tdis)
  char dat[];
  int len, use, mod, loc, fra, fdat, tar, tdat, tdis[]; {
  unsigned char *sz, *offset;
  if((fend - fnext) < 12) {       /* another may not fit */
    endDATA();                    /* so close & fixup this rec */
    begDATA(o1, fixuse, o2, off, currec); /* and start another one */
    }
  putLEDATA(dat, len);            /* may overflow too */
  offset = onext - len - opref;   /* now calc obj rec offset */
  sz = fnext++;                   /* skip and note size byte */
  fbyte(0x80                      /* 1st LOCAT Byte */
      | (mod << 6)
      | (loc << 2)
      |((offset >> 8) & 3));
  fbyte(offset);                  /* 2nd LOCAT Byte */
  fbyte((fra << 4) |  tar);       /* FIX DAT Byte */
  if(fra < 3) findex(fdat);
  if(tar < 8) findex(tdat);
  if(tar < 4) fword(tdis, use);
  *sz = fnext - sz;               /* set size of entry */
  }

/*
** write MODEND record to OBJ file
** (use FIXUPP parameters for "fra" and "tar")
*/
putMODEND(use, attr, fra, fdat, tar, tdat, tdis)
      int use, attr, fra, fdat, tar, tdat, tdis[]; {
  oopen((use == 2) ? MODEND : MOD386);
  obyte((attr << 6) | 1);
  if(attr & 1) {               /* give address */
    obyte((fra << 4) |  tar);  /* END DAT Byte */
    if(fra < 3) oindex(fdat);
    if(tar < 8) oindex(tdat);
    if(tar < 4) oword(tdis, use);
    }
  oclose();
  }

/*
** open an output OBJ record
*/
oopen(type) unsigned type; {
  obuf[0] = type;              /* set record type */
  onext = opref = 3;           /* reset for new record */
  }

/*
** check that the same type of OBJ record is open
*/
ocheck(type) unsigned type; {
  if(obuf[0] != type) error("+ OBJ Record Conflict");
  }

/*
** close an output OBJ record
*/
oclose() {
  unsigned i;
  unsigned char chksum;
  putint(obuf + 1, onext - 2); /* set record length */
  chksum = i = 0;              /* calc check sum */
  while(i < onext)  chksum += obuf[i++];
  obyte(-chksum);              /* set check sum */
  write(ofd, obuf, onext);     /* write record */
  obuf[0] = 0;                 /* designate record closed */
  }

/*
** output a name field into the OBJ record
*/
oname(name, up) char name[]; int up; {
  unsigned i, first;
  i = 0;
  first = onext++;                /* make room for length */
  while(name[i] && i < 127)       /* copy name */
    obyte(up ? toupper(name[i++]) : name[i++]);
  obuf[first] = i;                /* store length */
  }

/*
** output an index field into the OBJ record
*/
findex(ndx) unsigned ndx; {
  if(ndx > 127)                 /* two byte index */
    fbyte((ndx >> 8) | 0x80);   /* high 7 bits first */
  fbyte(ndx);                   /* low (or only) 8 bits */
  }

/*
** output a word field (of any length) into the OBJ record
*/
fword(word, sz) char word[]; int sz; {
  int i;  i = 0;
  while(i < sz) fbyte(word[i++]);
  }

/*
** output a byte field into the OBJ record
*/
fbyte(byte) char byte; {
  *fnext++ = byte;
  }

/*
** output an index field into the OBJ record
** return index length
*/
oindex(ndx) unsigned ndx; {
  if(ndx > 127)                 /* two byte index */
    obyte((ndx >> 8) | 0x80);   /* high 7 bits first */
  obyte(ndx);                   /* low (or only) 8 bits */
  }

/*
** output a word field (of any length) into the OBJ record
*/
oword(word, sz) char word[]; int sz; {
  int i;  i = 0;
  while(i < sz) obyte(word[i++]);
  }

/*
** output a byte field into the OBJ record
*/
obyte(byte) char byte; {
  if(onext < OSIZE) obuf[onext++] = byte;
  else error("+ obuf[] overflow");
  }

>>> REQ.C 900
/*
** req.c -- request user input
*/
#include <stdio.h>

/*
** request number
*/
reqnbr(prompt, nbr) char prompt[]; int *nbr; {
  char str[20];
  int sz;
  if(iscons(stdin)) {
    puts("");
    fputs(prompt, stdout);
    }
  getstr(str, 20);
  if((sz = utoi(str, nbr)) < 0 || str[sz])  return(NO);
  return(YES);
  }

/*
** request string
*/
reqstr(prompt, str, sz) char prompt[], *str; int sz; {
  if(iscons(stdin)) {
    puts("");
    fputs(prompt, stdout);
    }
  getstr(str, sz);
  return(*str);       /* null name returns false */
  }

/*
** get string from user
*/
getstr(str, sz) char *str; int sz; {
  char *cp;
  fgets(str, sz, stdin);
  if(iscons(stdin) && !iscons(stdout))  fputs(str, stdout);   /* echo */
  cp = str;
  while(*cp) {                       /* trim ctl chars & make uc */
    if(*cp == '\n')  break;
    if(isprint(*str = toupper(*cp++)))  ++str;
    }
  *str = NULL;
  }

>>> RSH32.C 319
/*
** 32-bit x >> y to x
*/
rsh32(x, y) unsigned x[], y[]; {
  if(y[1] || y[0] > 31) {
    x[1] = 0;
    x[0] = 0;
    }
  else {
    int i, bit;
    i = y[0];
    while(i--) {
      bit = x[1] & 1;
      x[1] = (x[1] >> 1) & 0x7FFF;
      x[0] = (x[0] >> 1) & 0x7FFF;
      if(bit) x[0] |= 0x8000;
      }
    }
  }


>>> RSH64.C 312
/*
** 64-bit x >> 1 to x with zero fill
** return true if bit lost
*/
rsh64(x) int x[]; {
  int rv;  rv = x[0] & 1;
  x[0] = ((x[0] >> 1) & 0x7FFF) | (x[1] << 15);
  x[1] = ((x[1] >> 1) & 0x7FFF) | (x[2] << 15);
  x[2] = ((x[2] >> 1) & 0x7FFF) | (x[3] << 15);
  x[3] = ((x[3] >> 1) & 0x7FFF);
  return (rv);
  }
>>> SCAN.C 730
/*
** scan.c -- scanning functions
*/
#include <stdio.h>
#include "asm.h"

/*
** is ch at end of line?
*/
atend(ch) int ch; {
  switch(ch) {
    case COMMENT:
    case    NULL:
    case    '\n': return (YES);
    }
  return (NO);
  }

/*
** are fields s and t the same?
*/
same(s, t) char *s, *t; {
  while(lexorder(*s, *t) == 0) {
    if(!isgraph(*s)) return (YES);
    ++s; ++t;
    }
  if((isspace(*s) || *s == '(' || atend(*s))
  && (isspace(*t) || *t == '(' || atend(*t))) return (YES);
  return (NO);
  }

/*
** find nth white-space-separated field in str
*/
skip(n, str) int n; char *str; {
  loop:
  while(*str && isspace(*str)) ++str;
  if(--n) {
    while(isgraph(*str)) ++str;
    goto loop;
    }
  return (str);
  }

>>> SUB32.C 404
/*
** 32 bit x - y to x
*/
sub32(x, y) unsigned x[], y[]; {
  #asm
  MOV  BX,[BP+4]       ; locate y low
  MOV  AX,[BX]         ; fetch y low
  XCHG CX,BX           ; save address
  MOV  BX,[BP+6]       ; locate x low
  SUB  [BX],AX         ; sub y low from x low
  XCHG CX,BX
  MOV  AX,[BX+2]       ; fetch y high
  XCHG CX,BX
  SBB  [BX+2],AX       ; sub y high from x high with borrow
  #endasm
  }


>>> SUB64.C 893
/*
** 64-bit x - y to x
** return true if borrowed into high order bit
*/
sub64(x, y) unsigned x[], y[]; {
  #asm
  MOV  BX,[BP+4]       ; locate y
  MOV  AX,[BX]         ; fetch y[0]
  XCHG CX,BX           ; save address
  MOV  BX,[BP+6]       ; locate x
  SUB  [BX],AX         ; subtract y[0] to x[0]
  XCHG CX,BX           ; locate y
  MOV  AX,[BX+2]       ; fetch y[1]
  XCHG CX,BX           ; locate x
  SBB  [BX+2],AX       ; subtract y[1] to x[1] with carry
  XCHG CX,BX           ; locate y
  MOV  AX,[BX+4]       ; fetch y[2]
  XCHG CX,BX           ; locate x
  SBB  [BX+4],AX       ; subtract y[2] to x[2] with carry
  XCHG CX,BX           ; locate y
  MOV  AX,[BX+6]       ; fetch y[3]
  XCHG CX,BX           ; locate x
  SBB  [BX+6],AX       ; subtract y[3] to x[3] with carry
  RCL  AX,1            ; fetch CF
  AND  AX,1            ; strip other bits and return it
  #endasm
  }
>>> WAITING.C 136
/*
** waiting.c -- wait for operator response
*/
#include <stdio.h>

waiting() {
  fputs("\nWaiting...", stderr);
  fgetc(stderr);
  }

>>> XOR32.C 98
/*
** 32 bit x ^ y to x
*/
xor32(x, y) unsigned x[], y[]; {
  x[0] ^= y[0];
  x[1] ^= y[1];
  }


