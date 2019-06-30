/*
** asm1.c -- Small Assembler -- Part 1: Mainline and Macro Functions
**
** Copyright 1988 J. E. Hendrix
**
** Usage: ASM source [object] [-C] [-L] [-NM] [-P] [-S#]
**        ASM source [object] [/C] [/L] [/NM] [/P] [/S#]
**
** Command-line arguments may be given in any order.  Switches may be
** introduced by a hyphen or a slash.  The brackets indicate optional
** arguments; do not enter the brackets themselves.
**
** source   Name of the source file to be assembled.  The default, and
**          only acceptable, extension is ASM.  Drive and path may be
**          specified.
**
** object   Name of the object file to be output.  It must have a OBJ
**          extension to be recognized as the output file.  Drive and path
**          may be specified.  If no object file is specified, output
**          will go to a file bearing the same name and path as the
**          source file, but with a OBJ extension.
**
** -C       Use case sensitivity.  Without this switch, all symbols are
**          converted to upper-case.  With this switch, they are taken
**          as is; upper- and lower-case variants of the same letter are
**          considered to be different.  With or without this switch,
**          segment, group, and class names are always converted to
**          upper-case in the output OBJ file.
**
** -L       Generate an assembly listing on the standard output file.
**
** -NM      No macro processing.  This speeds up the assembler somewhat.
**          Macro processing is NOT needed for Small-C output files.
**
** -P       Pause on errors waiting for an ENTER keystroke.
**
** -S#      Set symbol table size to accept # symbols.  Default is 1000.
*/
#include <stdio.h>
#include "notice.h"
#include "obj1.h"
#include "obj2.h"
#include "asm.h" 
#include "mit.h"

extern unsigned
  I16max[],
  lookups,
  zero[];

/*
** symbol table
*/
unsigned
   stmax = STMAX,       /* maximum symbols */
   stn,                 /* number of symbols loaded */
  *stp;                 /* symbol table pointer array */
unsigned char
  *st,                  /* symbol table buffer */
  *stend,               /* end of symbol table */
  *stptr,               /* st entry pointer */
  *strbuf,              /* st string buffer */
  *strnxt,              /* next byte in string buffer */
  *strend,              /* end of string buffer */
   stsym[MAXNAM+1];     /* temporary symbol space */

/*
** macro definition table
*/
unsigned char
  *mt,                  /* macro table buffer */
  *mtprev,              /* previous mt entry */
  *mtnext,              /* next available mt byte */
  *mtend,               /* end of macro table */
  *mtptr;               /* mt entry pointer */

/*
** miscellaneous variables
*/
unsigned
  debug,                /* debugging displays? */
  list,                 /* generate a listing? */
  pause,                /* pause on errors? */
  macros = YES,         /* macro processing? */
  upper = YES,          /* upper-case symbols? */
  pass = 1,             /* which pass? */
  use,                  /* 2 = USE16, 4 = USE32 */
  defuse = 2,           /* default use */
  asa,                  /* address size attribute for instruction */
  osa,                  /* operand size attribute for instruction */
  srpref[7],            /* MIT seg reg pref pointers */
  segndx,               /* current segment index */
  badsym,               /* bad symbol? */
  gotcolon,             /* symbol terminated with a colon? */
  gotcomma,             /* symbol terminated with a comma? */
  gotnam,               /* have a name? */
  eom,                  /* end of module? */
  errors,               /* error count */
  enda = M_A_NN,        /* END attribute */
  ends,                 /* END segment index */
  endv[2],              /* END value */
  inst[2],              /* both MIT instruction indexes */
  lerr,                 /* line error flags */
  loc[2],               /* location counter */
  lin,                  /* line counter */
  srcfd,                /* source file fd */
  lline,                /* listing line */
  part1,                /* part 1 of listing line printed? */
  ccnt,                 /* count of code characters printed */
  lpage,                /* listing page */
  mlnext,               /* next macro label to assign */
  mlnbr[10],            /* macro label numbers */
  mpptr[10],            /* macro parameter pointers */
  ofd,                  /* output file descriptor */
  seed,                 /* first mem ref type to look for im MIT */
  defmode,              /* macro definition mode */
  expmode;              /* macro expansion mode */

