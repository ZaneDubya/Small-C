/*
** ltoab(n, s, b) - Convert unsigned 32-bit long to string in base b.
** Supports bases 2, 8, 10, 16.
** Returns pointer to s.
*/
int ltoab(long n, char *s, int b) {
  int lo, hi, rem;
  char *ptr;
  int *p;
  ptr = s;
  p = &n;
  lo = *p; hi = *(p+1);
  if (hi == 0 && lo == 0) {
    *ptr++ = '0';
    *ptr = 0;
    return (s);
  }
  while (hi || lo) {
    rem = udiv3216(&lo, &hi, b);
    if (rem < 10) *ptr = rem + '0';
    else *ptr = rem + 55;
    ++ptr;
  }
  *ptr = 0;
  reverse(s);
  return (s);
}
