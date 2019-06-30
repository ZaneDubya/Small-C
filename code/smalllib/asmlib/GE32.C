/*
** 32 bit x >= y to x
*/
ge32(x, y) unsigned x[], y[]; {
  if( x[1] >  y[1]
  || (x[1] == y[1] && x[0] >= y[0])) {
    x[0] = 1;
    x[1] = 0;
    }
  else {
    x[0] = 0;
    x[1] = 0;
    }
  }