unsigned char
 *ep,                   /* expression pointer */
 *lp,                   /* line pointer */
 *prior,                /* prior ext ref in chain */
 *exthead,              /* head of extrn chain in symbol table */
  locstr[9],            /* ASCII version of location counter */
  locnull[]="        ", /* null ASCII location counter */
  line[MAXLINE],        /* source line */
  srcfn[MAXFN+4],       /* source filename */
  ofn[MAXFN+4],         /* object filename */
  omt[MAXFN+4],         /* object module title */
 *proptr,               /* current procedure st pointer */
 *segptr,               /* current segment st pointer */
  assume[(MAXSEG+1)<<3];/* assumed seg regs (seg reg pref index) */
                        /* only 6 possible per segment */
                        /* 2 extras supply null terminator and */
                        /* allow shifting rather than multiplying */

/*****************************************************************
                     high-level control
*****************************************************************/

main(argc, argv) int argc, *argv; {
  fputs("Small Assembler, ", stderr);
  fputs(VERSION, stderr);
  fputs(CRIGHT1, stderr);
  parms(argc, argv);            /* get command line switches */
  pass1(argc, argv);            /* build symbol table */
  pass2(argc, argv);            /* generate object code */
  if(errors) {
    if(!debug) {
      delete(ofn);              /* hide corrupt output from LINK */
      fputs("- Deleted Object File\n", stderr);
      }
    abort(10);
    }
  }

/*
** pass one
*/
pass1(argc, argv) int argc, *argv; {
  unsigned max;
  fputs("pass 1\n", stderr);
  st     = calloc(stmax*STENTRY, 1); /* allocate zeroed symbol table */ 
  stp    = calloc(stmax, 2);         /* allocate symbol table ptr array */
  stend  = st + stmax*STENTRY;       /* note end of table */
  strbuf =
  strnxt = malloc(stmax*STRAVG);     /* allocate string buffer */
  strend = strbuf + stmax*STRAVG;    /* note end of buffer */
  max    = avail(YES) - (STACK + OSIZE + FSIZE);
  mt     = mtnext = calloc(max, 1);  /* allocate space */
  mtend  = mt + max - MAXLINE;       /* note end of macro buffer */
  init();                            /* set initial conditions */
  dopass(argc, argv);                /* do pass 1 */
  }

/*
** pass two
*/
pass2(argc, argv) int argc, *argv; {
  int i;
  fputs("pass 2\n", stderr);
  ofd   = open(ofn, "w");
  nosegs();           /* reset seg reg assumptions */
  putTHEADR(omt);
  putNAMs();
  putSEGs();
  putGRPs();
  putEXTs();
  putPUBs();
  segptr = proptr = use = 0;
  pass = 2;
  dopass(argc, argv);
  putMODEND(valuse(endv), enda, F_F_SI, ends, F_T_SID, ends, endv);
  if(list) {
    errshow(stdout);
    symlist();
    }
  errshow(stderr);
  if(ferror(ofd))
    error("- Error in Object File");
  close(ofd);
  }

/*
** process passes 1 and 2
*/
dopass(argc, argv) int argc, *argv; {
  int mop;
  int i;
  mlnext = lpage = i = lin = loc[0] = loc[1] = 0;   /* reset everything */
  lline = 100;                          /* force page heading */
  while(getarg(++i, srcfn, MAXFN, argc, argv) != EOF) {
    if(isswitch(srcfn[0])
    || extend(srcfn, SRCEXT, OBJEXT)) continue; 
    if(!(srcfd = fopen(srcfn, "r")))    /* open source file */
      break;
    eom = NO;                           /* not end of module */
    goto input;

    while(YES) {
      poll(YES);
      ++lin;                            /* bump line counter */
      lerr = 0;                         /* zero line errors */
      part1 = NO;                       /* part 1 of line not listed */
      begline();                        /* begin a listing line */
      if(macros == NO) {
        if(dolabel()                    /* do label, is line empty? */
        && !domach()) doasm();          /* machine or assembler instr? */
        }
      else {                            /* do parocess macros */
        lp = line;
        lp = getsym(lp, YES, NO);
        if(!(mop = macop()) && gotnam) {/* 2nd field a macro keyword? */
          lp = skip(1, line);           /* no, try first */
          mop = macop();
          }
        if(defmode) {                   /* definition mode */
          if(mop == ENDM) defmode = NO;
          if(pass == 1) putmac();       /* put line in macro table */
          noloc();                      /* don't list loc */
          }
        else {                          /* copy or expansion mode */
          if(mop == CALL) {             /* enter expansion mode */
            expmode = YES;
            putparm();                  /* save parameters */
            dolabel();                  /* process label */
            }
          else if(mop == MACRO) {       /* enter definition mode */
            defmode = YES;
            if(pass == 1)  newmac();    /* init new macro in table */
            }
          else if(mop == ENDM) {        /* leave expansion mode */
            expmode = NO;
            }
          else {
            if(expmode)  replace();
            if(dolabel()                /* do label, is line empty? */
            && !domach()) doasm();      /* machine or assembler instr? */
            }
          }
        }
      endline();                        /* end a listing line */
      if(pass == 2)  gripe();           /* gripe about errors */
      if(expmode)  getmac();            /* fetch next macro line */
      else {

        input:
        if(eom || !fgets(line, MAXLINE, srcfd))  break;
        if(debug && (!list || pass == 1)) fputs(line, stdout);
        }
      }
    if(pass == 2) {
      if(defmode)   {outerr("- Missing ENDM\n"); errors++;}
      else if(!eom) {outerr("- Missing END\n" ); errors++;}
      }
    close(srcfd);                      /* close source file */
    return;
    }
  error("- No Source File");
  }

