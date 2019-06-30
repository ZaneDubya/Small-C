#include "stdio.h"
#include "clib.h"
/*
** Poll for console input or interruption
*/
poll(pause) int pause; {
  int i;
  if(i = _hitkey())  i = _getkey();
  if(pause) {
    if(i == PAUSE) {
      i = _getkey();           /* wait for next character */
      if(i == ABORT) exit(2);  /* indicate abnormal exit */
      return (0);
      }
    if(i == ABORT) exit(2);
    }
  return (i);
  }

