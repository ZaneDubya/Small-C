#include "stdio.h"
#include "clib.h"
extern int _nextc[], _bufuse[];
/*
** Position fd to the 128-byte record indicated by
** "offset" relative to the point indicated by "base."
** 
**     BASE     OFFSET-RELATIVE-TO
**       0      first record
**       1      current record
**       2      end of file (last record + 1)
**              (offset should be minus)
**
** Returns NULL on success, else EOF.
*/
cseek(fd, offset, base) int fd, offset, base; {
  int newrec, oldrec, hi, lo;
  if(!_mode(fd) || !_bufuse[fd]) return (EOF);
  if(_adjust(fd)) return (EOF);
  switch (base) {
     case 0: newrec = offset;
             break;
     case 1: oldrec = ctell(fd);
             goto calc;
     case 2: hi = lo = 0;
             if(!_seek(FROM_END, fd, &hi, &lo)) return (EOF);
             oldrec = ((lo >> 7) & 511) | (hi << 9);
     calc:
             newrec = oldrec + offset;
             break;
    default: return (EOF);
    }
  lo = (newrec << 7);       /* convert newrec to long int */
  hi = (newrec >> 9) & 127;
  if(!_seek(FROM_BEG, fd, &hi, &lo)) return (EOF);
  _nextc[fd] = EOF;
  _clreof(fd);
  return (NULL);
  }

/*
** Position fd to the character indicated by
** "offset" within current 128-byte record.
** Must be on record boundary.
**
** Returns NULL on success, else EOF.
*/
cseekc(fd, offset) int fd, offset; {
  int hi, lo;
  if(!_mode(fd) || isatty(fd) ||
     ctellc(fd) || offset < 0 || offset > 127) return (EOF);
  hi = 0; lo = offset;
  if(!_seek(FROM_CUR, fd, &hi, &lo)) return (EOF);
  return (NULL);
  }

