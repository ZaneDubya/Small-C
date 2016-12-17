/*
** asm4.c -- Small Assembler -- Part 4: Listing, Symbol Table, and 
**                                      Miscellaneous Functions
*/
#include <stdio.h>
#include "notice.h" 
#include "asm.h" 

extern unsigned
  badsym,  ccnt,  errors,  gotcolon,  lerr,  lin,
  list,  lline,  loc[],  looks,  lookups,  lpage,
  part1,  pause,  pass,  stmax,  stn,  *stp,  upper;

extern unsigned char
  assume[],  line[],  locnull[],  locstr[],  *segptr,
  srcfn[],  *st,  *stend,  *stptr,  *strnxt,  *strend,  stsym[];

/*****************************************************************
                     listing  functions
*****************************************************************/

/*
** list a code item
*/
listcode(rep, val, sz, suff, point)
  int rep, sz, point;  unsigned char val[], suff[]; {
  int i;  char str[5];
  if(list) {
    i = sz + sz + strlen(suff);       /* calc columns needed */
    if(rep) {                         /* adjust for repeated item */
      left(itox(rep, str, 5));
      i += strlen(str) + 2;
      }
    if((ccnt + i) > 22) {             /* won't fit, start new line */
      endline();
      begline();
      } 
    listloc();                        /* list loc if appropriate */
    if(rep) {                         /* list repetition prefix */
      fputs(str, stdout);
      fputc('[', stdout);
      }
    if(point) {                       /* list pointer */
      if(sz > 4) listone(val+2, str, 5);
      if(sz > 2) listone(val  , str, 5);
      fputs("----", stdout);
      }
    else {                            /* list byte or words */
      while(sz > 1) {
        sz -= 2;
        listone(val+sz, str, 5);
        }
      if(sz) listone(val, str, 3);
      }
    if(rep) fputc(']', stdout);       /* list repetition suffix */
    fputs(suff, stdout);              /* list item suffix */
    ccnt += i;                        /* advance column count */
    }
  }

/*
** list one hex item of one or two bytes
*/
listone(v, s, sz) char *s; unsigned *v, sz; {
  char *cp; cp = s;
  itox(*v, s, sz);
  while(*cp == ' ')  *cp++ = '0';
  fputs(s, stdout);
  }

/*
** prohibit listing the location counter
*/
noloc() {
  strcpy(locstr, locnull);
  }

/*
** list the ASCII location counter if not cleared
*/
listloc() {
  if(*locstr) {
    fputs(locstr, stdout);
    fputs("  ", stdout);
    *locstr = 0;
    }
  }

/*
** begin a line in the listing
*/
begline() {
  char str[6], *cp;
  if((pass == 2) && list) {
    if(begpage()) {
      puts("line location  -------object-------  source");
      puts("");
      lline += 2;
      }
    itou(lin, str, 5);
    fputs(str, stdout);
    ltox(loc, locstr, 9);   /* may or may not list */
    cp = locstr + 4;
    while(*cp == ' ') *cp++ = '0';    /* leading zeroes on loc ctr */
    putchar(' ');
    ccnt = 0;
    ++lline;
    }
  }

/*
** begin a page?
*/
begpage() {
  char str[4];
  if(lline >= LASTLINE) {
    lline = 2;
    ++lpage;
    if(lpage > 1)  puts("\n\n\n\n\n");
    fputs("File: ", stdout);
    fputs(srcfn, stdout);
    itou(lpage, str, 4);
    fputs("    Page: ", stdout);
    fputs(str, stdout);
    fputs("    Small Assembler, ", stdout);
    fputs(VERSION, stdout); puts("");
    return (YES);
    }
  return (NO);
  }

/*
** end a line in the listing
*/
endline() {
  char *cp;
  int col;
  col = 0;
  if((pass == 2) && list) {
    if(part1)  puts("");
    else {
      part1 = YES;
      listloc();
      while(ccnt++ < 22)  putchar(' ');
      cp = line;
      while(*cp) {
        if(*cp == '\t') {
          do putchar(' '); while(++col % 8);
          ++cp;
          }
        else {
          putchar(*cp++);
          ++col;
          }
        }
      }
    }
  }

hdwerr()   {lerr |= 0x0001;}
phserr()   {lerr |= 0x0002;}
experr()   {lerr |= 0x0004;}
inverr()   {lerr |= 0x0008;}
rederr()   {lerr |= 0x0010;}
symerr()   {lerr |= 0x0020;}
relerr()   {lerr |= 0x0040;}
underr()   {lerr |= 0x0080;}
parerr()   {lerr |= 0x0100;}
rngerr()   {lerr |= 0x0200;}
synerr()   {lerr |= 0x0400;}
csaerr()   {lerr |= 0x0800;}
naderr()   {lerr |= 0x1000;}
segerr()   {lerr |= 0x2000;}
proerr()   {lerr |= 0x4000;}
farerr()   {lerr |= 0x8000;}

