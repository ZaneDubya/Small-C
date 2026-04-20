#include "stdio.h"
/*
** Memory allocation of size bytes.
** size  = Size of the block in bytes.
** Returns the address of the allocated block,
** else NULL for failure.
*/
char *malloc(unsigned size) {
  return (_alloc(size, NO));
  }

