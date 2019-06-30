/*
** 64-bit x >> 1 to x with zero fill
** return true if bit lost
*/
rsh64(x) int x[]; {
  int rv;  rv = x[0] & 1;
  x[0] = ((x[0] >> 1) & 0x7FFF) | (x[1] << 15);
  x[1] = ((x[1] >> 1) & 0x7FFF) | (x[2] << 15);
  x[2] = ((x[2] >> 1) & 0x7FFF) | (x[3] << 15);
  x[3] = ((x[3] >> 1) & 0x7FFF);
  return (rv);
  }
