/*
** increment 32 bit value at x with i
*/
inc32(x, i) unsigned x[], i; {
  #asm
  MOV BX,[BP+6]       ; locate x
  MOV AX,[BP+4]       ; fetch i
  ADD [BX],AX         ; add i to x low
  INC BX              ; bump to x high
  INC BX
  ADC WORD PTR [BX],0 ; add carry
  #endasm
  }

