/*
** cmit.c -- Machine Instruction Table Compiler
**
** Copyright 1988 J. E. Hendrix
**
** Usage: CMIT [-#] [-O] [-L] [table] 
**
** -#     Highest numbered CPU to recognize:
**
**            -0  =>  8086
**            -1  => 80186
**            -2  => 80286
**            -3  => 80386
**
** -O     Output the indicated or default machine instruction table
**        as an OBJ file.
**
** -L     List the compiled machine instruction table.
**
** table  Name of the Machine Instruction Table source file
**        (default 80X86.MIT).  The default and only allowed
**        extension is MIT.  Drive and path are allowed.
**
** NOTE: if no switches are given, -L is assumed.  If any
**       switches are given, only the actions specified are taken.
**
** NOTE: An OBJ file created by CMIT must be linked into
**       ASM.EXE.
*/
#include <stdio.h>
#include "asm.h"
#include "obj2.h"
#include "notice.h"

#define MNCNT      363          /* nmemonic hash space */
#define OPCNT      107          /* operand hash space */
#define MICNT      671          /* machine instr hash space */
#define MITBUFSZ  8200          /* mit buffer space */
#define MITEXT       ".MIT"     /* M.I.T. file extension */
#define MITSRC  "80X86.MIT"     /* default M.I.T. source file */
#define MITOBJ  "80X86.OBJ"     /* default M.I.T. object file */
#define OTPLUS      52          /* max extension to otmap[] and otnext[] */

char
 *ptr,                     /* source line pointer */
  objfn[MAXFN] = MITOBJ,   /* default object mit filename */
  mitfn[MAXFN] = MITSRC;   /* default source mit filename */

unsigned
  ot_high,                 /* highest OTPLUS value */
  totinstr,                /* total number of instructions */
  totlooks,                /* total number of look-ups */
  prefix,                  /* is menomic an instruction prefix? */
  ibuf[2],                 /* buffer for instruction index pair */
  index,                   /* progressive index into mitbuf[] */
  ofd,                     /* output file descriptor */
  obj,                     /* output OBJ? */
  list,                    /* list? */
  hdw = 3;                 /* highest CPU/NDP to recognize */

extern unsigned
  looks;                   /* number of looks to find it */

/*
** machine instruction table
*/
unsigned
  mntbl[MNCNT*2],          /* table of mnemonics */
  optbl[OPCNT*2],          /* table of operands */
  mitbl[MICNT*2],          /* table of machine instructions */
  mncnt = MNCNT,           /* entries in mnemonic table */
  opcnt = OPCNT,           /* entries in operand table */
  micnt = MICNT,           /* entries in machine instr table */
  mnused,                  /* mnemonic table entries used */
  opused,                  /* operand table entries used */
  miused;                  /* machine instr table entries used */
unsigned char
  otmap [TYPES+OTPLUS],    /* operand-type mapping table */
                           /* if == 0 skip immediately to next */
  otnext[TYPES+OTPLUS],    /* operand-type next state */
                           /* if == 0 end of list */
  mitbuf[MITBUFSZ];        /* instruction syntax buffer */

/*
** high-level control
*/
main(argc, argv) int argc, *argv; {
  fputs("Small Assembler M.I.T. Compiler, ", stderr);
  fputs(VERSION, stderr);  /* issue signon notice */
  fputs(CRIGHT1, stderr);
  verify();                /* verify that otmap[] and otnext[] are okay */
  parms(argc, argv);       /* get run-time parameters */
  setmap();                /* set-up operand mapping table */
  load();                  /* load MIT into internal format */
  fprintf(stderr, "\n");
  fprintf(stderr, " mntbl[]: used %5u of %5u dwords\n", mnused, mncnt);
  fprintf(stderr, " optbl[]: used %5u of %5u dwords\n", opused, opcnt);
  fprintf(stderr, " mitbl[]: used %5u of %5u dwords\n", miused, micnt);
  fprintf(stderr, "mitbuf[]: used %5u of %5u bytes \n", index, MITBUFSZ);
  if(obj)  object();       /* create object file */
  if(list) {
    print();               /* create listing */
    fprintf(stderr, "\n");
    fprintf(stderr, "     instructions: %5u\n", totinstr);
    fprintf(stderr, "            looks: %5u\n", totlooks);
    if(totinstr)
      fprintf(stderr, "looks/instruction: %5u (3 min)\n", totlooks/totinstr);
    }
  }

