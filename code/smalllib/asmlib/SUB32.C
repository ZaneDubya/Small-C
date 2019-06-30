/*
** 32 bit x - y to x
*/
sub32(x, y) unsigned x[], y[]; {
  #asm
  MOV  BX,[BP+4]       ; locate y low
  MOV  AX,[BX]         ; fetch y low
  XCHG CX,BX           ; save address
  MOV  BX,[BP+6]       ; locate x low
  SUB  [BX],AX         ; sub y low from x low
  XCHG CX,BX
  MOV  AX,[BX+2]       ; fetch y high
  XCHG CX,BX
  SBB  [BX+2],AX       ; sub y high from x high with borrow
  #endasm
  }


