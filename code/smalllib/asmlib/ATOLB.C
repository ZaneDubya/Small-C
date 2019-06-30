/*
** atolb.c - Convert number of base b at s
**           to unsigned long at n
** highest base is 16
*/
atolb(s, n, b) unsigned char *s; unsigned n[], b; {
  int digit, base[2], len;
  base[0] = b;
  base[1] = len = n[0] = n[1] = 0;
  while(isxdigit(digit = *s++)) {
    if(digit >= 'a')      digit -= 87;
    else if(digit >= 'A') digit -= 55;
    else                  digit -= '0';
    if(digit >= b) break;
    mul32(n, base);
    inc32(n, digit);
    ++len;
    }
  return (len);
  }

