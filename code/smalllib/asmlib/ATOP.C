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

