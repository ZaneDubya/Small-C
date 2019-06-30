/*
** 32-bit x >> y to x
*/
rsh32(x, y) unsigned x[], y[]; {
  if(y[1] || y[0] > 31) {
    x[1] = 0;
    x[0] = 0;
    }
  else {
    int i, bit;
    i = y[0];
    while(i--) {
      bit = x[1] & 1;
      x[1] = (x[1] >> 1) & 0x7FFF;
      x[0] = (x[0] >> 1) & 0x7FFF;
      if(bit) x[0] |= 0x8000;
      }
    }
  }


