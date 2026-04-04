#include "stdio.h"
/*
** Write string to standard output. 
*/
void puts(char *string) {
  fputs(string, stdout);
  fputc('\n', stdout);
  }

