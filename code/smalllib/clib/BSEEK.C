#include "stdio.h"
#include "clib.h"
extern int _nextc[], _bufuse[];
/*
** Position fd to the character in file indicated by "offset."
** "Offset" is the address of a long integer or array of two
** integers containing the offset, low word first.
** 
**     BASE     OFFSET-RELATIVE-TO
**       0      beginning of file
**       1      current byte in file
**       2      end of file (minus offset)
**
** Returns NULL on success, else EOF.
*/
bseek(fd, offset, base) int fd, offset[], base; {
  int hi, lo;
  if(!_mode(fd) || !_bufuse[fd]) return (EOF);
  if(_adjust(fd)) return (EOF);
  lo = offset[0];
  hi = offset[1];
  if(!_seek(base, fd, &hi, &lo)) return (EOF);
  _nextc[fd] = EOF;
  _clreof(fd);
  return (NULL);
  }

