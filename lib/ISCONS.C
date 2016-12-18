/*
** Determine if fd is the console.
*/
#include <stdio.h>
extern int _cons[];

iscons(fd) int fd; {
  if(_cons[fd] == NULL) {
    if(_iscons(fd)) _cons[fd] = 2;
    else            _cons[fd] = 1;
    }
  if(_cons[fd] == 1) return (NO);
  return (YES);
  }

/*
** Call DOS only the first time for a file.
*/
_iscons(fd) int fd; {
  fd;             /* fetch handle */
#asm
  push bx         ; save 2nd reg
  mov  bx,ax      ; place handle
  mov  ax,4400h   ; ioctl get info function
  int 21h         ; call BDOS
  pop  bx         ; restore 2nd reg
  mov  ax,dx      ; fetch info bits
  and  ax,83h     ; keep device and console bits
  cmp  ax,80h     ; device and console?
  jg   __cons1
  xor  ax,ax      ; return false if not device and console
__cons1:
#endasm
  }
