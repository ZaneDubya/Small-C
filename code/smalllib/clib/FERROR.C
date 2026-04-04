#include "stdio.h"
#include "clib.h"
extern _status[];
/*
** Test for error status on fd.
*/
int ferror(int fd) {
  return (_status[fd] & ERRBIT);
  }

