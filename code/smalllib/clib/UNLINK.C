#include "stdio.h"
#include "clib.h"
/*
** Unlink (delete) the named file. 
** Entry: fn = Null-terminated DOS file path\name.
** Returns NULL on success, else ERR.
*/
unlink(fn) char *fn; {
  fn;           /* load fn into ax */
#asm
  mov dx,ax     ; put fn in its place
  mov ah,41h    ; delete function code
  int 21h
  mov ax,0
  jnc __unlk    ; return NULL
  mov ax,-2     ; return ERR
__unlk:
#endasm
  }
#asm
_delete: jmp    _unlink
         public _delete
#endasm

