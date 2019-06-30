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

