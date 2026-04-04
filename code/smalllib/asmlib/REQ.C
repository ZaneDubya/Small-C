/*
** req.c -- request user input
*/
#include <stdio.h>

/*
** request number
*/
int reqnbr(char prompt[], int *nbr) {
  char str[20];
  int sz;
  if(iscons(stdin)) {
    puts("");
    fputs(prompt, stdout);
    }
  getstr(str, 20);
  if((sz = utoi(str, nbr)) < 0 || str[sz])  return(NO);
  return(YES);
  }

/*
** request string
*/
int reqstr(char prompt[], char *str, int sz) {
  if(iscons(stdin)) {
    puts("");
    fputs(prompt, stdout);
    }
  getstr(str, sz);
  return(*str);       /* null name returns false */
  }

/*
** get string from user
*/
void getstr(char *str, int sz) {
  char *cp;
  fgets(str, sz, stdin);
  if(iscons(stdin) && !iscons(stdout))  fputs(str, stdout);   /* echo */
  cp = str;
  while(*cp) {                       /* trim ctl chars & make uc */
    if(*cp == '\n')  break;
    if(isprint(*str = toupper(*cp++)))  ++str;
    }
  *str = NULL;
  }