/*
** detect label and store it with location counter
** return true if the line is not empty
*/
dolabel() {
  lp = skip(1, line);                /* locate first field */
  if(atend(*lp)) {                   /* null or comment line */
    noloc();                         /* don't list loc */
    return (NO);                     /* don't try to process */
    }
  lp = getsym(lp, upper, NO);        /* fetch a symbol */
  if(gotcolon) {                     /* got a label */
    if(badsym) {
      symerr();
      return (YES);
      }
    if(pass == 1) {                    /* pass 1 */
      if(stfind()) {
        if(flags(stptr) & ~FPUB) {
          orflags(stptr, FRED);        /* report on pass 2 */
          return (YES);
          }
        }
      else addsym();                   /* table it */
      setval2(stptr, loc);             /* set value */
      orflags(stptr, FCOD);            /* set flags */
      stptr[STNDX] = segndx;           /* set segment index */
      }
    else {                             /* pass 2 */
      if(stfind()) {
        if( flags(stptr) & ~(FCOD|FFAR|FPUB)) rederr();
        if((flags(stptr) & FRED) == 0
        && cmp32(stptr + STVAL2, loc)) phserr();
        }
      else error2("+ Lost Label on Pass 2: ", line);
      }
    }
  return (YES);
  }

/*****************************************************************
                      macro facility
*****************************************************************/

/*
** get a line from the macro buffer
*/
getmac() {
  char *cp; cp = line;
  while(*cp++ = *mtptr++)  ;
  }

/*
** recognize macro operation
*/
macop() {
  if(same(lp, "ENDM" )) {noloc(); return (ENDM);}
  if(same(lp, "MACRO")) {noloc(); return (MACRO);}
  if(!expmode
  && !defmode
  && mtfind())          {noloc(); return (CALL);}
  return (NO);
  }

/*
** test for macro buffer overflow
*/
macover(ptr) char *ptr; {
  if(ptr > mtend)  error("+ Macro Buffer Overflow");
  }

/*
** find stsym in macro table
** return true if found, else false
** leave mtptr pointing to body of desired macro
*/
mtfind() {
  if(atend(*lp) == 0) {
    mtptr = mt;
    do {
      if(same(lp, mtptr+MTNAM)) {
        mtptr += MTNAM;
        mtptr += strlen(mtptr) + 1;
        return (YES);
        }
      mtptr = getint(mtptr);
      } while(mtptr);
    }
  return (NO);
  }

/*
** establish new macro
*/
newmac() {
  int i;
  i = 0;
  if(!gotnam || badsym)  symerr();
  else {
    macover(mtnext);
    if(mtprev)  putint(mtprev, mtnext);
    mtprev = mtnext;
    putint(mtnext, 0);
    mtnext += 2;
    while(*mtnext++ = stsym[i++]) ;
    }  
  }

/*
** put a line in the macro buffer
*/
putmac() {
  char *cp;
  cp = line;
  macover(mtnext);              /* will buffer take it? */
  while(*mtnext++ = *cp++)  ;   /* copy everything */
  }

