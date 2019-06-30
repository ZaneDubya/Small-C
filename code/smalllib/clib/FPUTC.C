#include "stdio.h"
#include "clib.h"
extern int _status[];
/*
** Character-stream output of a character to fd.
** Entry: ch = Character to write.
**        fd = File descriptor of perinent file.
** Returns character written on success, else EOF.
*/
fputc(ch, fd) int ch, fd; {
  switch(ch) {
    case  EOF: _write(DOSEOF, fd); break;
    case '\n': _write(CR, fd); _write(LF, fd); break;
      default: _write(ch, fd);
    }
  if(_status[fd] & ERRBIT) return (EOF);
  return (ch);
  }
#asm
_putc:  jmp     _fputc
        public  _putc
#endasm

