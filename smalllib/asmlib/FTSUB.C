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
