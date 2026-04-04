/*
** copy n bytes from src to dst
*/
void structcp(char *dst, char *src, int n) {
  while (n--) *dst++ = *src++;
  }
