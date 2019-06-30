/*
** 64-bit x << 1 to x with zero fill
** return true if bit lost
*/
lsh64(x) int x[]; {
  int rv;  rv = x[3] < 0;
  x[3] = (x[3] << 1) | ((x[2] >> 15) & 1);
  x[2] = (x[2] << 1) | ((x[1] >> 15) & 1);
  x[1] = (x[1] << 1) | ((x[0] >> 15) & 1);
  x[0] = (x[0] << 1);
  return (rv);
  }