/*
** get command line parameters
*/
parms(argc, argv) int argc, *argv; {
  char arg[MAXFN];
  int i;
  i = 0;
  while(getarg(++i, arg, MAXFN, argc, argv) != EOF) {
    if(arg[0] == '-' || arg[0] == '/') {
           if(arg[1] == '0')          hdw  = 0;
      else if(arg[1] == '1')          hdw  = 1;
      else if(arg[1] == '2')          hdw  = 2;
      else if(arg[1] == '3')          hdw  = 3;
      else if(toupper(arg[1]) == 'O') obj  = YES;
      else if(toupper(arg[1]) == 'L') list = YES;
      else usage();
      }
    else {
      char *cp;
      extend(arg, MITEXT, MITEXT);     /* default or explicit mit ext */
      strcpy(mitfn, arg);              /* copy as mit fn.ext */
      cp = strchr(arg, '.'); *cp = 0;  /* truncate ext */
      strcpy(objfn, arg);              /* copy as obj fn */
      strcat(objfn, OBJEXT);           /* extend with obj ext */
      }
    }
  if(!obj) list = YES;
  }

/*
** load table from source file
*/
load() {
  int  fd, i, byte;
  char str[MAXLINE], *cp;
  for(i = 0; i < MNCNT*2; mntbl[i++] = EOF) ;
  for(i = 0; i < OPCNT*2; optbl[i++] = EOF) ;
  for(i = 0; i < MICNT*2; mitbl[i++] = EOF) ;
  fd = fopen(mitfn, "r");                  /* open source file */
  while(fgets(str, MAXLINE, fd)) {         /* fetch next line */
    poll(YES);                             /* permit interruption */
    cp = skip(1, str);                     /* first field */
    if(atend(*cp)) continue;               /* ignore comment line */
    if((xtoi(cp, &byte)) > 2)              /* hardware field */
      error2("\n- Bad Hardware Code in:\n", str);
    if(( byte    &7) > hdw) continue;      /* ignore higher cpu */
    if(((byte>>4)&7) > hdw) continue;      /* ignore higher ndp */
    stow_mn(str);                          /* store unique mnemonic */
    stow_op(str);                          /* store unique operand */
    stow_mi(str);                          /* store instr (mnem/oper) */
    putmit(byte);                          /* store hardware code */
    cp = skip(2, str);                     /* op-code format */
    while(*cp > ' ') {                     /* scan it */
      if(isxdigit(*cp)) {                  /* absolute hex byte */
        if((i = xtoi(cp, &byte)) > 2)
          error2("\n- Bad Hex Byte in:\n", str);
        putmit(HEX);
        putmit(byte);
        }
      else if(*cp == '_') i = 1;           /* skip separator */
      else if(i = is(cp, "/01" )) putmit(M01);
      else if(i = is(cp, "/11" )) putmit(M11);
      else if(i = is(cp, "/21" )) putmit(M21);
      else if(i = is(cp, "/31" )) putmit(M31);
      else if(i = is(cp, "/41" )) putmit(M41);
      else if(i = is(cp, "/51" )) putmit(M51);
      else if(i = is(cp, "/61" )) putmit(M61);
      else if(i = is(cp, "/71" )) putmit(M71);
      else if(i = is(cp, "/r1" )) putmit(MR1);
      else if(i = is(cp, "/r12")) putmit(MR12);
      else if(i = is(cp, "/r21")) putmit(MR21);
      else if(i = is(cp, "/i12")) putmit(MI12);
      else if(i = is(cp, "/s12")) putmit(MS12);
      else if(i = is(cp, "/s21")) putmit(MS21);
      else if(i = is(cp, "+f"  )) putmit(PF);
      else if(i = is(cp, "+i1" )) putmit(PI1);
      else if(i = is(cp, "+r1" )) putmit(PR1);
      else if(i = is(cp, "+r2" )) putmit(PR2);
      else if(i = is(cp, "i1"  )) putmit(I1);
      else if(i = is(cp, "i2"  )) putmit(I2);
      else if(i = is(cp, "i3"  )) putmit(I3);
      else if(i = is(cp, "o1"  )) putmit(O1);
      else if(i = is(cp, "o2"  )) putmit(O2);
      else if(i = is(cp, "p1"  )) putmit(P1);
      else if(i = is(cp, "p2"  )) putmit(P2);
      else if(i = is(cp, "$1"  )) putmit(S1);
      else if(i = is(cp, "w6"  )) putmit(W6);
      else error2("\n- Bad Op-code Directive in:\n", str);
      cp += i;
      }
    putmit(0);
    }
  }

