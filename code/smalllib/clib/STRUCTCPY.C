/*
** copy n bytes from src to dst
*/
structcpy(dst, src, n) char *dst, *src; int n; {
  while (n--) *dst++ = *src++;
  }
