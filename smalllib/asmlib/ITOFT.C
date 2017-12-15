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
