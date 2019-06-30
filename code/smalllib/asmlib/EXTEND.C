/*
** extend.c -- provide filename extension
**
** convert fn to upper case
** if fn has no extension, extend it with ext1
** if fn has an extension, require it to match ext1 or ext2
** return true if fn's original extension matches ext2, else false
*/
#include <stdio.h>
#include "asm.h"

extend(fn, ext1, ext2) char *fn, *ext1, *ext2; {
  char *cp;
  for(cp = fn; *cp; cp++) *cp = toupper(*cp);
  if(cp = strchr(fn, '.')) {
    if(lexcmp(cp, ext2) == 0)  return (YES);
    if(lexcmp(cp, ext1) == 0)  return (NO);
    error2("- Invalid Extension: ", fn);
    }
  if(strlen(fn) > MAXFN-4)  error2("- Filename Too Long: ", fn);
  strcat(fn, ext1);
  return (NO);
  }