/*
** gripe about errors in a line
*/
gripe() {
  if(lerr) {
    if(!list || !iscons(stdout))  fputs(line, stderr);
    if(lerr &  0x0001)  outerr("- Wrong Hardware\n");
    if(lerr &  0x0002)  outerr("- Phase Error\n");
    if(lerr &  0x0004)  outerr("- Bad Expression\n");
    if(lerr &  0x0008)  outerr("- Invalid Instruction\n");
    if(lerr &  0x0010)  outerr("- Redundant Definition\n");
    if(lerr &  0x0020)  outerr("- Bad Symbol\n");
    if(lerr &  0x0040)  outerr("- Relocation Error\n");
    if(lerr &  0x0080)  outerr("- Undefined Symbol\n");
    if(lerr &  0x0100)  outerr("- Bad Parameter\n");
    if(lerr &  0x0200)  outerr("- Range Error\n"); 
    if(lerr &  0x0400)  outerr("- Syntax Error\n");
    if(lerr &  0x0800)  outerr("- CS Not Assumed for this Segment\n");
    if(lerr &  0x1000)  outerr("- Not Addressable\n");
    if(lerr &  0x2000)  outerr("- Segment Error\n");
    if(lerr &  0x4000)  outerr("- Procedure Error\n");
    if(lerr &  0x8000)  outerr("- Need FAR PTR\n");
    if(pause) waiting();
    outerr("\n");
    errors++;
    }
  }

/*
** report error count
*/
errshow(fd) int fd; {
  if(fd == stdout) {
    if(lline >= (LASTLINE - 2))
      while(!begpage()) {puts(""); ++lline;}
    lline += 2;
    }
  itou(errors, locstr, 7);
  fputs("\n", fd);
  fputs(locstr, fd);
  fputs(" lines have errors\n", fd);
#ifdef DEBUG
  itou(lookups, locstr, 7);
  fputs(locstr, fd);
  fputs(" MIT searches\n", fd);
  itou(looks, locstr, 7);
  fputs(locstr, fd);
  fputs(" looks required\n", fd);
  if(lookups) {
    itou(looks / lookups, locstr, 7);
    fputs(locstr, fd);
    fputs(" looks/search average\n", fd);
    }
#endif
  }

/*
** output an error line
*/
outerr(str) char *str; {
  if(list) {
    begpage();
    fputs(str, stdout);
    ++lline;
    }
  if(!list || !iscons(stdout)) fputs(str, stderr);
  }

/*
** list symbols
*/
symlist() {
  int i;
  if(lline >= (LASTLINE - 5))
    while(!begpage()) {puts(""); ++lline;}
  else                {puts(""); ++lline;}
  symhead();
  symsort(0, stn - 1);
  for(i = 0; i < stn; ++i) {
    symshow(stp[i]);
    poll(YES);
    }
  }

/*
** show a symbol
*/
symshow(sp) char *sp; {
  unsigned u[2];
  if(begpage()) symhead();
  if(flags(sp) & FSET)  fputs("  = ", stdout);
  else                  fputs("    ", stdout);
  if(flags(sp) & FEQU)  fputs(" equ", stdout);
  else                  fputs("    ", stdout);
  if(flags(sp) & FSEG)  fputs(" seg", stdout);
  else                  fputs("    ", stdout);
  if(flags(sp) & FGRP)  fputs(" grp", stdout);
  else                  fputs("    ", stdout);
  if(flags(sp) & FCLS)  fputs(" cls", stdout);
  else                  fputs("    ", stdout);
  if(flags(sp) & FPUB)  fputs(" pub", stdout);
  else                  fputs("    ", stdout);
  if(flags(sp) & FEXT)  fputs(" ext", stdout);
  else                  fputs("    ", stdout);
  if(flags(sp) & FFAR)  fputs(" far", stdout);
  else                  fputs("    ", stdout);
  if(flags(sp) & FPRO)  fputs(" pro", stdout);
  else                  fputs("    ", stdout);
  if(flags(sp) & FCOD)  fputs(" cod", stdout);
  else                  fputs("    ", stdout);
  if(flags(sp) & FDAT) {
    fputs(" dat", stdout);
    itox(sp[STSIZE], locstr, 6);
    fputs(locstr, stdout);
    fputs(" ", stdout);
    }
  else fputs("          ", stdout);
  ltox(sp+STVAL2,  locstr, 9);
  fputs(locstr, stdout);
  fputs("  ", stdout);
  fputs(symbol(sp), stdout); puts("");
  ++lline;
  }

/*
** write heading for symbol listing
*/
symhead() {
  fputs("---------------- attributes ---------------- ", stdout);
  fputs("size    value  symbol\n\n", stdout);
  lline += 2;
  }

/*
** shell sort the symbol table pointers
*/
symsort(l, u) int l, u; {
  int gap, i, j, k, jg;
  gap = (u - l + 1) >> 1;
  while(gap > 0) {
    i = gap + l;
    while(i <= u) {
      j = i++ - gap;
      while(j >= l) {
        jg = j + gap;
        if(strcmp(symbol(stp[j]), symbol(stp[jg])) <= 0)  break;
        k = stp[jg];
        stp[jg] = stp[j];
        stp[j] = k;
        j -= gap;
        }
      }
    gap >>= 1;
    }
  }

