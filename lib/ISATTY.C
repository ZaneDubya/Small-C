/*
** Return "true" if fd is a device, else "false"
*/
isatty(fd) int fd; {
fd;               /* fetch handle */
#asm
  push bx         ; save 2nd reg
  mov  bx,ax      ; place handle
  mov  ax,4400h   ; ioctl get info function
  int 21h         ; call BDOS
  pop  bx         ; restore 2nd reg
  mov  ax,dx      ; fetch info bits
  and  ax,80h     ; isdev bit
#endasm
  }


