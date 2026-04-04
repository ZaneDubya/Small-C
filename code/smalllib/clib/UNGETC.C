#include "stdio.h"
extern _nextc[];
/*
** Put c back into file fd.
** Entry:  c = character to put back
**        fd = file descriptor
** Returns c if successful, else EOF.
*/
int ungetc(int c, int fd) {
  if(!_mode(fd) || _nextc[fd]!=EOF || c==EOF) return (EOF);
  return (_nextc[fd] = c);
  }