/*
** stow mnemonic field in mitbuf[]
** and return with index in ibuf[0]
*/
stow_mn(str) char *str; {
  char *cp, buf[11];
  get_mn(str, buf, 11);                        /* extract mnemonic */
  if(find(buf, mntbl, MNCNT) == EOF) {         /* new field */
                                               /* -2 byte */
    if(strncmp(buf, "MOVS", 4) == 0
    || strncmp(buf, "SCAS", 4) == 0
    || strncmp(buf, "STOS", 4) == 0)      putmit(1);
    else                                  putmit(0);
                                               /* -1 byte */
    if(prefix)                            putmit(PREF);
    else if(buf[0] == 'J')                putmit(S8);     /* jump seed */
    else if(strncmp(buf, "LOOP", 4) == 0) putmit(S8);     /* loop seed */
    else if(strncmp(buf, "CALL", 4) == 0) putmit(S1632);  /* call seed */
    else                                  putmit(M);      /* other seed */

    if(!table(mntbl, hash(buf, MNCNT), MNCNT)) /* insert into mntbl[] */
      error2("\n- mntbl[] Overflow on:\n", str);
    else ++mnused;
    cp = buf; while(putmit(*cp++)) ;           /* copy field */
    }
  if((ibuf[0] = find(buf, mntbl, MNCNT)) == EOF) 
    error2("\n- Can't Find Mnemonic in:\n", str);
  }

/*
** stow encoded operand fields in mitbuf[]
** and return with index in ibuf[1]
*/
stow_op(str) char *str; {
  char buf[11];
  int j;
  get_op(str, buf, 11);
  if(find(buf, optbl, OPCNT) == EOF) {         /* new field */
    if(!table(optbl, hash(buf, OPCNT), OPCNT)) /* insert into optbl[] */
      error2("\n- optbl[] Overflow on:\n", str);
    else ++opused;
    for(j=0; putmit(buf[j++]); ) ;             /* copy opnd field codes */
    }
  if((ibuf[1] = find(buf, optbl, OPCNT)) == EOF) 
    error2("\n- Can't Find Operand in:\n", str);
  }

/*
** stow instruction (mnemonic/operand index pair) in mitbuf[]
*/
stow_mi(str) char *str; {
  char *cp;  int i;
  if(!table(mitbl, hashi(ibuf), MICNT))    /* insert into mitbl[] */
    error2("\n- mitbl[] Overflow on:\n", str);
  else ++miused;
  putmit(ibuf[0]);                         /* copy instruction */
  putmit(ibuf[0] >> 8);
  putmit(ibuf[1]);
  putmit(ibuf[1] >> 8);
  }

/*
** put a new entry into a table
** returns true on success, else false
*/
table(tbl, h, cnt) int tbl[]; unsigned h, cnt; {
  unsigned start, try, end;
  h <<= 1;                            /* convert to offset */
  if(tbl[h] == EOF) tbl[h] = index;   /* empty slot */
  else {                              /* must extend chain */
    start = try = h;                  /* init circular scan */
    end   = (cnt - 1) << 1;           /* last slot */
    while(YES) {
      try += 2;                       /* next slot */
      if(try == start) return (NO);   /* back to start? */
      if(try > end) try = -2;         /* cycle back? */
      else {
        if(tbl[try] == EOF) {         /* empty slot? */
          tbl[try + 0] = index;       /* point into mitbuf[] */
          tbl[try + 1] = tbl[h + 1];  /* link first in chain */
          tbl[h   + 1] = try >> 1;
          break;                      /* exit scan */
          }
        }
      }
    }
  return (YES);
  }

/*
** put a byte into mitbuf[] with overflow checking
** returns value stored
*/
putmit(ch) unsigned char ch; {
  if(index < MITBUFSZ) mitbuf[index++] = ch;
  else error("\n- mitbuf[] Overflow"); 
  return (ch);
  }

/*
** is src (substring) the same as lit (string)?
** return length if so
*/
is(src, lit) unsigned char *src, *lit; {
  int len;  len = 0;
  while(*lit) {
    if(*src++ == *lit++) len++;
    else return (0);
    }
  if((*src <= ' ')
  || (*src == '_')
  || (*src == ',')
  || (*src == ';')) return (len);
  else              return (0);
  }

