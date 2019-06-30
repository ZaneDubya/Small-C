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
