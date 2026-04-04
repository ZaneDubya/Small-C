#include "stdio.h"
/*
** Write character to standard output. 
*/
int putchar(int ch) {
  return (fputc(ch, stdout));
  }