/*
** save macro call parameters in macro buffer
** and reset macro labels
*/
putparm() {
  int i, dlm;
  char *cp;
  i = -1;
  cp = mtnext;
  lp = skip(2, lp);             /* skip to parameters */
  while(++i < 10) {
    mlnbr[i] = 0;               /* null macro label nbr */
    while(isspace(*lp))  ++lp;
    if(atend(*lp) || *lp == ',')  mpptr[i] = 0;
    else {
      macover(cp);
      mpptr[i] = cp;
      while(!atend(*lp) && *lp != ',') {
        if(*lp == '\"' || *lp == '\'') {        /* string? */
          dlm = *lp;
          while(!atend(*++lp)) {
            if(*lp == dlm && *++lp != dlm) break;
            *cp++ = *lp;
            }
          }
        else  *cp++ = *lp++;
        }
      *cp++ = NULL;
      }
    if(*lp == ',')  ++lp;
    }
  if(!atend(*lp))  parerr();
  }

/*
** replace parameters
*/
replace() {
  char lin[MAXLINE], *cp, *cp2;
  int ndx, i;
  strcpy(lin, line);
  cp = lin;
  i = 0;
  do {
    if(*cp == '?') {                    /* substitution marker? */
      if(isdigit(*++cp)) {              /* parameter substitution? */
        ndx = *cp++ - '0' - 1;          /* which one? */
        if(ndx < 0)  ndx = 9;           /* make 0 mean 10 */
        if(cp2 = mpptr[ndx]) {          /* got parameter? */
          while(*cp2)                   /* yes, copy it */
            if(cantake(i, 1))  line[i++] = *cp2++;
            }
        continue;
        }
      }
    if(*cp == '@') {                            /* label substitution? */
      if(cantake(i, 1))  line[i++] = '@';       /* insert label prefix */
      if(isdigit(*++cp)) {                      /* which one? */
        ndx = *cp++ - '0';
        if(!mlnbr[ndx]) mlnbr[ndx] = ++mlnext;  /* need new label number? */
        if(cantake(i, 5)) {
          left(itou(mlnbr[ndx], line+i, 5));    /* insert label number */
          while(line[i]) ++i;                   /* bypass label number */
          }
        continue;
        }
      }
    if(cantake(i, 1))  line[i++] = *cp++;
    else {
      line[i++] = '\n';
      break;
      }
    } while(*cp);
  line[i] = NULL;
  }

/*
** can line take more?
*/
cantake(i, need) int i, need; {
  return (i < (MAXLINE - 3) - need);
  }

/*****************************************************************
                 dump object file front matter
*****************************************************************/

/*
** declare list of names 
*/
putNAMs() {
  unsigned char *cp;
  int i, namndx;
  namndx = 1;
  begLNAMES();
  putLNAMES("");                /* null name - index 1 */
  for(i = 0; i < stn; ++i) {
    if(flags(stp[i]) & (FSEG|FGRP|FCLS)) {
      ++namndx;
      if(flags(stp[i]) & FCLS) {
        cp = stp[i] + STNDX;    /* save name index for putSEGs() */
        *cp = namndx;           /* st entry must be for class name only */
        }
      putLNAMES(symbol(stp[i]));
      }
    }
  endLNAMES();
  }

/*
** declare segments 
*/
putSEGs() {
  unsigned char *cp1, *cp2;
  int i, sndx, cndx;
  sndx = 1;
  for(i = 0; i < stn; ++i) {
    if(flags(stp[i]) & (FSEG|FGRP|FCLS)) ++sndx;
    if(flags(stp[i]) & FSEG) {
      if(cp2 = val1(cp1 = stp[i]))
           cndx = cp2[STNDX];
      else cndx = 1;
      putSEGDEF(                 /* segment definition */
        valuse(stp[i] + STVAL2), /* 2 = 16 bit, 4 = 32 bit */
        (cp1[STSIZE] >> 3) & 7,  /* align type */
        cp1[STSIZE] & 7,         /* combine type */
        B_NOTBIG,                /* not big segment */
        0,                       /* not absolute, so no frame */
        0,                       /* not absolute, so no offset */
        stp[i] + STVAL2,         /* length of module (loc ctr) */
        sndx,                    /* segment name index */
        cndx,                    /* class name index - null */
        1);                      /* overlay name index - null */
      setval2(stp[i], zero);     /* zero loc ctr for pass 2 */
      }
    }
  }

