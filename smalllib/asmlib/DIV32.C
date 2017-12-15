/*
** 32 bit x / y -- quotient to x, remainder to y
*/
div32(x, y) unsigned x[], y[]; {
  int d[4], t[4];
  d[0] = x[0];
  d[1] = x[1];
  #asm
  MOV   DI,[BP+6]    ; locate x -->  remainder
  MOV   SI,[BP+4]    ; locate y - divisor
  LEA   BX,[BP-8]    ; locate d - dividend --> quotient
;
; algorithm from Christopher L. Morgan's "Bluebook ..."
;
  PUSH DI
  LEA  DI,[BP-16]      ; locate t - temporary divisor
  MOV  CX,2
  CLD
  REP  MOVSW
  XOR  AX,AX
  MOV  CX,2
  REP  STOSW
  POP  DI
  LEA  SI,[BP-16]      ; locate t - temporary divisor
  MOV  CX,1
__1:
  TEST [BP-10],8000H   ; MSB of t
  JNZ  __2
  CALL __dsal
  INC  CX
  JMP  __1
__2:
  CALL __cmp
  JA   __3
  CALL __sub
  STC
  JMP  __4
__3:
  CLC
__4:
  CALL __qsal
  CALL __dslr
  LOOP __2
  JMP  __exit

__cmp:
  PUSH SI
  PUSH DI
  PUSH CX
  STD
  ADD  SI,6
  ADD  DI,6
  MOV  CX,4
  REP  CMPSW
  POP  CX
  POP  DI
  POP  SI
  RET

__sub:
  PUSH SI
  PUSH DI
  PUSH CX
  CLC
  MOV  CX,4
__sub1:
  MOV  AX,[SI]
  INC  SI
  INC  SI
  SBB  [DI],AX
  INC  DI
  INC  DI
  LOOP __sub1
  POP  CX
  POP  DI
  POP  SI
  RET

__qsal:
  PUSH BX
  PUSH CX
  MOV  CX,2
__qsal1:
  RCL  WORD PTR [BX],1
  INC  BX
  INC  BX
  LOOP __qsal1
  POP  CX
  POP  BX
  RET

__dsal:
  PUSH SI
  PUSH CX
  MOV  CX,4
__dsal1:
  RCL  WORD PTR [SI],1
  INC  SI
  INC  SI
  LOOP __dsal1
  POP  CX
  POP  SI
  RET 

__dslr:
  PUSH SI
  PUSH CX
  ADD  SI,6
  MOV  CX,4
  CLC
__dslr1:
  RCR  WORD PTR [SI],1
  DEC  SI
  DEC  SI
  LOOP __dslr1
  POP  CX
  POP  SI
  RET 

__exit:
  #endasm
  y[0] = x[0];  y[1] = x[1];
  x[0] = d[0];  x[1] = d[1];
  }


