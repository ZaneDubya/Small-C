/*
** mess.c -- message functions
*/
#include <stdio.h>

extern int list;

puts2(str1, str2) char *str1, *str2; {
  eput(str1);  eputn(str2);
  }

cant(str) char *str; {
  error2("- Can't Open ", str);
  }

error2(str1, str2) char *str1, *str2; {
  eput(str1);  error(str2);
  }

error(str) char *str; {
  eputn(str);  abort(10);
  }

eputn(str) char *str; {
  eput(str);  eput("\n");
  }

eput(str) char *str; {
  fputs(str, stderr);
  if(list & !iscons(stdout)) fputs(str, stdout);
  }