/*
** declare groups 
*/
putGRPs() {
  int i, namndx;
  unsigned char *cp;
  namndx = 1;
  for(i = 0; i < stn; ++i) {
    if(flags(stp[i]) & (FSEG|FGRP|FCLS)) ++namndx;
    if(flags(stp[i]) & FGRP) {
      begGRPDEF(namndx);
      cp = val1(stp[i]);            /* fetch seg list ptr */
      while(*cp) putGRPDEF(*cp++);  /* write each seg ndx */
      endGRPDEF();
      }
    }
  }

/*
** declare publics 
*/
putPUBs() {
  int i, x, ouse, nuse;
  unsigned char *cp;
  for(i = x = ouse = 0; i < stn; ++i) {
    if(flags(cp = stp[i]) & FPUB) {
      if(cmp32(cp + STVAL2,   zero) < 0
      || cmp32(cp + STVAL2, I16max) > 0)
           nuse = 4;
      else nuse = 2;
      if(cp[STNDX] != x || ouse != nuse) {
        if(x || ouse) endPUBDEF();
        begPUBDEF(ouse = nuse, 0, x = cp[STNDX], 0);
        }
      putPUBDEF(symbol(cp), cp + STVAL2);
      }
    }
  if(x) endPUBDEF();
  }

/*
** declare externals in order of indexes
*/
putEXTs() {
  char *cp;  int i;
  begEXTDEF();
  for(i = 0, cp = exthead; cp; cp = val1(cp)) {
    cp[STNDX] = ++i;         /* set index in symbol table */
    putEXTDEF(symbol(cp));   /* write to OBJ file */
    }
  endEXTDEF();
  }

/*
** return the use value of v
*/
valuse(v) unsigned v[]; {
  if(cmp32(v,   zero) < 0
  || cmp32(v, I16max) > 0) return (4);
  return (2);
  }

/*****************************************************************
                      startup functions
*****************************************************************/

/*
** initialize globals
*/
init() {
  strcpy(stsym, "?");                 /* establish null symbol */
  stfind(); addsym();
  setflags(stptr, FSET);
  if((srpref[P_CS] = getseg("CS:"))   /* set-up seg reg prefix table */
  && (srpref[P_SS] = getseg("SS:"))
  && (srpref[P_DS] = getseg("DS:"))
  && (srpref[P_ES] = getseg("ES:"))) ;
  else error("+ Segment Prefix not in MIT");
  srpref[P_FS]  = getseg("FS:");
  srpref[P_GS]  = getseg("GS:");
  }

/*
** return ptr to seg pref in M.I.T.
*/
getseg(str) char *str; {
  int i;
  if((inst[0] = find ( str, mntbl, mncnt)) == EOF) return (0);
  if((inst[1] = find (zero, optbl, opcnt)) == EOF) return (0);
  if((      i = findi(inst))               == EOF) return (0);
  return (mitbl[i << 1] + 5);
  }

/*
** get parameters from command line
*/
parms(argc, argv) int argc, *argv; {
  char arg[MAXFN+4];  int i, j, len;
  i = 0;
  while(getarg(++i, arg, MAXFN, argc, argv) != EOF) {
    if(isswitch(arg[0])) {
           if(same(arg+1, "L" ))   list = YES;
      else if(same(arg+1, "P" ))  pause = YES;
      else if(same(arg+1, "D" ))  debug = YES;
      else if(same(arg+1, "NM")) macros = NO;
      else if(same(arg+1, "C" ))  upper = NO;
      else if(toupper(arg[1]) == 'S') {
        len = utoi(arg + 2, &j);
        if(len > 0 && !arg[len + 2])  stmax = j;
        else                          usage();
        }
      else  usage();
      }
    else if(extend(arg, SRCEXT, OBJEXT) || !*ofn) {
      strcpy(ofn, arg);
      strcpy(strchr(ofn, '.'), OBJEXT);
      }
    }
  strcpy(omt, ofn);               /* default object module title */
  }

/*
** is ch a switch lead-in character
*/
isswitch(ch) char ch; {
  return (ch == '-' || ch == '/');
  }

/*
** abort with a usage message
*/
usage() {
#ifdef DEBUG
  error("Usage: ASM source [object] [-C] [-D] [-L] [-NM] [-P] [-S#]");
#else
  error("Usage: ASM source [object] [-C] [-L] [-NM] [-P] [-S#]");
#endif
  }