/*
** encode operand fields from str into buf
*/
get_op(str, buf, max) char str[], buf[]; int max; {
  char *cp, *ret;
  int i, j, k;
  cp = ret = skip(4, str);         /* locate operands */
  j = 0;
  while(!atend(*cp) && j < max) {
    if(isspace(*cp) || *cp == ',') {
      cp++;
      continue;
      }
    else if(i = is(cp, "i6"   )) k = I6;
    else if(i = is(cp, "i8"   )) k = I8;
    else if(i = is(cp, "i16"  )) k = I16;
    else if(i = is(cp, "i32"  )) k = I32;
    else if(i = is(cp, "i1632")) k = I1632;
    else if(i = is(cp, "r8"   )) k = R8;
    else if(i = is(cp, "r16"  )) k = R16;
    else if(i = is(cp, "r32"  )) k = R32;
    else if(i = is(cp, "r1632")) k = R1632;
    else if(i = is(cp, "/8"   )) k = _8;
    else if(i = is(cp, "/16"  )) k = _16;
    else if(i = is(cp, "/32"  )) k = _32;
    else if(i = is(cp, "/1632")) k = _1632;
    else if(i = is(cp, "$8"   )) k = S8;
    else if(i = is(cp, "$1632")) k = S1632;
    else if(i = is(cp, "m"    )) k = M;
    else if(i = is(cp, "m8"   )) k = M8;
    else if(i = is(cp, "m16"  )) k = M16;
    else if(i = is(cp, "m32"  )) k = M32;
    else if(i = is(cp, "m1632")) k = M1632;
    else if(i = is(cp, "m48"  )) k = M48;
    else if(i = is(cp, "m3248")) k = M3248;
    else if(i = is(cp, "m64"  )) k = M64;
    else if(i = is(cp, "m3264")) k = M3264;
    else if(i = is(cp, "m80"  )) k = M80;
    else if(i = is(cp, "n8"   )) k = N8;
    else if(i = is(cp, "n16"  )) k = N16;
    else if(i = is(cp, "n32"  )) k = N32;
    else if(i = is(cp, "n1632")) k = N1632;
    else if(i = is(cp, "p32"  )) k = P32;
    else if(i = is(cp, "p48"  )) k = P48;
    else if(i = is(cp, "p3248")) k = P3248;
    else if(i = is(cp, "1"    )) k = ONE;
    else if(i = is(cp, "3"    )) k = THREE;
    else if(i = is(cp, "AL"   )) k = AL;
    else if(i = is(cp, "AX"   )) k = AX;
    else if(i = is(cp, "CL"   )) k = CL;
    else if(i = is(cp, "DX"   )) k = DX;
    else if(i = is(cp, "eAX"  )) k = eAX;
    else if(i = is(cp, "EAX"  )) k = EAX;
    else if(i = is(cp, "CS"   )) k = CS;
    else if(i = is(cp, "DS"   )) k = DS;
    else if(i = is(cp, "ES"   )) k = ES;
    else if(i = is(cp, "FS"   )) k = FS;
    else if(i = is(cp, "GS"   )) k = GS;
    else if(i = is(cp, "SS"   )) k = SS;
    else if(i = is(cp, "CRx"  )) k = CRx;
    else if(i = is(cp, "DRx"  )) k = DRx;
    else if(i = is(cp, "TRx"  )) k = TRx;
    else if(i = is(cp, "xS"   )) k = xS;
    else if(i = is(cp, "STx"  )) k = STx;
    else if(i = is(cp, "ST"   )) k = ST0;
    else  error2("\n- Bad Operand Field in:\n", str);
    buf[j++] = k;
    cp += i;
    }
  buf[j] = 0;
  return (ret);
  }

/*
** create OBJ file
*/
object() {
  int tmp[2];
  tmp[1] = 0;
  ofd = fopen(objfn, "w");

  putTHEADR(objfn);         /* translator module header */

  begLNAMES();              /* list of names */
  putLNAMES("");               /* index 1 */
  putLNAMES("DATA");           /* index 2 */
  endLNAMES();

  tmp[0] = MNCNT*4          /* length of module */
         + OPCNT*4
         + MICNT*4
         + index
         + TYPES+OTPLUS
         + TYPES+OTPLUS
         + 6;
  putSEGDEF(                   /* segment definition */
            2,                 /* use 2 byte word length */
            A_PARA,            /* paragraph aligned */
            C_PUBLIC,          /* public combine-type */
            B_NOTBIG,          /* not big segment */
            0,                 /* not absolute, so no frame */
            0,                 /* not absolute, so no offset ptr */
            tmp,               /* length of module */
            2,                 /* segment name index */
            1,                 /* class name index */
            1);                /* overlay name index */

