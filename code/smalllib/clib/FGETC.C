#include "stdio.h"
#include "clib.h"

extern int _nextc[];

/*
** Character-stream input of one character from fd.
** Entry: fd = File descriptor of pertinent file.
** Returns the next character on success, else EOF.
*/
int fgetc(int fd) {
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
      case     CR:
          if (!iscons(fd)) {
              ch = _read(fd);       /* peek: consume LF if CRLF pair */
              if (ch != LF) _nextc[fd] = ch;
          }
          return ('\n');
      case     LF:
          return ('\n');            /* standalone LF also terminates line */
      }
    }
  }
#asm
_getc:  jmp     _fgetc
        public  _getc
#endasm

