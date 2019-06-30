#include "stdio.h"
#include "clib.h"
/*
** Write a string to fd. 
** Entry: string = Pointer to null-terminated string.
**        fd     = File descriptor of pertinent file.
*/
fputs(string, fd) char *string; int fd; {
  while(*string) fputc(*string++, fd) ;
  }

