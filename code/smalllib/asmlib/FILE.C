/*
** file.c -- file related functions
*/

int open(char *name, char *mode) {
  int fd;
  if(fd = fopen(name, mode))  return(fd);
  cant(name);
  }

void close(int fd) {
  if(fclose(fd))  error("Close Error");
  }