  begPUBDEF(2, 0, 1, 0);       /* public names definition (use, gi, si, f#) */
  tmp[0]  = 0;
  putPUBDEF("_mntbl", tmp);
  tmp[0] += MNCNT*4;
  putPUBDEF("_optbl", tmp);
  tmp[0] += OPCNT*4;
  putPUBDEF("_mitbl", tmp);
  tmp[0] += MICNT*4;
  putPUBDEF("_mitbuf", tmp);
  tmp[0] += index;
  putPUBDEF("_otmap", tmp);
  tmp[0] += TYPES+OTPLUS;
  putPUBDEF("_otnext", tmp);
  tmp[0] += TYPES+OTPLUS;
  putPUBDEF("_mncnt", tmp);
  tmp[0] += 2;
  putPUBDEF("_opcnt", tmp);
  tmp[0] += 2;
  putPUBDEF("_micnt", tmp);
  endPUBDEF();

  tmp[0] = 0;
  begLEDATA(2, 2, 1, tmp);         /* logical enum data (use, si, offset) */
  putLEDATA(mntbl,  MNCNT*4);      /* output mntbl[] (source, len) */
  putLEDATA(optbl,  OPCNT*4);      /* output optbl[] (source, len) */
  putLEDATA(mitbl,  MICNT*4);      /* output mitbl[] (source, len) */
  putLEDATA(mitbuf, index);        /* output mitbl[] (source, len) */
  putLEDATA(otmap,  TYPES+OTPLUS); /* output otmap[] (source, len) */
  putLEDATA(otnext, TYPES+OTPLUS); /* output otnext[] (source, len) */
  putLEDATA(&mncnt,  2);           /* output mncnt   (source, len) */
  putLEDATA(&opcnt,  2);           /* output opcnt   (source, len) */
  putLEDATA(&micnt,  2);           /* output micnt   (source, len) */
  endLEDATA();

  tmp[0] = 0;
  putMODEND(                   /* module end record */
            2,                 /* 2 byte word length */
            M_A_NN,            /* non-main, no starting address */
            0, 0, 0, 0, tmp);  /* no other parameters */
  fclose(ofd);
  }

/*
** print compiled machine instruction table
*/
print() {
  int fd, line, ndx, byte;
  unsigned char str[MAXLINE], buf[11], fmt, *cp, first;
  fd = open(mitfn, "r");
  header(line = 1);
  while(fgets(str, MAXLINE, fd)) {
    poll(YES);
    cp = skip(1, str);                     /* first field */
    if(atend(*cp)) continue;               /* ignore comment line */
    if((xtoi(cp, &byte)) > 2)              /* hardware field */
      error2("\n- Bad Hardware Code in:\n", str);
    if(( byte    &7) > hdw) continue;      /* ignore higher cpu */
    if(((byte>>4)&7) > hdw) continue;      /* ignore higher ndp */
    if(cp = strchr(str, ';')) *cp = NULL;  /* truncate comments */
    cp = str + strlen(str);
    while((--cp >= str)
       && (*cp <= ' ')) *cp = NULL;        /* truncate trailing spaces & \n */
    cp = get_mn(str, buf, 11);
    if(++line > 58) line = header(line);
    printf("%-30s ", cp);                  /* print source */
    looks = 0;
    if((ibuf[0] = find(buf, mntbl, MNCNT)) == EOF)
      error("\n- Can't Find Mnemonic in mntbl[]");
    cp = get_op(str, buf, 11);             /* encode operands in buf[] */
    if((ibuf[1] = find(buf, optbl, OPCNT)) == EOF)
      error("\n- Can't Find Operand in optbl[]");
    if((ndx = findi(ibuf)) == EOF)
      error("\n- Can't Find Instruction in mitbl[]");
    printf("     %3u       ", looks);
    totinstr++;
    totlooks += looks;
    ndx = mitbl[ndx << 1] + 5;             /* op-code directives */
    first = YES;
    do {
      fmt = mitbuf[ndx++];
      if(!first && fmt)  putchar(' ');
      switch(fmt) {
        case   0:  printf("\n");  break;
        case HEX:  printf("%2x ", mitbuf[ndx++] & 255); break;
        case M01:  printf("/01 ");   break;
        case M11:  printf("/11 ");   break;
        case M21:  printf("/21 ");   break;
        case M31:  printf("/31 ");   break;
        case M41:  printf("/41 ");   break;
        case M51:  printf("/51 ");   break;
        case M61:  printf("/61 ");   break;
        case M71:  printf("/71 ");   break;
        case MR1:  printf("/r1 ");   break;
        case MR12: printf("/r12");   break;
        case MR21: printf("/r21");   break;
        case MI12: printf("/i12");   break;
        case MS12: printf("/s12");   break;
        case MS21: printf("/s21");   break;
        case PF:   printf("+f  ");   break;
        case PI1:  printf("+i1 ");   break;
        case PR1:  printf("+r1 ");   break;
        case PR2:  printf("+r2 ");   break;
        case I1:   printf("i1  ");   break;
        case I2:   printf("i2  ");   break;
        case I3:   printf("i3  ");   break;
        case O1:   printf("o1  ");   break;
        case O2:   printf("o2  ");   break;
        case P1:   printf("p1  ");   break;
        case P2:   printf("p2  ");   break;
        case W6:   printf("w6 " );   break;
        case S1:   printf("$1  ");   break;
        default:   printf("\n- Unexpected Format Byte: %x", fmt);
        }
      first = NO;
      } while(fmt);  
    }
  fclose(fd);
  }

