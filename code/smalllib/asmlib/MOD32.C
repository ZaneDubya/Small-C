/*
** 32 bit x % y -- remainder to x and y
*/
void mod32(unsigned x[], unsigned y[]) {
  div32(x, y);
  x[0] = y[0];
  x[1] = y[1];
  }

