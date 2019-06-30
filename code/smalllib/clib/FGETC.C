#include "stdio.h"
#include "clib.h"

extern int _nextc[];

/*
** Character-stream input of one character from fd.
** Entry: fd = File descriptor of pertinent file.
** Returns the next character on success, else EOF.
*/
fgetc(fd) int fd; {
  int ch;                   /* must be int so EOF will flow through */
  if(_nextc[fd] != EOF) {   /* an ungotten byte pending? */
    ch = _nextc[fd];
    _nextc[fd] = EOF;
    return (ch & 255);      /* was cooked the first time */
    }
  while(1) {
    ch = _read(fd);
    if(iscons(fd)) {
      switch(ch) {          /* extra console cooking */
        case ABORT:  exit(2);
        case    CR: _write(CR, stderr); _write(LF, stderr); break;
        case   DEL:  ch = RUB;
        case   RUB:
        case  WIPE:  break;
        default:    _write(ch, stderr);
        }
      }
    switch(ch) {            /* normal cooking */
          default:  return (ch);
      case DOSEOF: _seteof(fd); return (EOF);
      case     CR:  return ('\n');
      case     LF:
      }
    }
  }
#asm
_getc:  jmp     _fgetc
        public  _getc
#endasm

