#include "stdio.h"
#include "clib.h"
extern int _status[];
/*
** Clear error status for fd.
*/
void clearerr(int fd) {
  if(_mode(fd)) _clrerr(fd);
  }

