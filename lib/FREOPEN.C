#include <stdio.h>
#include "clib.h"
/*
** Close previously opened fd and reopen it. 
** Entry: fn   = Null-terminated DOS file name.
**        mode = "a"  - append
**               "r"  - read
**               "w"  - write
**               "a+" - append update
**               "r+" - read   update
**               "w+" - write  update
**        fd   = File descriptor of pertinent file.
** Returns the original fd on success, else NULL.
*/
extern int _status[];
freopen(fn, mode, fd) char *fn, *mode; int fd; {
  int tfd;
  if(fclose(fd)) return (NULL);
  if(!_open(fn, mode, &tfd)) return (NULL);
  if(fd != tfd) {
    if(_bdos2(FORCE<<8, tfd, fd, NULL) < 0) return (NULL);
    _status[fd] = _status[tfd];
    _status[tfd] = 0;       /* leaves DOS using two handles */
    }
  return (fd);
  }

