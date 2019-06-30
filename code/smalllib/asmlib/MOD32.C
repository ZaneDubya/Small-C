/*
** 32 bit x % y -- remainder to x and y
*/
mod32(x, y) unsigned x[], y[]; {
  div32(x, y);
  x[0] = y[0];
  x[1] = y[1];
  }

