#include <stdio.h>

main() {
  unsigned int x[2], y[2];
  x[0] = 4000;
  x[1] = 20;
  y[0] = 47;
  y[1] = 1;
  printf("%u %u %u %u\n", x[0], x[1], y[0], y[1]);
  div32(x, y);
  printf("%u %u %u %u\n", x[0], x[1], y[0], y[1]);
  return;
}

/*
** 32 bit n / d -- quotient to n, remainder to d
*/
div32(unsigned n[], unsigned d[]) {
  unsigned q[2], r[2];
  unsigned i, b;
  if (d[1] == 0 && d[0] == 0) {
    printf("Error in div32: Divide by zero.\n");
    abort(1);
  }
  q[0] = q[1] = 0;
  r[0] = r[1] = 0;
  for (i = 31; ; i--) {
    xhiftl32(r);
    b = 0x0001 << (i % 16);
    if ((n[i / 16] & b) != 0) {
      // printf("n[%u] & %x != 0\n", i / 16, b);
      r[0] |= 0x0001;
    }
    if (xmp32(r, d) >= 0) {
      xub32(r, d);
      q[i / 16] |= b;
    }
    if (i == 0) {
      break;
    }
  }
  n[1] = q[1];
  n[0] = q[0];
  d[1] = r[1];
  d[0] = r[0];
}

// 32 bit x - y to x
xub32(unsigned x[], unsigned y[]) {
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

// 32 bit compare. returns 1 if a > b, 0 if a == b, -1 if a < b
xmp32(unsigned a[], unsigned b[]) {
  if (a[1] > b[1]) {
    return 1;
  }
  if (a[1] < b[1]) {
    return -1;
  }
  if (a[0] > b[0]) {
    return 1;
  }
  if (a[0] < b[0]) {
    return -1;
  }
  return 0;
}

// 32 bit shift a left by 1 bit
xhiftl32(unsigned a[]) {
  a[1] = a[1] << 1;
  if (a[0] & 0x8000) {
    a[1] |= 0x0001;
  }
  a[0] = a[0] << 1;
}