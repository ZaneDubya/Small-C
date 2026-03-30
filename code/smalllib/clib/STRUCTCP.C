/*
** copy n bytes from src to dst
*/
structcp(dst, src, n) char *dst, *src; int n; {
  while (n--) *dst++ = *src++;
  }
