/*
** waiting.c -- wait for operator response
*/
#include <stdio.h>

void waiting() {
  fputs("\nWaiting...", stderr);
  fgetc(stderr);
  }

