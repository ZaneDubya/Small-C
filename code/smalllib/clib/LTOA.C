/*
** ltoa(n, s) - Convert signed 32-bit long to decimal string.
** Returns pointer to s.
*/
int ltoa(long n, char *s) {
  int lo, hi, rem, sign;
  char *ptr;
  int *p;
  ptr = s;
  p = &n;
  lo = *p; hi = *(p+1);
  sign = 0;
  if (hi & 0x8000) {
    sign = 1;
    lo = -lo;
    hi = ~hi;
    if (lo == 0) hi = hi + 1;
  }
  if (hi == 0 && lo == 0) {
    *ptr++ = '0';
    *ptr = 0;
    return (s);
  }
  while (hi || lo) {
    rem = udiv3216(&lo, &hi, 10);
    *ptr++ = rem + '0';
  }
  if (sign) *ptr++ = '-';
  *ptr = 0;
  reverse(s);
  return (s);
}
