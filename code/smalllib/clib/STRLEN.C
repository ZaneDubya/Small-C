/*
** return length of string s (fast version)
*/
int strlen(char *s) {
  #asm
  push es
  push di
  mov  ax, ds      ; ES must equal DS for near-pointer scan;
  mov  es, ax      ; BIOS INT 10h (and others) clobber ES
  xor  al, al      ; set search value to zero
  mov  cx, 65535   ; set huge maximum
  mov  di, [bp+4]  ; get address of s
  cld              ; set direction flag forward
  ; repne prefix
  DB 0F2H          
  scasb            ; scan ES:[DI] for zero
  mov  ax, 65534
  sub  ax, cx      ; calc and return length
  pop  di
  pop  es
  #endasm
  }

