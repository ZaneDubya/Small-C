#include "stdio.h"
#include "clib.h"
/*
** Close fd 
** Entry: fd = file descriptor for file to be closed.
** Returns NULL for success, otherwise ERR
*/
extern int _status[];
fclose(fd) int fd; {
  if(!_mode(fd) || _flush(fd)) return (ERR);
  if(_bdos2(CLOSE<<8, fd, NULL, NULL) == -6) return (ERR);
  return (_status[fd] = NULL);
  }

