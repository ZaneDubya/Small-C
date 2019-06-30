/*
** 32 bit x * y to x
*/
mul32(x, y) unsigned x[], y[]; {
  int p[4];
  #asm
  MOV   SI,[BP+6]    ; locate x - 1st multiplicand
  MOV   DI,[BP+4]    ; locate y - 2nd multiplicand
  LEA   BX,[BP-8]    ; locate p --> product
;
; algorithm from Christopher L. Morgan's "Bluebook ..."
;
  PUSH BX
  XOR  AX,AX
  MOV  CX,4
  CLD
__1:
  MOV [BX],AX
  INC  BX
  INC  BX
  LOOP __1
  POP  BX
  MOV  CX,2
__2:
  PUSH CX
  MOV  DX,[SI]
  INC SI
  INC SI
  PUSH BX
  PUSH DI
  MOV  CX,2
__3:
  PUSH CX
  PUSH DX
  MOV  AX,[DI]
  INC  DI
  INC  DI
  MUL  DX
  ADD  [BX],AX
  INC  BX
  INC  BX
  ADC  [BX],DX
  POP  DX
  POP  CX
  LOOP __3
  POP  DI
  POP  BX
  INC  BX
  INC  BX
  POP  CX
  LOOP __2
  #endasm
  x[0] = p[0];
  x[1] = p[1];   /* ignore high order bits */
  }


