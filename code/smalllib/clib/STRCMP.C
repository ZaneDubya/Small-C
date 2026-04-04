/*
** return <0,   0,  >0 a_ording to
**       s<t, s=t, s>t
*/
int strcmp(char *s, char *t) {
  while(*s == *t) {
    if(*s == 0) return (0);
    ++s; ++t;
    }
  return (*s - *t);
  }

