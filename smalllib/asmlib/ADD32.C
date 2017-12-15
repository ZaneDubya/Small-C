/*
** 32 bit x + y to x
*/
add32(x, y) unsigned x[], y[]; {
  #asm
  MOV  BX,[BP+4]       ; locate y low
  MOV  AX,[BX]         ; fetch y low
  XCHG CX,BX           ; save address
  MOV  BX,[BP+6]       ; locate x low
  ADD  [BX],AX         ; add y low to x low
  XCHG CX,BX
  MOV  AX,[BX+2]       ; fetch y high
  XCHG CX,BX
  ADC  [BX+2],AX       ; add y high with carry
  #endasm
  }


