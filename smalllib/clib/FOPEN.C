#include "stdio.h"
#include "clib.h"
/*
** Open file indicated by fn.
** Entry: fn   = Null-terminated DOS file name.
**        mode = "a"  - append
**               "r"  - read
**               "w"  - write
**               "a+" - append update
**               "r+" - read   update
**               "w+" - write  update
** Returns a file descriptor on success, else NULL.
*/
fopen(fn, mode) char *fn, *mode; {
  int fd;
  if(!_open(fn, mode, &fd)) return (NULL);
  return (fd);
  }

