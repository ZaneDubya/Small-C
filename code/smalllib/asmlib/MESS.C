/*
** mess.c -- message functions
*/
#include <stdio.h>

extern int list;

void puts2(char *str1, char *str2) {
  eput(str1);  eputn(str2);
  }

void cant(char *str) {
  error2("- Can't Open ", str);
  }

void error2(char *str1, char *str2) {
  eput(str1);  error(str2);
  }

void error(char *str) {
  eputn(str);  abort(10);
  }

void eputn(char *str) {
  eput(str);  eput("\n");
  }

void eput(char *str) {
  fputs(str, stderr);
  if(list & !iscons(stdout)) fputs(str, stdout);
  }

