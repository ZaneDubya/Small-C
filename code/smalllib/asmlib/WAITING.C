/*
** waiting.c -- wait for operator response
*/
#include <stdio.h>

waiting() {
  fputs("\nWaiting...", stderr);
  fgetc(stderr);
  }

