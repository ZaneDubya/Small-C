/*
** int.c -- integer manipulation functions
*/

/*
** get integer from "a"
*/
getint(a) int *a; {
  return (*a);
  }

/*
** put integer "i" at "a"
*/
putint(a, i) int *a, i; {
  *a = i;
  }

