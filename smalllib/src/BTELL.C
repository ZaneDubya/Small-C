#include "stdio.h"
#include "clib.h"
extern int _bufuse[];
/*
** Retrieve offset to next character in fd.
** "Offset" must be the address of a long int or
** a 2-element int array.  The offset is placed in
** offset in low, high order.
*/
btell(fd, offset) int fd, offset[]; {
  if(!_mode(fd) || !_bufuse[fd]) return (EOF);
  if(_adjust(fd)) return (EOF);
  offset[0] = offset[1] = 0;
  _seek(FROM_CUR, fd, offset+1, offset);
  return (NULL);
  }
