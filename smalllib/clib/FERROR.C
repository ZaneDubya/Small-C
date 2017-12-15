#include "stdio.h"
#include "clib.h"
extern _status[];
/*
** Test for error status on fd.
*/
ferror(fd) int fd; {
  return (_status[fd] & ERRBIT);
  }

