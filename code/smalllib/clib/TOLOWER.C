/*
** return lower-case of c if upper-case, else c
*/
int tolower(int c) {
  if(c<='Z' && c>='A') return (c+32);
  return (c);
  }

