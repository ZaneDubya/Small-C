/*
** Place n occurrences of ch at dest.
*/
void pad(char *dest, unsigned ch, unsigned n) {
  while(n--) *dest++ = ch;
  }

