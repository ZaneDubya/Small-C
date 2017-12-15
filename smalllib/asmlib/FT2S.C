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
