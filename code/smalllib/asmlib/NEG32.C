/*
** 32 bit -x to x
*/
neg32(x) unsigned x[]; {
  cpl32(x);
  inc32(x, 1);
  }

