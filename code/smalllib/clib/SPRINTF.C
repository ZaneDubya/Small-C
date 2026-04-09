#include "stdio.h"
/*
** sprintf(buf, ctlstring, arg, arg, ...) - Formatted print to string.
** Operates as described by Kernighan & Ritchie.
** b, c, d, o, s, u, and x specifications are supported.
** Note: b (binary) is a non-standard extension.
*/
int sprintf(char *buf, char *fmt, ...) {
  return(_sprint(buf, &fmt));
  }

/*
** _sprint(buf, argptr) - core string formatter; backs sprintf().
** buf:    destination buffer (null-terminated on return).
** argptr: pointer to the fmt argument on the stack; variadic args follow.
*/
int _sprint(char *buf, int *nxtarg) {
  int  arg, left, pad, cc, len, maxchr, width;
  int  islong;
  long lval;
  int  *lp;
  char *ctl, *sptr, *dst, str[17];
  cc = 0;
  dst = buf;
  ctl = *nxtarg++;
  while(*ctl) {
    if(*ctl!='%') {*dst++ = *ctl++; ++cc; continue;}
    else ++ctl;
    if(*ctl=='%') {*dst++ = *ctl++; ++cc; continue;}
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
        default:  *dst = NULL; return (cc);
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
        default:  *dst = NULL; return (cc);
        }
      }
    len = strlen(sptr);
    if(maxchr && maxchr<len) len = maxchr;
    if(width>len) width = width - len; else width = 0;
    if(!left) while(width--) {*dst++ = pad; ++cc;}
    while(len--) {*dst++ = *sptr++; ++cc;}
    if(left) while(width--) {*dst++ = pad; ++cc;}
    }
  *dst = NULL;
  return(cc);
  }
