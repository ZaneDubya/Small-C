#include "stdio.h"
/*
** Write string to standard output. 
*/
puts(string) char *string; {
  fputs(string, stdout);
  fputc('\n', stdout);
  }

