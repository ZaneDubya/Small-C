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
