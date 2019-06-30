/*
** 32 bit x << y to x
*/
lsh32(x, y) unsigned x[], y[]; {
  if(y[1] || y[0] > 31) {
    x[1] = 0;
    x[0] = 0;
    }
  else {
    int i, bit;
    i = y[0];
    while(i--) {
      bit = x[1] & 0x8000;
      x[1] <<= 1;
      x[0] <<= 1;
      if(bit) x[1] |= 1;
      }
    }
  }


