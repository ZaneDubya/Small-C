/*
** concatenate t to end of s 
** s must be large enough
*/
char *strcat(char *s, char *t) {
  char *d;
  d = s;
  --s;
  while (*++s) ;
  while (*s++ = *t++) ;
  return (d);
  }

