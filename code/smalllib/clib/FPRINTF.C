#include "stdio.h"
/*
** fprintf(fd, ctlstring, arg, arg, ...) - Formatted print.
** Operates as described by Kernighan & Ritchie.
** b, c, d, o, s, u, and x specifications are supported.
** Note: b (binary) is a non-standard extension.
*/
int fprintf(int fd, char *fmt, ...) {
  return(_print(fd, &fmt));
  }

/*
** printf(ctlstring, arg, arg, ...) - Formatted print.
** Operates as described by Kernighan & Ritchie.
** b, c, d, o, s, u, and x specifications are supported.
** Note: b (binary) is a non-standard extension.
*/
int printf(char *fmt, ...) {
  return(_print(stdout, &fmt));
  }

/*
** _print(fd, ctlstring, arg, arg, ...)
** Called by fprintf() and printf().
*/
int _print(int fd, int *nxtarg) {
  int  arg, left, pad, cc, len, maxchr, width;
  int  islong;
  long lval;
  int  *lp;
  char *ctl, *sptr, str[17];
  cc = 0;                                         
  ctl = *nxtarg++;                          
  while(*ctl) {
    if(*ctl!='%') {fputc(*ctl++, fd); ++cc; continue;}
    else ++ctl;
    if(*ctl=='%') {fputc(*ctl++, fd); ++cc; continue;}
    if(*ctl=='-') {left = 1; ++ctl;} else left = 0;       
    if(*ctl=='0') pad = '0'; else pad = ' ';           
    if(isdigit(*ctl)) {
      width = atoi(ctl++);
      while(isdigit(*ctl)) ++ctl;
      }
    else width = 0;
    if(*ctl=='.') {            
      maxchr = atoi(++ctl);
      while(isdigit(*ctl)) ++ctl;
      }
    else maxchr = 0;
    if(*ctl=='l') {islong = 1; ++ctl;} else islong = 0;
    sptr = str;
    if(islong) {
      lp = &lval;
      *lp = *nxtarg++;
      *(lp+1) = *nxtarg++;
      switch(*ctl++) {
        case 'd': ltoa(lval, str);      break;
        case 'u': ltoab(lval, str, 10); break;
        case 'x': ltoab(lval, str, 16); break;
        case 'o': ltoab(lval, str, 8);  break;
        default:  return (cc);
        }
      }
    else {
      arg = *nxtarg++;
      switch(*ctl++) {
        case 'c': str[0] = arg; str[1] = NULL; break;
        case 's': sptr = arg;        break;
        case 'd': itoa(arg,str);     break;
        case 'b': itoab(arg,str,2);  break;
        case 'o': itoab(arg,str,8);  break;
        case 'u': itoab(arg,str,10); break;
        case 'x': itoab(arg,str,16); break;
        default:  return (cc);
        }
      }
    len = strlen(sptr);
    if(maxchr && maxchr<len) len = maxchr;
    if(width>len) width = width - len; else width = 0; 
    if(!left) while(width--) {fputc(pad,fd); ++cc;}
    while(len--) {fputc(*sptr++,fd); ++cc; }
    if(left) while(width--) {fputc(pad,fd); ++cc;}  
    }
  return(cc);
  }

