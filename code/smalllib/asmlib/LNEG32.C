/*
** 32 bit !x to x
*/
lneg32(x) unsigned x[]; {
  if(x[0] == 0
  && x[1] == 0) {
    x[0] = 1;
    }
  else {
    x[0] = 0;
    x[1] = 0;
    }
  }

