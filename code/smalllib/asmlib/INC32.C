/*
** increment 32 bit value at x with i
*/
void inc32(unsigned x[], unsigned i) {
  #asm
  MOV BX,[BP+4]       ; locate x
  MOV AX,[BP+6]       ; fetch i
  ADD [BX],AX         ; add i to x low
  INC BX              ; bump to x high
  INC BX
  ADC WORD PTR [BX],0 ; add carry
  #endasm
  }

