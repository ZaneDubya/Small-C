/*
** 32 bit ~x to x
*/
cpl32(x) unsigned x[]; {
  x[0] = ~x[0];
  x[1] = ~x[1];
  }

