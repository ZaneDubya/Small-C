/*
** return pointer to 1st occurrence of c in str, else 0
*/
char *strchr(char *str, char c) {
  while(*str) {
    if(*str == c) return (str);
    ++str;
    }
  return (0);
  }

