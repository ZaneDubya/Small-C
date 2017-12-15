/*
** scan.c -- scanning functions
*/
#include <stdio.h>
#include "asm.h"

/*
** is ch at end of line?
*/
atend(ch) int ch; {
  switch(ch) {
    case COMMENT:
    case    NULL:
    case    '\n': return (YES);
    }
  return (NO);
  }

/*
** are fields s and t the same?
*/
same(s, t) char *s, *t; {
  while(lexorder(*s, *t) == 0) {
    if(!isgraph(*s)) return (YES);
    ++s; ++t;
    }
  if((isspace(*s) || *s == '(' || atend(*s))
  && (isspace(*t) || *t == '(' || atend(*t))) return (YES);
  return (NO);
  }

/*
** find nth white-space-separated field in str
*/
skip(n, str) int n; char *str; {
  loop:
  while(*str && isspace(*str)) ++str;
  if(--n) {
    while(isgraph(*str)) ++str;
    goto loop;
    }
  return (str);
  }

