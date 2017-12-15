#include "stdio.h"
#include "clib.h"
/*
** Close all open files and exit to DOS. 
** Entry: ec = exit code.
*/
exit(ec) int ec; {
  int fd;  char str[4];
  ec &= 255;
  if(ec) {
    left(itou(ec, str, 4));
    fputs("Exit Code: ", stderr);
    fputs(str, stderr);
    fputs("\n", stderr);
    }
  for(fd = 0; fd < MAXFILES; ++fd) fclose(fd);
  _bdos2((RETDOS<<8)|ec, NULL, NULL, NULL);
#asm
_abort: jmp    _exit
        public _abort
#endasm
  }

