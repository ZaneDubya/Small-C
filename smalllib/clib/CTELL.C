#include "stdio.h"
#include "clib.h"
extern int _bufuse[];
/*
** Return offset to current 128-byte record.
*/
ctell(fd) int fd; {
  int hi, lo;
  if(!_mode(fd) || !_bufuse[fd]) return (-1);
  if(_adjust(fd)) return (-1);
  hi = lo = 0;
  _seek(FROM_CUR, fd, &hi, &lo);
  return ((hi << 9) | ((lo >> 7) & 511));
  }

/*
** Return offset to next byte in current 128-byte record.
*/
ctellc(fd) int fd; {
  int hi, lo;
  if(!_mode(fd) || !_bufuse[fd]) return (-1);
  if(_adjust(fd)) return (-1);
  hi = lo = 0;
  _seek(FROM_CUR, fd, &hi, &lo);
  return (lo & 127);
  }

