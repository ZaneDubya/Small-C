/*
** Rewind file to beginning. 
*/
int rewind(int fd) {
  return(cseek(fd, 0, 0));
  }

