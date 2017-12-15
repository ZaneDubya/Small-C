/*
** compare signed 32-bit values x and y
*/
cmp32(x, y) int x[], y[]; {
  if(x[1] == y[1])
       if(x[0] == y[0])   return ( 0);
       else {
            unsigned xx;
            xx = x[0];
            if(xx < y[0]) return (-1);
            else          return ( 1);
            }
  else if(x[1] < y[1])    return (-1);
       else               return ( 1);
  }

