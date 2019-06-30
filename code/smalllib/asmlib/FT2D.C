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
