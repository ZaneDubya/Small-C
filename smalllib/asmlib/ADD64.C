/*
** 64-bit x + y to x
** return true if carried out of high order bit
*/
add64(x, y) unsigned x[], y[]; {
  #asm
  MOV  BX,[BP+4]       ; locate y
  MOV  AX,[BX]         ; fetch y[0]
  XCHG CX,BX           ; save address
  MOV  BX,[BP+6]       ; locate x
  ADD  [BX],AX         ; add y[0] to x[0]
  XCHG CX,BX           ; locate y
  MOV  AX,[BX+2]       ; fetch y[1]
  XCHG CX,BX           ; locate x
  ADC  [BX+2],AX       ; add y[1] to x[1] with carry
  XCHG CX,BX           ; locate y
  MOV  AX,[BX+4]       ; fetch y[2]
  XCHG CX,BX           ; locate x
  ADC  [BX+4],AX       ; add y[2] to x[2] with carry
  XCHG CX,BX           ; locate y
  MOV  AX,[BX+6]       ; fetch y[3]
  XCHG CX,BX           ; locate x
  ADC  [BX+6],AX       ; add y[3] to x[3] with carry
  RCL  AX,1            ; fetch CF
  AND  AX,1            ; strip other bits and return it
  #endasm
  }


