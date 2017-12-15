/*
** compare unsigned 64-bit values x and y
*/
cmp64(x, y) unsigned x[], y[]; {
  if(x[3] != y[3]) return (x[3] > y[3] ? 1 : -1);
  if(x[2] != y[2]) return (x[2] > y[2] ? 1 : -1);
  if(x[1] != y[1]) return (x[1] > y[1] ? 1 : -1);
  if(x[0] != y[0]) return (x[0] > y[0] ? 1 : -1);
  return (0);
  }