/*
** extract upper case mnemonic field from str into buf
** return buf
*/
get_mn(str, buf, max) char *str, *buf; int max; {
  char *ret;
  ret = skip(3, str);
  if(*ret == '+') {
    prefix = YES;
    ++ret;
    }
  else prefix = NO;
  str = ret;
  while(*str >  ' ')
    if(--max) *buf++ = toupper(*str++);      /* force to upper case */
    else error("\n- Field Buffer Overflow in get_mn()\n");
  *buf = NULL;
  return (ret);
  }

/*
** print page header
*/
header(ln) int *ln; {
  if(ln > 2) fputs("\n\n\n\n\n\n\n\n", stdout);
  fputs("---- instruction ----", stdout);
  fputs("               looks     translation\n", stdout);
  return (2);
  }

/*
** set operand mapping array
*/
setmap() {
  otmap[ONE       ] = ONE;     otnext[ONE       ] = I6;
  otmap[THREE     ] = THREE;   otnext[THREE     ] = I6;

  otmap[I6        ] = I8;      otnext[I6        ] = TYPES +  0;
  otmap[TYPES +  0] = I1632;   otnext[TYPES +  0] = TYPES +  1;
  otmap[TYPES +  1] = I16;     otnext[TYPES +  1] = TYPES +  2;
  otmap[TYPES +  2] = I32;     otnext[TYPES +  2] = TYPES +  3;
  otmap[TYPES +  3] = I6;      otnext[TYPES +  3] = 0;
  otmap[I8        ] = I8;      otnext[I8        ] = I16;
  otmap[I16       ] = I1632;   otnext[I16       ] = TYPES +  4;
  otmap[TYPES +  4] = I16;     otnext[TYPES +  4] = TYPES +  5;
  otmap[I32       ] = I1632;   otnext[I32       ] = TYPES +  5;
  otmap[TYPES +  5] = I32;     otnext[TYPES +  5] = 0;

  otmap[S8        ] = S8;      otnext[S8        ] = TYPES +  6;
  otmap[TYPES +  6] = S1632;   otnext[TYPES +  6] = 0;
  otmap[S1632     ] = S1632;   otnext[S1632     ] = TYPES +  7;
  otmap[TYPES +  7] = S8;      otnext[TYPES +  7] = 0;

  otmap[P32       ] = P3248;   otnext[P32       ] = TYPES +  8;
  otmap[TYPES +  8] = P32;     otnext[TYPES +  8] = 0;
  otmap[P48       ] = P3248;   otnext[P48       ] = TYPES +  9;
  otmap[TYPES +  9] = P48;     otnext[TYPES +  9] = 0;

  otmap[M8        ] = M8;      otnext[M8        ] = TYPES + 10;
  otmap[TYPES + 10] = N8;      otnext[TYPES + 10] = TYPES + 11;
  otmap[TYPES + 11] = _8;      otnext[TYPES + 11] = TYPES + 36;
  otmap[M16       ] = M1632;   otnext[M16       ] = TYPES + 12;
  otmap[TYPES + 12] = M16;     otnext[TYPES + 12] = TYPES + 13;
  otmap[TYPES + 13] = N1632;   otnext[TYPES + 13] = TYPES + 14;
  otmap[TYPES + 14] = N16;     otnext[TYPES + 14] = TYPES + 15;
  otmap[TYPES + 15] = _1632;   otnext[TYPES + 15] = TYPES + 16;
  otmap[TYPES + 16] = _16;     otnext[TYPES + 16] = TYPES + 36;
  otmap[M32       ] = M1632;   otnext[M32       ] = TYPES + 17;
  otmap[TYPES + 17] = M32;     otnext[TYPES + 17] = TYPES + 18;
  otmap[TYPES + 18] = M3248;   otnext[TYPES + 18] = TYPES + 19;
  otmap[TYPES + 19] = M3264;   otnext[TYPES + 19] = TYPES + 20;
  otmap[TYPES + 20] = N1632;   otnext[TYPES + 20] = TYPES + 21;
  otmap[TYPES + 21] = N32;     otnext[TYPES + 21] = TYPES + 22;
  otmap[TYPES + 22] = _1632;   otnext[TYPES + 22] = TYPES + 23;
  otmap[TYPES + 23] = _32;     otnext[TYPES + 23] = TYPES + 36;
  otmap[M48       ] = M3248;   otnext[M48       ] = TYPES + 24;
  otmap[TYPES + 24] = M48;     otnext[TYPES + 24] = TYPES + 36;
  otmap[M64       ] = M64;     otnext[M64       ] = TYPES + 25;
  otmap[TYPES + 25] = M3264;   otnext[TYPES + 25] = TYPES + 36;
  otmap[M80       ] = M80;     otnext[M80       ] = TYPES + 36;

  otmap[_8        ] = _8;      otnext[_8        ] = TYPES + 26;
  otmap[TYPES + 26] = M8;      otnext[TYPES + 26] = TYPES + 36;
  otmap[_16       ] = _1632;   otnext[_16       ] = TYPES + 27;
  otmap[TYPES + 27] = _16;     otnext[TYPES + 27] = TYPES + 28;
  otmap[TYPES + 28] = M1632;   otnext[TYPES + 28] = TYPES + 29;
  otmap[TYPES + 29] = M16;     otnext[TYPES + 29] = TYPES + 36;
  otmap[_32       ] = _1632;   otnext[_32       ] = TYPES + 30;
  otmap[TYPES + 30] = _32;     otnext[TYPES + 30] = TYPES + 31;
  otmap[TYPES + 31] = M1632;   otnext[TYPES + 31] = TYPES + 32;
  otmap[TYPES + 32] = M3248;   otnext[TYPES + 32] = TYPES + 33;
  otmap[TYPES + 33] = M32;     otnext[TYPES + 33] = TYPES + 34;
  otmap[TYPES + 34] = M3264;   otnext[TYPES + 34] = TYPES + 36;
  otmap[_48       ] = M3248;   otnext[_48       ] = TYPES + 35;
  otmap[TYPES + 35] = M48;     otnext[TYPES + 35] = TYPES + 36;
  otmap[_64       ] = M64;     otnext[_64       ] = TYPES + 36;
  otmap[_80       ] = M80;     otnext[_80       ] = TYPES + 36;
  otmap[TYPES + 36] = M;       otnext[TYPES + 36] = 0;

  otmap[AL        ] = AL;      otnext[AL        ] = TYPES + 37;
  otmap[BL        ] = R8;      otnext[BL        ] = TYPES + 38;
  otmap[CL        ] = CL;      otnext[CL        ] = TYPES + 37;
  otmap[DL        ] = R8;      otnext[DL        ] = TYPES + 38;
  otmap[AH        ] = R8;      otnext[AH        ] = TYPES + 38;
  otmap[BH        ] = R8;      otnext[BH        ] = TYPES + 38;
  otmap[CH        ] = R8;      otnext[CH        ] = TYPES + 38;
  otmap[DH        ] = R8;      otnext[DH        ] = TYPES + 38;
  otmap[TYPES + 37] = R8;      otnext[TYPES + 37] = TYPES + 38;
  otmap[TYPES + 38] = _8;      otnext[TYPES + 38] = 0;

  otmap[AX        ] = eAX;     otnext[AX        ] = TYPES + 39;
  otmap[TYPES + 39] = AX;      otnext[TYPES + 39] = TYPES + 40;
  otmap[BX        ] = R1632;   otnext[BX        ] = TYPES + 41;
  otmap[CX        ] = R1632;   otnext[CX        ] = TYPES + 41;
  otmap[DX        ] = DX;      otnext[DX        ] = TYPES + 40;
  otmap[SP        ] = R1632;   otnext[SP        ] = TYPES + 41;
  otmap[BP        ] = R1632;   otnext[BP        ] = TYPES + 41;
  otmap[SI        ] = R1632;   otnext[SI        ] = TYPES + 41;
  otmap[DI        ] = R1632;   otnext[DI        ] = TYPES + 41;
  otmap[TYPES + 40] = R1632;   otnext[TYPES + 40] = TYPES + 41;
  otmap[TYPES + 41] = R16;     otnext[TYPES + 41] = TYPES + 42;
  otmap[TYPES + 42] = _1632;   otnext[TYPES + 42] = TYPES + 43;
  otmap[TYPES + 43] = _16;     otnext[TYPES + 43] = 0;

  otmap[EAX       ] = eAX;     otnext[EAX       ] = TYPES + 44;
  otmap[TYPES + 44] = AX;      otnext[TYPES + 44] = TYPES + 45;
  otmap[EBX       ] = R1632;   otnext[EBX       ] = TYPES + 46;
  otmap[ECX       ] = R1632;   otnext[ECX       ] = TYPES + 46;
  otmap[EDX       ] = R1632;   otnext[EDX       ] = TYPES + 46;
  otmap[ESP       ] = R1632;   otnext[ESP       ] = TYPES + 46;
  otmap[EBP       ] = R1632;   otnext[EBP       ] = TYPES + 46;
  otmap[ESI       ] = R1632;   otnext[ESI       ] = TYPES + 46;
  otmap[EDI       ] = R1632;   otnext[EDI       ] = TYPES + 46;
  otmap[TYPES + 45] = R1632;   otnext[TYPES + 45] = TYPES + 46;
  otmap[TYPES + 46] = R32;     otnext[TYPES + 46] = TYPES + 47;
  otmap[TYPES + 47] = _1632;   otnext[TYPES + 47] = TYPES + 48;
  otmap[TYPES + 48] = _32;     otnext[TYPES + 48] = 0;

  otmap[CS        ] = CS;      otnext[CS        ] = TYPES + 49;
  otmap[SS        ] = SS;      otnext[SS        ] = TYPES + 49;
  otmap[DS        ] = DS;      otnext[DS        ] = TYPES + 49;
  otmap[ES        ] = ES;      otnext[ES        ] = TYPES + 49;
  otmap[FS        ] = FS;      otnext[FS        ] = TYPES + 49;
  otmap[GS        ] = GS;      otnext[GS        ] = TYPES + 49;
  otmap[TYPES + 49] = xS;      otnext[TYPES + 49] = 0;

  otmap[CR0       ] = CRx;     otnext[CR0       ] = 0;
  otmap[CR2       ] = CRx;     otnext[CR2       ] = 0;
  otmap[CR3       ] = CRx;     otnext[CR3       ] = 0;

  otmap[DR0       ] = DRx;     otnext[DR0       ] = 0;
  otmap[DR1       ] = DRx;     otnext[DR1       ] = 0;
  otmap[DR2       ] = DRx;     otnext[DR2       ] = 0;
  otmap[DR3       ] = DRx;     otnext[DR3       ] = 0;
  otmap[DR6       ] = DRx;     otnext[DR6       ] = 0;
  otmap[DR7       ] = DRx;     otnext[DR7       ] = 0;

  otmap[TR6       ] = TRx;     otnext[TR6       ] = 0;
  otmap[TR7       ] = TRx;     otnext[TR7       ] = 0;

  otmap[ST0       ] = ST0;     otnext[ST0       ] = TYPES + 50;
  otmap[ST1       ] = STx;     otnext[ST1       ] = 0;
  otmap[ST2       ] = STx;     otnext[ST2       ] = 0;
  otmap[ST3       ] = STx;     otnext[ST3       ] = 0;
  otmap[ST4       ] = STx;     otnext[ST4       ] = 0;
  otmap[ST5       ] = STx;     otnext[ST5       ] = 0;
  otmap[ST6       ] = STx;     otnext[ST6       ] = 0;
  otmap[ST7       ] = STx;     otnext[ST7       ] = 0;
  otmap[TYPES + 50] = STx;     otnext[TYPES + 50] = 0;

  ot_high     = 50;
  }

/*
** verify that OTPLUS is set properly
*/
verify() {
  if(OTPLUS < ot_high) {
    fprintf(stderr, "\n+ adjust OTPLUS and recompile\n");
    exit(11);
    }
  }

/*
** abort with a usage message
*/
usage() {
  error("Usage: CMIT [-#] [-O] [-L] [table]");
  }

