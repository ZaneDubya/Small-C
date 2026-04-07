#include "stdio.h"
#include "clib.h"
/*
** Rename a file.
**  from = address of old filename.
**    to = address of new filename.
**  Returns NULL on success, else ERR.
*/
int rename(char *from, char *to) {
  if(_rename(from, to)) return (NULL);
  return (ERR);
  }

int _rename(char *old, char *new) {
#asm
  push ds         ; ds:dx points to old name
  pop  es         ; es:di points to new name
  mov  dx,[bp+4]  ; get "old" offset
  mov  di,[bp+6]  ; get "new" offset
  mov  ah,56h     ; rename function
  int  21h        ; call bdos
  jnc  __ren1     ; error?
  xor  ax,ax      ; yes, return false
  jmp  __ren2 
__ren1:           ; no, set hi and lo
  mov  ax,1       ; return true
__ren2:
#endasm
  }

