/*
** file.c -- file related functions
*/

open(name, mode) char *name, *mode; {
  int fd;
  if(fd = fopen(name, mode))  return(fd);
  cant(name);
  }

close(fd) int fd; {
  if(fclose(fd))  error("Close Error");
  }

