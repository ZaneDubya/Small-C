/*
** return length of string s (fast version)
*/
int strlen(char *s) {
  #asm
  xor al,al        ; set search value to zero
  mov cx,65535     ; set huge maximum
  mov di,[bp+4]    ; get address of s
  cld              ; set direction flag forward
  ; repne prefix (0F2h)
  DB 0F2H
  scasb            ; scan for zero
  mov ax,65534
  sub ax,cx        ; calc and return length
  #endasm
  }

