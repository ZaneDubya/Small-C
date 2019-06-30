#include "stdio.h"
#include "clib.h"
extern int
  _bufsiz[MAXFILES],  /* size of buffer */
  _bufptr[MAXFILES];  /* aux buffer address */

/*
** auxbuf -- allocate an auxiliary input buffer for fd
**   fd = file descriptor of an open file
** size = size of buffer to be allocated
** Returns NULL on success, else ERR.
** Note: Ungetc() still works.
**       A 2nd call allocates a new buffer replacing old one.
**       If fd is a device, buffer is allocated but ignored.
**       Buffer stays allocated when fd is closed or new one is allocated.
**       May be used on a closed fd.
*/
auxbuf(fd, size) int fd; char *size; {   /* fake unsigned */
  if(!size || avail(NO) < size) return (ERR);
  _bufptr[fd] = malloc(size);
  _bufsiz[fd] = size;
  _empty(fd, NO);
  return (NULL);
  }