/*****************************************************************
                   symbol table functions
*****************************************************************/

/*
** add a new symbol to the symbol table
*/
addsym() {
  int *ip;  char *cp;
  ip = stptr + STSTR;
  if(*ip)
    error2("+ Symbol Table Overflow at: ", line);
  *ip  = strnxt;
  cp = stsym;
  while(*strnxt++ = (upper ? toupper(*cp++) : *cp++))
    if(strnxt >= strend)
      error2("+ Symbol Buffer Overflow at: ", line);
  stp[stn++] = stptr;       /* set symbol pointer */
  }

/*
** find stsym in symbol table
** leave stptr pointing to desired or null entry
** return true if found, else false
*/
stfind() {
  char *start;
  stptr = start = st + hash(stsym, stmax) * STENTRY;
  while(stptr[0] || stptr[1]) {
    if(strcmp(stsym, symbol(stptr)) == 0)  return (YES);
    if((stptr += STENTRY) >= stend)  stptr = st;
    if(stptr == start)  break;
    }
  return(NO);
  }

/*
** Verify new symbol and table it.
** Return YES if symbol added (even if
** previously defined PUBLIC) else NO.
** On return, if stptr is not zero then
** (1) a new symbol was added or
** (2) an old symbol was found.
*/
newsym(must, colon, fbits) int must, colon, fbits; {
  stptr = 0;
  if((must && *stsym == 0) || badsym) {
    symerr();
    return (NO);
    }
  if(*stsym == 0) return (NO);
  if(gotcolon != colon) {
    synerr();
    return (NO);
    }
  if(stfind()) {              /* already in table? */
    if(flags(stptr) == FPUB) ;
    else {
      if(pass == 1) orflags(stptr, FRED);
      else if(flags(stptr) & FRED) rederr();
      return (NO);
      }
    }
  else addsym();
  orflags(stptr, fbits);
  return (YES);
  }

/*
** return symbol pointer
*/
symbol(sp) unsigned *sp; {
  return (sp[STSTR]);
  }

/*
** get 1 word value from symbol table entry sp
*/
val1(sp) char *sp; {
  int *ip;
  ip = sp + STVAL1;
  return (*ip);
  }

/*
** get 2 word value from symbol table entry sp into dest
*/
val2(dest, sp) unsigned *dest; char *sp; {
  int *ip;
  ip = sp + STVAL2;
  dest[0] = ip[0];
  dest[1] = ip[1];
  }

/*
** put 1-word value v into symbol table entry sp
*/
setval1(sp, v) char *sp; int v; {
  int *ip;
  ip = sp + STVAL1;
  *ip = v;
  }

/*
** put 2-word value v into symbol table entry sp
*/
setval2(sp, v) char *sp; int v[]; {
  int *ip;
  ip = sp + STVAL2;
  ip[0] = v[0];
  ip[1] = v[1];
  }

/*
** get flags from symbol table entry sp
*/
flags(sp) char *sp; {
  int *ip;
  ip = sp + STFLAGS;
  return (*ip);
  }

/*
** put flags f into symbol table entry sp
*/
setflags(sp, f) char *sp; int f; {
  int *ip;
  ip = sp + STFLAGS;
  *ip = f;
  }

/*
** bitwise OR f with symbol table flags in entry sp
*/
orflags(sp, f) char *sp; int f; {
  int *ip;
  ip = sp + STFLAGS;
  *ip |= f;
  }

/*****************************************************************
                   miscellaneous functions
*****************************************************************/

/*
** map seg reg (xS) to seg reg pref index (P_xS)
*/
sr2srpx(r) unsigned char r; {   /* must strip high order 8 bits */
  switch(r) {
    case ES: return (P_ES);
    case CS: return (P_CS);
    case SS: return (P_SS);
    case DS: return (P_DS);
    case ES: return (P_ES);
    case FS: return (P_FS);
    case GS: return (P_GS);
    }
  return (0);
  }

/*
** map from seg index to srpx
*/
sx2srpx(segx, prefer) int segx, prefer; {
  char *cp;
  cp = assume + (segx << 3);
  while(cp[1]) {
    if(cp[0] == prefer) return (prefer);
    ++cp;
    }
  return (cp[0]);
  }

/*
** map from srpx index to seg index
** return zero if sr not assumed
*/
srpx2sx(srpx) int srpx; {
  char *cp;  int i;
  i = 0;
  while(++i <= MAXSEG) {
    cp = assume + (i << 3);
    while(*cp) if(*cp++ == srpx) return (i);
    }
  return (0);
  }

/*
** clear all assumed seg regs
*/
nosegs() {
  int i;  i = 7;
  while(++i < ((MAXSEG+1)<<3)) assume[i] = 0;
  }

